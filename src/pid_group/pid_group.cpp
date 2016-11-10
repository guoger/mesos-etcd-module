// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <algorithm>
#include <set>

#include <process/check.hpp>
#include <process/dispatch.hpp>

#include <stout/check.hpp>

#include "client.hpp"
#include "pid_group.hpp"

using std::string;
using std::vector;
using namespace process;

namespace etcd {

class EtcdPIDGroupProcess : public ProtobufProcess<EtcdPIDGroupProcess>
{
public:
  EtcdPIDGroupProcess(
      const etcd::URL& url,
      const Duration& timeout,
      const std::set<process::UPID>& _base);

  process::Future<Nothing> join(const std::string& pid);

  process::Future<Option<etcd::Node>> watch(
      const Option<uint64_t>& index = None());

  process::Future<Option<etcd::Node>> get();

private:
  etcd::EtcdClient client;
  const Duration ttl;
  // The set of PIDs that are always in the network.
  std::set<process::UPID> base;
  process::Future<Option<etcd::Node>> watching;

  // Max Etcd index we've seen so far, which means we always watch for
  // waitIndex + 1.
  uint64_t waitIndex;

  process::Future<Nothing> _join(
      const process::Future<Option<etcd::Node>>& node);

  process::Future<Nothing> __join(
      const etcd::Node& node);

  process::Future<Option<etcd::Node>> repair(
      const process::Future<Option<etcd::Node>>&);
};


EtcdPIDGroup::EtcdPIDGroup(const etcd::URL& _url, const Duration& _ttl)
  : url(_url), ttl(_ttl)
{
}


Future<Nothing> EtcdPIDGroup::join(const std::string& pid) const
{
  return dispatch(process, &EtcdPIDGroupProcess::join, pid);
}


void EtcdPIDGroup::initialize(const process::UPID& _base)
{
  base = {_base};

  set(base);

  process = new EtcdPIDGroupProcess(url, ttl, base);

  spawn(process);

  dispatch(process, &EtcdPIDGroupProcess::join, _base);

  node = dispatch(process, &EtcdPIDGroupProcess::get);
  node.onAny(
      executor.defer(lambda::bind(&EtcdPIDGroup::_watch, this)));
}


void EtcdPIDGroup::watch(const Option<uint64_t>& index)
{
  node = dispatch(process, &EtcdPIDGroupProcess::watch, index);
  node.onAny(
      executor.defer(lambda::bind(&EtcdPIDGroup::_watch, this)));
}


void EtcdPIDGroup::_watch()
{
  if (!node.isReady()) {
    LOG(INFO) << "Failed to watch Etcd, retry";
    watch();
    return;
  }

  CHECK_READY(node);

  if (node.get().isNone() ||
      node.get().get().nodes.isNone()) {
    LOG(INFO) << "No data found, retry";
    watch();
    return;
  }

  const vector<Node>& nodes = node.get().get().nodes.get();
  std::set<process::UPID> pids;
  uint64_t index = 1;

  foreach (const Node& _node, nodes) {
    process::UPID pid(_node.value.get());
    CHECK(pid) << "Failed to parse '" << _node.value.get() << "'";
    pids.insert(pid);

    index = std::max(index, _node.modifiedIndex.get());
  }

  LOG(INFO) << "Etcd group PIDs: " << stringify(pids);
  set(pids | base);

  watch(index);
}


EtcdPIDGroupProcess::EtcdPIDGroupProcess(
    const etcd::URL& url,
    const Duration& _ttl,
    const std::set<process::UPID>& _base)
  : client(url),
    ttl(_ttl),
    base(_base),
    watching(None()),
    waitIndex(1)
{
}


Future<Nothing> EtcdPIDGroupProcess::join(const string& pid)
{
  return client.join(pid, ttl)
    .then(defer(self(), &Self::_join, lambda::_1));
}


Future<Option<Node>> EtcdPIDGroupProcess::get()
{
  return client.get();
}


Future<Nothing> EtcdPIDGroupProcess::_join(const Future<Option<Node>>& node)
{
  if (node.isFailed() || node.isDiscarded()) {
    LOG(FATAL) << "Failed to insert pid";
  }

  Option<Node> _node = node.get();
  CHECK_SOME(_node);

  Option<uint64_t> waitIndex = _node.get().modifiedIndex.get() + 1;
  Option<string> key = _node.get().key;

  // Reuse previous watch if possbile, otherwise create anew.
  if (!watching.isPending()) {
    watching = client.watch(waitIndex, key);
  }

  return watching
    .after(Seconds(ttl * 8 / 10), defer(self(), &Self::repair, lambda::_1))
    .repair(defer(self(), &Self::repair, lambda::_1))
    .then(defer(self(), &Self::__join, _node.get()));
}


Future<Nothing> EtcdPIDGroupProcess::__join(const Node& node)
{
  return client.create(
      node.key,
      None(),
      ttl,
      true,
      None(),
      None(),
      true)
    .then(defer(self(), &Self::_join, lambda::_1));
}


Future<Option<Node>> EtcdPIDGroupProcess::repair(
    const Future<Option<Node>>& future)
{
  return None();
}


Future<Option<Node>> EtcdPIDGroupProcess::watch(const Option<uint64_t>& index)
{
  if (index.isSome()) {
    waitIndex = std::max(index.get(), waitIndex);
  }

  return client.watch(waitIndex + 1, None(), true)
    .then(defer(self(), [this](const Future<Option<Node>>& node){
      CHECK_READY(node);
      // An empty node is most likely due to watch error 401, outdated index,
      // which indicates that join has not been completed yet. We simply ignore
      // the error here because we expect join to be completed soon.
      // TODO(guoger) Use index in reponse with error code 401.
      if (node.get().isSome()) {
        CHECK_SOME(node.get().get().modifiedIndex);
        waitIndex = std::max(node.get().get().modifiedIndex.get(), waitIndex);
      }
      return client.get();
    }));
}

} // namespace etcd {
