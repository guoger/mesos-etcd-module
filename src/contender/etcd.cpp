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

#include <process/future.hpp>

#include "etcd.hpp"

using namespace mesos;
using namespace process;
//using namespace mesos::master;
using namespace mesos::master::contender;

namespace etcd {
namespace contender {

EtcdMasterContender::EtcdMasterContender()
{
  std::cout << "######### Contender Constructor!" << std::endl;
}

EtcdMasterContender::~EtcdMasterContender()
{
  std::cout << "######### Contender Destructor" << std::endl;
}

void EtcdMasterContender::initialize(const MasterInfo& masterInfo)
{
  std::cout << "######### initialize()" << std::endl;
}

Future<Future<Nothing>> EtcdMasterContender::contend()
{
  std::cout << "######### contend()" << std::endl;
  Future<Future<Nothing>> f;
  return f;
}

} // namespace contender {
} // namespace etcd {
