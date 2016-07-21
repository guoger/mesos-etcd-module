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

#ifndef __CONTENDER_ETCD_HPP__
#define __CONTENDER_ETCD_HPP__

//#include <mesos/mesos.hpp>

#include <mesos/master/contender.hpp>
#include <mesos/etcd/url.hpp>

using namespace mesos;
using namespace mesos::master::contender;

namespace etcd {
namespace contender {

const Duration DEFAULT_ETCD_TTL = Seconds(10);

// Forward declarations.
class EtcdMasterContenderProcess;

class EtcdMasterContender : public MasterContender
{
public:
  explicit EtcdMasterContender(const etcd::URL& url);
  virtual ~EtcdMasterContender();
  virtual void initialize(const MasterInfo& masterInfo);
  virtual process::Future<process::Future<Nothing>> contend();

private:
  EtcdMasterContenderProcess* process;
};

} // namespace contender {
} // namespace etcd {

#endif // __CONTENDER_ETCD_HPP__
