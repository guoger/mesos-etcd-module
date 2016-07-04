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
#include <mesos/module.hpp>

#include <mesos/module/contender.hpp>
#include <mesos/module/detector.hpp>

#include <mesos/master/contender.hpp>
#include <mesos/master/detector.hpp>

#include "contender/etcd.hpp"
#include "detector/etcd.hpp"

#include "url.hpp"

//using namespace mesos;
using namespace mesos::master::contender;
using namespace mesos::master::detector;

static MasterContender* createContender(const Parameters& parameters)
{
  Option<std::string> urls;
  Option<std::string> retry_interval;
  Option<std::string> retry_times;
  Option<std::string> ttl;
  foreach (const Parameter& parameter, parameters.parameter()) {
    if (parameter.key() == "url") {
      urls = parameter.value();
    }

    if (parameter.key() == "retry_interval") {
      retry_interval = parameter.value();
      std::cout << "Parameter retry_interval: " << retry_interval.get() << std::endl;
    }

    if (parameter.key() == "retry_times") {
      retry_times = parameter.value();
    }

    if (parameter.key() == "ttl") {
      ttl = parameter.value();
    }
  }

  // Try to parse urls from parameters.
  if (urls.isNone()) {
    LOG(ERROR) << "No etcd URLs provided";
    return NULL;
  }

  Try<etcd::URL> urls_ = etcd::URL::parse(urls.get());
  if (urls_.isError()) {
    LOG(ERROR) << "Parameter '" << urls.get() << "' could not be parsed into "
                  "a valid URL object: " << urls_.error();
    return NULL;
  }

  // Try to parse retry times from parameters.
  uint8_t retry_times__;
  if (retry_times.isNone()) {
    LOG(WARNING) << "No retry times provided, using default value 3";
    retry_times__ = 3;
  } else {
    Try<uint8_t> retry_times_ = numify<uint8_t>(retry_times.get());
    if (retry_times_.isError()) {
      LOG(ERROR) << "Parameter '" << retry_times.get()
                 << "' could not be parsed into a valid uint8_t: "
                 << retry_times_.error();
      return NULL;
    }
    retry_times__ = retry_times_.get();
  }

  // Try to parse retry interval from parameters.
  Duration retry_interval__;
  if (retry_interval.isNone()) {
    LOG(WARNING) << "No retry interval provided, using default value 3sec";
    retry_interval__ = Seconds(3);
  } else {
    Try<Duration> retry_interval_ = Duration::parse(retry_interval.get());
    if (retry_interval_.isError()) {
      LOG(ERROR) << "Parameter '" << retry_interval.get()
                 << "' could not be parsed into a valid Duration: "
                 << retry_interval_.error();
      return NULL;
    }
    retry_interval__ = retry_interval_.get();
  }

  // Try to parse retry interval from parameters.
  Duration ttl__;
  if (ttl.isNone()) {
    LOG(WARNING) << "No retry TTL provided, using default value 5sec";
    ttl__ = Seconds(5);
  } else {
    Try<Duration> ttl_ = Duration::parse(ttl.get());
    if (ttl_.isError()) {
      LOG(ERROR) << "Parameter '" << ttl.get()
                 << "' could not be parsed into a valid Duration: "
                 << ttl_.error();
      return NULL;
    }
    ttl__ = ttl_.get();
  }

  return new etcd::contender::EtcdMasterContender(urls_.get(),
                                                  retry_times__,
                                                  retry_interval__,
                                                  ttl__);
}


// Declares a MasterContender module named
// 'org_apache_mesos_TestMasterContender'.
mesos::modules::Module<MasterContender> org_apache_mesos_EtcdMasterContender(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "Apache Mesos",
    "modules@mesos.apache.org",
    "Test MasterContender module.",
    NULL,
    createContender);


static MasterDetector* createDetector(const Parameters& parameters)
{
  Option<std::string> urls;
  Option<std::string> retry_interval;
  Option<std::string> retry_times;
  foreach (const Parameter& parameter, parameters.parameter()) {
    if (parameter.key() == "url") {
      urls = parameter.value();
    }

    if (parameter.key() == "retry_interval") {
      retry_interval = parameter.value();
    }

    if (parameter.key() == "retry_times") {
      retry_times = parameter.value();
    }
  }

  if (urls.isNone()) {
    LOG(ERROR) << "No etcd URLs provided";
    return NULL;
  }

  Try<etcd::URL> urls_ = etcd::URL::parse(urls.get());
  if (urls_.isError()) {
    LOG(ERROR) << "Parameter '" << urls.get() << "' could not be parsed into "
                  "a valid URL object: " << urls_.error();
    return NULL;
  }

  // Try to parse retry times from parameters.
  uint8_t retry_times__;
  if (retry_times.isNone()) {
    LOG(WARNING) << "No retry times provided, using default value 3";
    retry_times__ = 3;
  } else {
    Try<uint8_t> retry_times_ = numify<uint8_t>(retry_times.get());
    if (retry_times_.isError()) {
      LOG(ERROR) << "Parameter '" << retry_times.get()
                 << "' could not be parsed into a valid uint8_t: "
                 << retry_times_.error();
      return NULL;
    }
  }

  // Try to parse retry interval from parameters.
  Duration retry_interval__;
  if (retry_interval.isNone()) {
    LOG(WARNING) << "No retry interval provided, using default value 3sec";
    retry_interval__ = Seconds(3);
  } else {
    Try<Duration> retry_interval_ = Duration::parse(retry_interval.get());
    if (retry_interval_.isError()) {
      LOG(ERROR) << "Parameter '" << retry_interval.get()
                 << "' could not be parsed into a valid Duration: "
                 << retry_interval_.error();
      return NULL;
    }
  }

  return new etcd::detector::EtcdMasterDetector(urls_.get(),
                                                retry_times__,
                                                retry_interval__);
}


// Declares a MasterDetector module named
// 'org_apache_mesos_TestMasterDetector'.
mesos::modules::Module<MasterDetector> org_apache_mesos_EtcdMasterDetector(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "Apache Mesos",
    "modules@mesos.apache.org",
    "Test MasterDetector module.",
    NULL,
    createDetector);
