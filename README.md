# Mesos Contender and Detector Modules for etcd
*Most of the code are borrowed from [previous work done](https://github.com/lins05/mesos/tree/etcd) and [an example Mesos module](https://github.com/mesos/modules). This work is still under active development and may change on daily basis.*

# How-To
## Linux
### Step 1. Build Mesos
Refer to [Mesos getting started](http://mesos.apache.org/gettingstarted/)

Supply `--enable-install-module-dependencies` to `configure`. This installs 3rdparty libraries that modules depend on. If you don't want system-wide Mesos installation, supply `--prefix=/path/to/install/location` as well. `make install` when compilation is finished.

### Step 2. Build Mesos contender and detector module
```
git clone https://github.com/guoger/mesos-etcd-module.git && cd mesos-etcd-module
./bootstrap
mkdir build && cd build
../configure --with-mesos=$MESOS_INSTALL CXXFLAGS="-I$MESOS_INSTALL/include -I$MESOS_INSTALL/lib/mesos/3rdparty/include -I$MESOS_INSTALL/lib/mesos/3rdparty/usr/local/include"
make
```
`$MESOS_INSTALL` is the install path specified in previous step. Running above commands produces shared library _libmesos_etcd_module-0.1.so_ in _build/.libs/_.

### Step 3. Create Module config file
An example JSON config file `etcd_module.json.sample` can be found in root directory of the project. Grab a copy, replace `file` and `url` value according to your environment.

### Step 4. Start Mesos
The easiest way to try out the module is to have an etcd instance running locally. See [this guide](https://github.com/coreos/etcd#running-etcd).

After etcd is started, run multiple Mesos master instances `./bin/mesos-master.sh` with flags:
`--modules="file:///path/to/etcd_module.json"`, `--master_contender=org_apache_mesos_EtcdMasterContender`, `--master_detector=org_apache_mesos_EtcdMasterDetector`
One of the masters will get elected as leader. Killing the leader will result in another round of contending until new leader being elected.

Run Mesos agent `./bin/mesos-agent.sh` with flags:
`--modules="file:///path/to/etcd_module.json"`, `--master_detector=org_apache_mesos_EtcdMasterDetector`.

## OSX
_work in progress_

## Windows
_help needed_