#!/bin/bash

set -x
set -e

umask 666

if [ -d build ]
then
    rm -rf build/
fi

scons --architecture=x86 .

echo "" >> /root/.bashrc
echo "alias ogm=$(pwd)/build/ogm" >> /root/.bashrc
echo "export PS1=\"\e[1;92mdocker\[\e[0m\]:\e[0;94m\w\[\e[0m\]\$ \"" >> /root/.bashrc

set +e

apt-get update
apt-get install -y gdb gedit

exit 0