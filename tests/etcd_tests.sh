#!/usr/bin/env bash

# Find MESOS_SOURCE_DIR and MESOS_BUILD_DIR.
env | grep MESOS_SOURCE_DIR >/dev/null

if [ "${?}" -ne 0 ]; then
  echo "Failed to find MESOS_SOURCE_DIR in environment"
  exit 1
fi

env | grep MESOS_BUILD_DIR >/dev/null

if [ "${?}" -ne 0 ]; then
  echo "Failed to find MESOS_BUILD_DIR in environment"
  exit 1
fi

source ${MESOS_SOURCE_DIR}/support/atexit.sh

# Create a work directory in our local testing directory so that
# everything gets cleaned up after the tests complete.
WORK_DIR=`pwd`/work_dir

OS=`uname -s`

ETCD=etcd
ETCDCTL=etcdctl

# Make sure etcd exists, othewise try and download.
${ETCD} --help >/dev/null 2>&1

if [ "${?}" -ne 0 ]; then
  echo "Need to download etcd"
  if [ "${OS}" == 'Linux' ]; then
    curl -L https://github.com/coreos/etcd/releases/download/v2.0.0/etcd-v2.0.0-linux-amd64.tar.gz -o etcd-v2.0.0-linux-amd64.tar.gz
    tar xzvf etcd-v2.0.0-linux-amd64.tar.gz
    ETCD=./etcd-v2.0.0-linux-amd64/etcd
    ETCDCTL=./etcd-v2.0.0-linux-amd64/etcdctl
  elif [ "${OS}" == 'Darwin' ]; then
    curl -L https://github.com/coreos/etcd/releases/download/v2.0.0/etcd-v2.0.0-darwin-amd64.zip -o etcd-v2.0.0-darwin-amd64.zip
    unzip etcd-v2.0.0-darwin-amd64.zip
    ETCD=./etcd-v2.0.0-darwin-amd64/etcd
    ETCDCTL=./etcd-v2.0.0-darwin-amd64/etcdctl
  else
    echo "Unsupported platfrom '${OS}', exiting"
    exit -1
  fi
fi

# Start etcd and make sure it exits when we do.
${ETCD} -data-dir=${WORK_DIR}/etcd &

ETCD_PID=${!}

atexit "kill ${ETCD_PID}"

echo "Started etcd"

# Now give etcd a chance to come up properly so we can use etcdctl
# without it failing because etcd isn't accepting connections yet.
sleep 5

common_flags=(
  # List of nodes for replicated log. Tests using the default port (5050)
  # with the first entry.
  --ip=127.0.0.1
  --masters=127.0.0.1,127.0.0.1:6060,127.0.0.1:7070
  --etcd=etcd://127.0.0.1/v2/keys/mesos
)

# Start watching etcd so that we can check our expectations.
#${ETCDCTL} watch /mesos >/dev/null 2>&1 &
${ETCDCTL} watch /mesos &

ETCDCTL_WATCH_PID=${!}

atexit "kill ${ETCDCTL_WATCH_PID}"

# First Mesos master, explicit quorum size. Should become leader in
# election.
${MESOS_BUILD_DIR}/src/mesos-master "${common_flags[@]}" \
  --quorum=2 \
  --log_dir="${WORK_DIR}/master1/logs" \
  --work_dir="${WORK_DIR}/master1" &

MASTER1_PID=${!}

atexit "kill ${MASTER1_PID}"

# Wait for the watch to terminate since the first master should become
# elected on its own.
# TODO(benh): This will BLOCK forever if we have a bug!
wait ${ETCDCTL_WATCH_PID}

if [ "${?}" -ne 0 ]; then
  echo "Failed to wait for the first master to become elected"
  exit -1
fi

check_elected() {
    local port
    case $1 in
        1)
            port=5050
            ;;
        2)
            port=6060
            ;;
        3)
            port=7070
    esac
    for _ in $(seq 1 5); do
        sleep 1
        # Check that the master has become elected.
        if curl -s http://127.0.0.1:$port/metrics/snapshot | grep -q '"master\\/elected":1'; then
            return 0
            break
        fi
    done

    return 1
}

ensure_elected() {
    check_elected $1 || {
        echo "Expecting master$1 to be elected!"
        exit -1
    }
}

ensure_not_elected() {
    check_elected $1 && {
        echo "Not expecting master$1 to be elected!"
        exit -1
    }
}

ensure_elected 1

echo "---------------- FIRST MASTER ELECTED ----------------"

