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

#ifndef __ETCD_PID_GROUP_HPP__
#define __ETCD_PID_GROUP_HPP__

#include <process/pid_group.hpp>

#include "client.hpp"
#include "url.hpp"

namespace etcd {

// Forward declaration
class EtcdPIDGroupProcess;

class EtcdPIDGroup : public process::PIDGroup
{
public:
  EtcdPIDGroup(const etcd::URL& url, const Duration& ttl);

  process::Future<Nothing> join(const std::string& pid) const;

  void initialize(const process::UPID& _base);

private:
  const etcd::URL url;
  const Duration ttl;
  process::Future<Option<etcd::Node>> node;
  EtcdPIDGroupProcess* process;
  process::Executor executor;

  process::Future<Option<etcd::Node>> _join(const std::string& pid);

  void watch(const Option<uint64_t>& index = None());
  void _watch();
};

} // namespace etcd {

#endif // __ETCD_PID_GROUP_HPP__
