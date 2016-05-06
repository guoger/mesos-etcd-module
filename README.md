# A Mesos Contender and Detector Module based on Etcd
Mostly copied from https://github.com/mesos/modules

# How-To
Basically you need to supply --with-XXX=/path/to/pkg to `configure` for either Mesos or Module, in order to make sure they are compiled against same dependencies.

## OSX
### Install dependencies:
1. google-protobuf
2. glog
3. boost
4. picojson

### Compile Mesos with system packages instead of those ones coming with Mesos
```
../configure --with-glog=/usr/local --with-protobuf=/usr/local --with-boost=/usr/local --prefix=/$HOME/mesos-install --disable-java --disable-python
make
make install
```
With --prefix, make installs packages to user specified location.


### Compile Module
```
./bootstrap
mkdir build && cd build
../configure --with-mesos=/path/to/mesos/installation
make
```

### Run Mesos master with modules
```
./bin/mesos-master.sh --ip=0.0.0.0 --work_dir=~/workspace/mesos_work_dir/ --modules="file://$MODULE_HOME/etcd_module.json" --master_contender=org_apache_mesos_EtcdMasterContender --master_detector=org_apache_mesos_EtcdMasterDetector
```

_Following content is copied directly from above link_
# Building the Modules

Mesos modules provide a way to easily extend inner workings of Mesos by creating
and using shared libraries that are loaded on demand. Modules can be used to
customize Mesos without having to recompiling/relinking for each specific use
case. Modules can isolate external dependencies into separate libraries, thus
resulting into a smaller Mesos core. Modules also make it easy to experiment
with new features. For example, imagine loadable allocators that contain a VM
(Lua, Python, …) which makes it possible to try out new allocator algorithms
written in scripting languages without forcing those dependencies into the
project. Finally, modules provide an easy way for third parties to easily extend
Mesos without having to know all the internal details.

For more details, please see
[Mesos Modules](http://mesos.apache.org/documentation/latest/modules/).


## Prerequisites

Building Mesos modules requires system-wide installation of the following:

1. google-protobuf
2. glog
3. boost
4. picojson

## Build Mesos with some unbundled dependencies

### Preparing Mesos source code
First we need to prepare Mesos source code.  You can either download the Mesos
standard release in the form of a tarball and extract it, or clone the git
repository.

Let us assume you did extract/clone
the repository into `~/mesos`. Let us also assume that you build Mesos in a
subdirectory
called `build` (`~/mesos/build`).

### Building and Installing Mesos
Next, we need to configure and build Mesos.
Due to the fact that modules will need to have access to a couple of libprocess
dependencies, Mesos itself should get built with unbundled dependencies to
reduce chances of problems introduced by varying versions (libmesos vs. module
library).

We recommend using the following configure options:

```
cd <mesos-source-tree>
mkdir build
cd build
../configure --with-glog=/usr/local --with-protobuf=/usr/local --with-boost=/usr/local --prefix=$HOME/usr
make
make install
```

Note that the `--prefix=$HOME/usr` is required only if you don't want to do a system-wide Mesos installation.

## Build Mesos Modules

Once that is done, extract/clone the mesos-modules package. For the sake of this
example, that could be in `~/mesos-modules`. Note that you should not put
`mesos-modules` into the `mesos` folder.

You may now run start building the modules.

The configuration phase needs to know some details about your Mesos installation
location, hence the following are used:
`--with-mesos=/path/to/mesos/installation`

## Example
```
./bootstrap
mkdir build && cd build
../configure --with-mesos=/path/to/mesos/installation
make
```

At this point, the Module libraries are ready in `/build/.libs`.

## Using Mesos Modules
See [Mesos Modules](http://mesos.apache.org/documentation/latest/modules/).