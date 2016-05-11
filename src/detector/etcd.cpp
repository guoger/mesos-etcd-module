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

#include <string>

#include <mesos/mesos.hpp>
#include <mesos/master/detector.hpp>
#include <mesos/type_utils.hpp>

#include <process/defer.hpp>
#include <process/future.hpp>
#include <process/owned.hpp>
#include <process/pid.hpp>
#include <process/process.hpp>

#include <stout/check.hpp>
#include <stout/lambda.hpp>
#include <stout/protobuf.hpp>

#include "etcd.hpp"
#include "detector.hpp"

using namespace mesos;
using namespace process;
using namespace mesos::master::detector;

using std::string;

namespace etcd {
namespace detector {

class EtcdMasterDetectorProcess : public Process<EtcdMasterDetectorProcess>
{
public:
  explicit EtcdMasterDetectorProcess(const etcd::URL& _url)
    : detector(_url), url(_url)
  {
  }

  virtual ~EtcdMasterDetectorProcess()
  {
  }

  Future<Option<MasterInfo>> detect(const Option<MasterInfo>& previous);
private:
  Future<Option<MasterInfo>> detected(const Option<MasterInfo>& previous,
                                      const Option<string>& data);

  etcd::LeaderDetector detector;
  const etcd::URL url;
};


Future<Option<MasterInfo>> EtcdMasterDetectorProcess::detect(
  const Option<MasterInfo>& previous)
{
  // Try and get the current master.
  Option<string> data;
  if (previous.isSome()) {
    data = stringify(JSON::protobuf(previous.get()));
  }
  return detector.detect(data)
    .then(defer(self(), &Self::detected, previous, lambda::_1));
}


Future<Option<MasterInfo>> EtcdMasterDetectorProcess::detected(
  const Option<MasterInfo>& previous, const Option<string>& data)
{
  // Check and see if the node still exists.
  if (data.isNone() && previous.isSome()) {
    return None();
  }

  // Determine if we have a newly elected master
  if (data.isSome()) {
    Try<JSON::Value> json = JSON::parse(data.get());
    if (json.isError()) {
      return Failure("Failed to parse JSON: " + json.error());
    }

    Try<MasterInfo> info = ::protobuf::parse<MasterInfo>(json.get());
    if (info.isError()) {
      return Failure("Failed to parse MasterInfo from JSON: " + info.error());
    }

    // Check if we have a newly detected master.
    if (previous != info.get()) {
      return info.get();
    }
  }

  // Keep detecting
  return detector.detect(data)
    .then(defer(self(), &Self::detected, previous, lambda::_1));
}


EtcdMasterDetector::EtcdMasterDetector(const etcd::URL& url)
{
  process = new EtcdMasterDetectorProcess(url);
  spawn(process);
}


EtcdMasterDetector::~EtcdMasterDetector()
{
  terminate(process);
  process::wait(process);
  delete process;
}


Future<Option<MasterInfo>> EtcdMasterDetector::detect(
  const Option<MasterInfo>& previous)
{
  return dispatch(process, &EtcdMasterDetectorProcess::detect, previous);
}


} // namespace detector {
} // namespace etcd {