# Save the etcd key so that we can compare it when the masters change.
ETCD_MESOS_KEY=`${ETCDCTL} get /mesos`

# TODO: CHECK THE VALUE IN ETCD_MESOS_KEY.
echo ${ETCD_MESOS_KEY}

echo "---------------- FIRST MASTER ELECTED ----------------"

# Now start a second Mesos master but let it determine the quorum size
# implicitly based on --masters.
${MESOS_BUILD_DIR}/src/mesos-master "${common_flags[@]}" \
  --log_dir="${WORK_DIR}/master2/logs" \
  --work_dir="${WORK_DIR}/master2" \
  --port=6060 &

MASTER2_PID=${!}

atexit "kill ${MASTER2_PID}"

# And finally start a third Mesos master because without at least 2 *
# quorum + 1 masters we'll never be able to auto-initialize the log.
${MESOS_BUILD_DIR}/src/mesos-master "${common_flags[@]}" \
  --quorum=2 \
  --log_dir="${WORK_DIR}/master3/logs" \
  --work_dir="${WORK_DIR}/master3" \
  --port=7070 &

MASTER3_PID=${!}

atexit "kill ${MASTER3_PID}"

# And start a slave.
${MESOS_BUILD_DIR}/src/mesos-slave \
  --master=etcd://127.0.0.1/v2/keys/mesos \
  --resources="cpus:2;mem:10240" \
  --log_dir="${WORK_DIR}/slave/logs" \
  --work_dir="${WORK_DIR}/slave" \
  --launcher_dir="${MESOS_BUILD_DIR}/src/" \
  --port=5052 &

SLAVE_PID=${!}

atexit "kill ${SLAVE_PID}"

# Wait for the masters to perform log initialization and the slave to
# (recover and) register.
sleep 3

# Now run the test framework using etcd to find the master.
echo "Running the test framework"
${MESOS_BUILD_DIR}/src/test-framework --master=etcd://127.0.0.1/v2/keys/mesos

if [ "${?}" -ne 0 ]; then
  echo "Expecting the test framework to exit successfully!"
  exit -1
fi

# Now shutdown the first master so that we can check that the second
# master becomes the leader.
kill ${MASTER1_PID}
wait ${MASTER1_PID}

echo "--------------- FIRST MASTER IS DEAD ---------------"

# Now watch etcd to wait until the TTL expires and the first master is
# no longer considered elected (otherwise when the first master comes
# back online it will think it's elected again!).
# TODO(benh): This will BLOCK forever if we have a bug!
${ETCDCTL} watch /mesos

if [ "${?}" -ne 0 ]; then
  echo "Failed to wait for the first master's etcd key to expire"
  exit -1
fi

echo "--------------- FIRST MASTER'S ETCD KEY EXPIRED ---------------"

# Now watch etcd to wait until the second master gets elected.
# TODO(benh): This will BLOCK forever if we have a bug!
${ETCDCTL} watch /mesos

if [ "${?}" -ne 0 ]; then
  echo "Failed to wait for another master to become elected"
  exit -1
fi

echo "--------------- SECOND/THIRD MASTER ELECTED ---------------"

# Check that the second or third master has become elected.
check_elected 2 || ensure_elected 3

# Restart the first master and check that it's not elected.
${MESOS_BUILD_DIR}/src/mesos-master "${common_flags[@]}" \
  --quorum=2 \
  --log_dir="${WORK_DIR}/master1/logs" \
  --work_dir="${WORK_DIR}/master1" &

MASTER1_PID=${!}

atexit "kill ${MASTER1_PID}"

ensure_not_elected 1

# Now re-run the test framework using etcd to find the master.
${MESOS_BUILD_DIR}/src/test-framework --master=etcd://127.0.0.1/v2/keys/mesos

if [ "${?}" -ne 0 ]; then
  echo "Expecting the test framework to exit successfully!"
  exit -1
fi

# echo "Now kill the etcd server"

# kill ${ETCD_PID}
# wait ${ETCD_PID}

# ${ETCD} -data-dir=${WORK_DIR}/etcd &

# ETCD_PID=${!}

# atexit "kill ${ETCD_PID}"

# # And re-run the test framework using the restarted etcd.
# ${MESOS_BUILD_DIR}/src/test-framework --master=etcd://127.0.0.1/v2/keys/mesos

# if [ "${?}" -ne 0 ]; then
#   echo "Expecting the test framework to exit successfully!"
#   exit -1
# fi

# The atexit handlers will clean up the remaining masters/slaves/etcd.
