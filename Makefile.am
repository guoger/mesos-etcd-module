# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License

AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4

# We want to install modules in mesos directory.
pkglibdir = $(libdir)/mesos

# Initialize variables here so we can use += operator everywhere else.
pkglib_LTLIBRARIES =

# Add compiler and linker flags for pthreads.
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LIBS = $(PTHREAD_LIBS)

# Setup CPPFLAGS that are used for most source files.
AM_CPPFLAGS = $(MESOS_CPPFLAGS) -Wall -Werror

# Library containing test contender module.
pkglib_LTLIBRARIES += libmesos_etcd_module.la
libmesos_etcd_module_la_SOURCES = \
  src/contender/contender.cpp \
  src/contender/etcd.cpp \
  src/detector/etcd.cpp \
  src/client.cpp \
  src/url.cpp \
  src/etcd_module.cpp

libmesos_etcd_module_la_CPPFLAGS = -I$(top_srcdir)/src
libmesos_etcd_module_la_LDFLAGS = -release $(PACKAGE_VERSION) -shared $(MESOS_LDFLAGS)