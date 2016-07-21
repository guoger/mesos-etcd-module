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

#ifndef __DETECTOR_ETCD_HPP__
#define __DETECTOR_ETCD_HPP__

//#include <mesos/mesos.hpp>

#include <mesos/master/detector.hpp>

#include <mesos/etcd/url.hpp>

using namespace mesos;
using namespace mesos::master::detector;

namespace etcd {
namespace detector {

// Forward declarations.
class EtcdMasterDetectorProcess;

class EtcdMasterDetector : public MasterDetector
{
public:
  // Creates a detector which uses etcd to determine (i.e., elect) a
  // leading master.
  explicit EtcdMasterDetector(const etcd::URL& url);

  virtual ~EtcdMasterDetector();

  // MasterDetector implementation.
  virtual process::Future<Option<MasterInfo>> detect(
      const Option<MasterInfo>& previous = None());

private:
  EtcdMasterDetectorProcess* process;
};

} // namespace detector {
} // namespace etcd {

#endif // __DETECTOR_ETCD_HPP__
