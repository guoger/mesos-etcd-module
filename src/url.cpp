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
#include <vector>

#include <stout/error.hpp>
#include <stout/numify.hpp>
#include <stout/option.hpp>
#include <stout/strings.hpp>

#include <mesos/etcd/url.hpp>

using std::string;
using std::vector;


Try<etcd::URL> etcd::URL::parse(string url)
{
  url = strings::trim(url);

  // Check and remove etcd:// scheme.
  if (!strings::startsWith(url, etcd::URL::scheme())) {
    return Error("Expecting '" + etcd::URL::scheme() +
                 "' at the beginning of the URL");
  }

  url = url.substr(etcd::URL::scheme().size());

  // Look for the trailing first '/' after the scheme which marks the
  // start of the path.
  size_t index = url.find_first_of('/');

  if (index == string::npos) {
    return Error("Expecting an etcd API v2 key path");
  }

  // Grab the path and remove it from the rest of the URL.
  string path = url.substr(index);
  url = url.substr(0, index);

  // Now split out the servers and validate their values.
  vector<Server> servers;

  foreach (const string& server, strings::split(url, ",")) {
    // TODO(cmaloney): This doesn't work for IPv6.
    vector<string> parts = strings::split(server, ":");

    if (parts.size() == 1) {
      servers.push_back(etcd::URL::Server{parts[0], 4001});
    } else if (parts.size() == 2) {
      Try<uint16_t> port = numify<uint16_t>(parts[1]);
      if (port.isError()) {
        return Error("Invalid port '" + parts[1] + "' for server '" + server +
                     "': " + port.error());
      }
      servers.push_back(etcd::URL::Server{parts[0], port.get()});
    } else {
      return Error("Expecting 'host' or 'host:port'; "
                   "IPv6 addresses are not currently supported");
    }
  }

  // Ensure we found at least one server.
  if (servers.empty()) {
    return Error("At least one etcd server must be specified");
  }

  // The path should begin with '/v2/keys/'.
  if (!strings::startsWith(path, "/v2/keys/")) {
    return Error("Must pass a etcd v2 API path which begins with /v2/keys/ "
                 "(got '" + path + "')");
  }

  // Only operate on etcd keys not directories.
  if (path[path.size() - 1] == '/') {
    return Error("Specified etcd path can't end with '/'");
  }

  return etcd::URL{std::move(servers), std::move(path)};
}
