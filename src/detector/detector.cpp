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

#include "detector/detector.hpp"
#include "detector/etcd.hpp"

#include "client.hpp"
#include "url.hpp"

using namespace process;

using std::set;
using std::string;

namespace etcd {

class LeaderDetectorProcess : public Process<LeaderDetectorProcess>
{
public:
  LeaderDetectorProcess(const URL& url,
                        const uint8_t& retry_times,
                        const Duration& retry_interval);

  virtual ~LeaderDetectorProcess();

  Future<Option<string>> detect(const Option<string>& previous)
  {
    // Try and get the current master.
    return client.get()
      .then(defer(self(), &Self::_detect, previous, lambda::_1));
  }

private:
  Future<Option<string>> _detect(const Option<string>& previous,
                                 const Option<etcd::Node>& node);
  Future<Option<etcd::Node>> repair(const Future<Option<etcd::Node>>&);

  EtcdClient client;
};


LeaderDetectorProcess::LeaderDetectorProcess(const URL& url,
                                             const uint8_t& retry_times,
                                             const Duration& retry_interval)
  : client(url, retry_times, retry_interval)
{
}


LeaderDetectorProcess::~LeaderDetectorProcess()
{
}


Future<Option<string>> LeaderDetectorProcess::_detect(
  const Option<string>& previous, const Option<etcd::Node>& node)
{
  // Check and see if the node still exists.
  if (node.isNone() && previous.isSome()) {
    return None();
  }

  // If we need to continue watching then we'll use a 'waitIndex' to
  // pass to etcd extracted from the 'modifiedIndex' from the
  // node. But it's also possible that 'node' is None in which case
  // we'll leave 'waitIndex' as None as well).
  Option<uint64_t> waitIndex = None();

  // Determine if we have a newly elected master or need to keep
  // waiting by watching.
  if (node.isSome() && node.get().value.isSome()) {
    string info = node.get().value.get();
    // Check if we have a newly detected master.
    if (previous != info) {
      VLOG(2) << "master info changed in etcd";
      return info;
    }

    if (node.get().modifiedIndex.isSome()) {
      // In order to watch for the next change we want
      // 'modifiedIndex + 1'.
      waitIndex = node.get().modifiedIndex.get() + 1;
    }
  }

  // NOTE: We're explicitly ignoring the return value of
  // 'etcd::watch' since we can't distinguish a failed future from
  // when etcd might have closed our connection because we were
  // connected for the maximum watch time limit. Instead, we simply
  // retry after 'etcd::watch' completes or fails.
  return client.watch(waitIndex)
    .repair(defer(self(), &Self::repair, lambda::_1))
    .then(defer(self(), &Self::detect, previous));
}


Future<Option<etcd::Node>> LeaderDetectorProcess::repair(
  const Future<Option<etcd::Node>>&)
{
  // We "repair" the future by just returning None as that will
  // cause the detection loop to continue.
  return None();
}


LeaderDetector::LeaderDetector(const URL& url,
                               const uint8_t& retry_times,
                               const Duration& retry_interval)
{
  process = new LeaderDetectorProcess(url, retry_times, retry_interval);
  spawn(process);
}


LeaderDetector::~LeaderDetector()
{
  terminate(process);
  process::wait(process);
  delete process;
}


Future<Option<string>> LeaderDetector::detect(const Option<string>& previous)
{
  return dispatch(process, &LeaderDetectorProcess::detect, previous);
}


} // namespace etcd {
