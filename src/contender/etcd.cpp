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

#include <mesos/mesos.hpp>

#include <mesos/master/contender.hpp>

#include <process/defer.hpp>
#include <process/future.hpp>
#include <process/owned.hpp>
#include <process/pid.hpp>
#include <process/process.hpp>

#include <stout/check.hpp>
#include <stout/lambda.hpp>
#include <stout/protobuf.hpp>

#include "etcd.hpp"
#include "contender.hpp"
#include "client.hpp"

using namespace mesos;
using namespace process;
//using namespace mesos::master;
using namespace mesos::master::contender;

namespace etcd {
namespace contender {

class EtcdMasterContenderProcess : public Process<EtcdMasterContenderProcess>
{
public:
  EtcdMasterContenderProcess(const etcd::URL& _url) : contender(NULL), url(_url)
  {
  }

  virtual ~EtcdMasterContenderProcess()
  {
    // TODO(cmaloney): If currently the leader, then delete the key.
    // Currently the key will naturally time out after the TTL.
  }

  // Explicitely use 'initialize' since we're overloading below.
  using process::ProcessBase::initialize;

  // MasterContender implementation.
  void initialize(const MasterInfo& masterInfo);
  Future<Future<Nothing>> contend();

private:
  LeaderContender* contender;

  const etcd::URL url;
  Option<MasterInfo> masterInfo;
};


EtcdMasterContender::EtcdMasterContender(const etcd::URL& url)
{
  process = new EtcdMasterContenderProcess(url);
  spawn(process);
}


EtcdMasterContender::~EtcdMasterContender()
{
  terminate(process);
  process::wait(process);
  delete process;
}


void EtcdMasterContender::initialize(const MasterInfo& masterInfo)
{
  process->initialize(masterInfo);
}


Future<Future<Nothing>> EtcdMasterContender::contend()
{
  return dispatch(process, &EtcdMasterContenderProcess::contend);
}


void EtcdMasterContenderProcess::initialize(const MasterInfo& _masterInfo)
{
  masterInfo = _masterInfo;
}


Future<Future<Nothing>> EtcdMasterContenderProcess::contend()
{
  if (masterInfo.isNone()) {
    return Failure("Initialize the contender first");
  }

  // Check if we're already contending.
  if (contender != NULL) {
    LOG(INFO) << "Withdrawing the previous contending before recontending";
    delete contender;
  }

  // Serialize the MasterInfo to JSON.
  JSON::Object json = JSON::protobuf(masterInfo.get());

  contender = new etcd::contender::LeaderContender(url, stringify(json), DEFAULT_ETCD_TTL);
  return contender->contend();
}

} // namespace contender {
} // namespace etcd {
