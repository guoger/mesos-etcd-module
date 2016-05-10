/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <signal.h> // For strsignal.
#include <stdio.h>  // For freopen.
#include <sys/wait.h> // For wait (and associated macros).

#include <stout/os.hpp>

#include "common/status_utils.hpp"

using std::string;

void execute()
{
  Result<string> path = os::realpath("../tests/etcd_tests.sh");
  if (!path.isSome()) {
    FAIL() << "Failed to locate script etcd_tests.sh: "
           << (path.isError() ? path.error() : "No such file or directory");
  }

  // Fork a process to change directory and run the test.
  pid_t pid;
  if ((pid = fork()) == -1) {
    FAIL() << "Failed to fork to launch script";
  }

  if (pid > 0) {
    // In parent process.
    int status;
    while (wait(&status) != pid || WIFSTOPPED(status));

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
      FAIL() << path.get().c_str() << " " << WSTRINGIFY(status);
    }
  } else {
    // In child process. 
    execl(path.get().c_str(), "", (char*) NULL);
  }
}

TEST(EtcdTest, MultiMaster) {
  execute();  
}

