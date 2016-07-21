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

#ifndef __ETCD_CONTENDER_HPP
#define __ETCD_CONTENDER_HPP

#include <string>

#include <process/future.hpp>

#include <stout/nothing.hpp>
#include <stout/option.hpp>

#include <mesos/etcd/client.hpp>

namespace etcd {
namespace contender {

// Forward declaration.
class LeaderContenderProcess;


// Provides an abstraction for contending to be the leader using etcd.
class LeaderContender
{
public:
  LeaderContender(const URL& url, const std::string& data, const Duration& ttl);

  virtual ~LeaderContender();

  process::Future<process::Future<Nothing> > contend();

private:
  LeaderContenderProcess* process;
};

} // namespace contender {
} // namespace etcd {

#endif // __ETCD_CONTENDER_HPP
