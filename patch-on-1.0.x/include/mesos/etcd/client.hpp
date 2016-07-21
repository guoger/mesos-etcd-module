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

#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include <string>

#include <process/future.hpp>
#include <process/shared.hpp>

#include <stout/duration.hpp>
#include <stout/json.hpp>
#include <stout/option.hpp>
#include <stout/try.hpp>

#include "url.hpp"

namespace etcd {

struct Node;
class EtcdClientProcess;

// This class implements etcd client api
class EtcdClient
{
public:
  EtcdClient(const URL& url, const Option<Duration>& defaultTTL = None());

  process::Future<Option<Node>> create(
      const Option<std::string>& key,
      const Option<std::string>& value,
      const Option<Duration>& ttl = None(),
      const Option<bool> prevExist = None(),
      const Option<uint64_t>& prevIndex = None(),
      const Option<std::string>& prevValue = None(),
      const Option<bool> refresh = None());

  process::Future<Option<Node>> get();

  process::Future<Option<Node>> watch(
      const Option<uint64_t>& waitIndex = None(),
      const Option<std::string>& key = None(),
      const Option<bool> recursive = None());

  process::Future<Option<Node>> join(
      const std::string& pid,
      const Duration& ttl) const;

private:
  EtcdClientProcess* process;
};


// Represents the JSON structure etcd returns for a node (key/value).
struct Node {
  static Try<Node*> parse(const JSON::Object& object);

  Option<uint64_t> createdIndex;
  Option<std::string> expiration;
  std::string key;
  Option<uint64_t> modifiedIndex;
  Option<Duration> ttl;
  Option<std::string> value;
  Option<bool> dir;
  Option<std::vector<Node>> nodes;

  process::Shared<Node> previous;

private:
  // Require everyone to call 'parse'.
  Node()
  {
  }
};


// Represents the JSON structure etcd returns for each call.
struct Response {
  // Returns an etcd "response" from a JSON object.
  static Try<Response> parse(const Try<JSON::Object>& object);

  // Fields present if the request was successful. Note, however, that
  // since our APIs return just the Node rather than the entire
  // Response we capture 'prevNode' within Node as Node::previous so
  // that it is accessible.
  Option<std::string> action;
  Option<Node> node;

  // Fields present if the response represents an error.
  Option<int> errorCode;
  Option<std::string> cause;
  Option<std::string> message;
  Option<uint64_t> index;
};

} // namespace etcd {

#endif // __CLIENT_HPP__
