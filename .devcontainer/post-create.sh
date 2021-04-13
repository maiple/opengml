#!/bin/bash

set -x
set -e

umask 666

if [ -d build ]
then
    rm -rf build/
fi

if [ -f CMakeCache.txt ]
then
    mv CMakeCache.txt.bk.$HOSTNAME
fi

mkdir build/

cmake_args=""

cd build

cmake .. ${cmake_args} \
    -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} \
    -DNO_FCL=ON \
    -DLINK_ORIGIN=ON \
    -DRELEASE=OFF \
    -DCMAKE_CXX_FLAGS="-static-libgcc -static-libstdc++"

echo "" >> /root/.bashrc
echo "alias ogm=$(pwd)/build/ogm" >> /root/.bashrc
echo "export PS1=\"\e[1;92mdocker\[\e[0m\]:\e[0;94m\w\[\e[0m\]\$ \"" >> /root/.bashrc

set +e

apt-get update
apt-get install -y gdb gedit

exit 0