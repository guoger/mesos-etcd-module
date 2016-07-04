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

#include <set>
#include <string>

#include <process/defer.hpp>
#include <process/dispatch.hpp>
#include <process/future.hpp>
#include <process/id.hpp>

#include <stout/check.hpp>
#include <stout/lambda.hpp>
#include <stout/option.hpp>
#include <stout/some.hpp>

#include "contender/contender.hpp"
#include "contender/etcd.hpp"

#include "client.hpp"
#include "url.hpp"

using namespace process;

using std::set;
using std::string;

namespace etcd {
namespace contender {

class LeaderContenderProcess : public Process<LeaderContenderProcess>
{
public:
  LeaderContenderProcess(const URL& url,
                         const string& data,
                         const uint8_t& retry_times,
                         const Duration& retry_interval,
                         const Duration& ttl);

  virtual ~LeaderContenderProcess();

  // LeaderContender implementation.
  Future<Future<Nothing>> contend();

protected:
  virtual void finalize();

private:
  etcd::EtcdClient client;
  const Duration ttl;
  string data;

  // Continuations.
  Future<Nothing> _contend(const Option<etcd::Node>& node);
  Future<Nothing> __contend(const Option<etcd::Node>& node);
  Future<Nothing> ___contend(const etcd::Node& node);

  // Helper for repairing failures from etcd::watch.
  Future<Option<etcd::Node>> repair(const Future<Option<etcd::Node>>&);

  Option<Future<Nothing>> future;
  Option<Promise<Future<Nothing>>*> contending;
};


LeaderContenderProcess::LeaderContenderProcess(const URL& _url,
                                               const string& _data,
                                               const uint8_t& _retry_times,
                                               const Duration& _retry_interval,
                                               const Duration& _ttl)
  : client(_url, _retry_times, _retry_interval),
    data(_data),
    ttl(_ttl)
{
}


LeaderContenderProcess::~LeaderContenderProcess()
{
}


void LeaderContenderProcess::finalize()
{
  // TODO(lins05): implementation
}


Future<Future<Nothing>> LeaderContenderProcess::contend()
{
  VLOG(2) << "contending with data" << data;
  return client.get()
    .repair(defer(self(), &Self::repair, lambda::_1))
    .then(defer(self(), &Self::_contend, lambda::_1));
}


Future<Nothing> LeaderContenderProcess::_contend(const Option<etcd::Node>& node)
{
  std::cout << "LeaderContenderProcess::_contend" << std::endl;
  if (node.isNone()) {
    return client.create(data, ttl, false)
      .then(defer(self(), &Self::__contend, lambda::_1));
  }

  // A node exists which means someone else is elected and we need to
  // keep watching until we can try again. First check to make sure
  // that it has a value.

  if (node.get().value.isNone()) {
    // TODO(benh): Consider just retrying instead of failing so as to
    // limit this being used to cause a denial-of-service.
    return Failure("Not expecting a missing value");
  }

  // Extract the 'modifiedIndex' from the node so that we can
  // watch for _new_ changes, i.e., 'modifiedIndex + 1'.
  Option<uint64_t> waitIndex = node.get().modifiedIndex.get() + 1;

  // Watch the node until we can try and elect ourselves.
  //
  // NOTE: We're explicitly ignoring the return value of 'etcd::watch
  // since we can't distinguish a failed future from when etcd might
  // have closed our connection because we were connected for the
  // maximum watch time limit. Instead, we simply resume contending
  // after 'etcd::watch' completes or fails.
  return client.watch(waitIndex)
    .repair(defer(self(), &Self::repair, lambda::_1))
    .then(lambda::bind(&EtcdClient::get, client))
    .then(defer(self(), &Self::_contend, lambda::_1));
}


Future<Nothing> LeaderContenderProcess::__contend(
  const Option<etcd::Node>& node)
{
  std::cout << "LeaderContenderProcess::__contend" << std::endl;
  if (node.isNone()) {
    // Looks like we we're able to create (or update) the node before
    // someone else (or our TTL elapsed), either way we are not
    // elected.
    return Nothing();
  }

  // We're now elected, or we're still elected, i.e., the etcd::create
  // was successful! Now we watch the node to make sure that no
  // changes occur (they shouldn't since we should be the incumbent,
  // but better to be conservative). If no changes occur we try and
  // extend our reign after 80% of the TTL has elapsed.

  // Extract the 'modifiedIndex' from the node so that we can
  // watch for _new_ changes, i.e., 'modifiedIndex + 1'.
  Option<uint64_t> waitIndex = node.get().modifiedIndex.get() + 1;

  // Extract the 'ttl' from the node (which we should have set
  // anyways) to use when watching the node for changes.
  Duration ttl = node.get().ttl.getOrElse(ttl);

  // NOTE: We're explicitly ignoring the return value of 'etcd::watch
  // since we can't distinguish a failed future from when etcd might
  // have closed our connection because we were connected for the
  // maximum watch time limit. Instead, we simply resume contending
  // after 'etcd::watch' completes or fails.
  return client.watch(waitIndex)
    .after(Seconds(ttl * 8 / 10), defer(self(), &Self::repair, lambda::_1))
    .repair(defer(self(), &Self::repair, lambda::_1))
    .then(defer(self(), &Self::___contend, node.get()));
}


Future<Nothing> LeaderContenderProcess::___contend(const etcd::Node& node)
{
  std::cout << "LeaderContenderProcess::___contend" << std::endl;
  return client.create(data, ttl, true, node.modifiedIndex)
    .repair(defer(self(), &Self::repair, node))
    .then(defer(self(), &Self::__contend, lambda::_1));
}


Future<Option<etcd::Node>> LeaderContenderProcess::repair(
  const Future<Option<etcd::Node>>&)
{
  // We "repair" the future by just returning None as that will
  // cause the contending loop to continue.
  return None();
}


LeaderContender::LeaderContender(const etcd::URL& url,
                                 const string& data,
                                 const uint8_t& retry_times,
                                 const Duration& retry_interval,
                                 const Duration& ttl)
{
  process = new LeaderContenderProcess(url,
                                       data,
                                       retry_times,
                                       retry_interval,
                                       ttl);
  spawn(process);
}


LeaderContender::~LeaderContender()
{
  terminate(process);
  process::wait(process);
  delete process;
}


Future<Future<Nothing>> LeaderContender::contend()
{
  return dispatch(process, &LeaderContenderProcess::contend);
}

} // namespace contender {
} // namespace etcd {
