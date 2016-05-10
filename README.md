# Mesos Contender and Detector Modules for etcd
*Most of the code are borrowed from https://github.com/lins05/mesos/tree/etcd and https://github.com/mesos/modules. The first link is previous work done before contender and detector being modulerized. The second is an example of Mesos module. This work is still under active development and may change on daily basis.*

# How-To
## Linux
### Step 1. Patch Mesos
_IMPORTANT_ Currently `libprocess` does not have `PUT`, which is required to run contender against etcd. To patch Mesos, You can either checkout [this branch](https://github.com/guoger/mesos/tree/etcd) or patch Mesos according to the diff. We are actively working on merging this into Mesos mainstream.

### Step 2. Build
Refer to http://mesos.apache.org/gettingstarted/

`make install` when compilation is finished. If you don't want system-wide Mesos installation, supply `--prefix=/path/to/install/location` to `configure`

### Step 3. Build Mesos contender and detector module
```
git clone https://github.com/guoger/mesos-etcd-module.git
cd mesos-etcd-module
./bootstrap
mkdir build && cd build
../configure --with-mesos=$MESOS_INSTALL CXXFLAGS="-I$MESOS_HOME/build/3rdparty/libprocess/3rdparty/glog-0.3.3/src/ -I$MESOS_HOME/build/3rdparty/libprocess/3rdparty/protobuf-2.6.1/src/ -I$MESOS_HOME/build/3rdparty/libprocess/3rdparty/boost-1.53.0/ -I$MESOS_HOME/include/"
make
```
`$MESOS_INSTALL` is the install path specified in previous step. `$MESOS_HOME` is Mesos root directory. We compile module against libraries (protobuf, glog, boost) shipped with Mesos source code. These libraries are extracted to Mesos build directory.

Running above commands will produce shared library file `libmesos_etcd_module-0.1.so` in `.libs/`.

### Step 4. Create Module config file
An example JSON config file `etcd_module.json.sample` can be found in root directory of the project. Grab a copy, replace `file` and `url` value according to your environment.

### Step 5. Start Mesos master
Run `./bin/mesos-master.sh` with following additional flags:
`--modules="file://etcd_module.json"`: the JSON file created in previous step
`--master_contender=org_apache_mesos_EtcdMasterContender`
`--master_detector=org_apache_mesos_EtcdMasterDetector`

## OSX
_work in progress_

## Windows
_help needed_

For more details, see [Mesos Modules](http://mesos.apache.org/documentation/latest/modules/).