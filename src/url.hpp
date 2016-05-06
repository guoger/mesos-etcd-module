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

#ifndef __ETCD_URL_HPP__
#define __ETCD_URL_HPP__

#include <string>
#include <vector>

#include <stout/strings.hpp>
#include <stout/try.hpp>

namespace etcd {

// Describes an etcd URL of the form:
//
//     etcd://servers/path
//
// Where 'servers' is of the form:
//
//     host1:port1,host2:port2,host3:port3
//
// Where 'path' is an etcd v2 API key such as:
//
//      /v2/keys/mesos/master
struct URL
{
  static Try<URL> parse(std::string url);

  struct Server
  {
    std::string host; // Hostname or IP.
    uint16_t port;
  };

  const std::vector<Server> servers;

  const std::string path;

  static std::string scheme()
  {
    return "etcd://";
  }
};


inline std::ostream& operator << (
    std::ostream& stream,
    const URL::Server& server)
{
  return stream << server.host << ':' << server.port;
}


inline std::ostream& operator << (std::ostream& stream, const URL& url)
{
  return stream
    << URL::scheme() << strings::join(",", url.servers) << url.path;
}

} // namespace etcd {

#endif // __ETCD_URL_HPP__
