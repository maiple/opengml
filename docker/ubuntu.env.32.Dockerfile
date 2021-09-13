FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive
ENV ARCHITECTURE=x86

RUN dpkg --add-architecture i386 && apt-get update

RUN apt-get install -y \
    build-essential

RUN apt-get install -y \
    gcc-8 g++-8 \
    gcc-8-multilib g++-8-multilib

# switch to gcc-8
RUN ln -f /usr/bin/gcc-8 /usr/bin/gcc
RUN ln -f /usr/bin/g++-8 /usr/bin/g++

# check that gcc compiles in 32 bit mode correctly
RUN echo "int main(int a, char** b) { return 1; }" >> /tmp/a.c
RUN cd /tmp && gcc -m32 a.c

# for convenience
RUN apt-get install -y git nano man

RUN apt-get install -y --no-remove \
    libassimp-dev:i386

# (install fcl later, as it has no i386 package)

RUN apt-get install -y --no-remove \
    libglew-dev:i386 \
    libglm-dev:i386 \
    libsdl2-dev:i386 \
    libsdl2-ttf-dev:i386 \
    libsdl2-mixer-dev:i386 \
    libcurl4-openssl-dev:i386

RUN apt-get install -y --no-remove \
     libccd-dev:i386 \
     libeigen3-dev:i386

RUN apt-get install -y --no-remove \
     cmake

#  custom install fcl 0.6, as no 32-bit binary is available. --------------------------
RUN git clone -b 0.6.1 --depth 1 https://github.com/flexible-collision-library/fcl
RUN mkdir /fcl/build

WORKDIR /fcl/build

RUN cmake .. -DCMAKE_CXX_FLAGS=-m32 "-DCMAKE_PREFIX_PATH=/usr/include;/usr/lib;/usr/lib/i386-linux-gnu"
RUN make -j 3 && make install

ENV OGM_DEB_REQUIREMENTS="libcfl0.5"

# done (fcl)----------------------------------------------------------------------------

RUN apt-get install -y --no-remove \
    libboost-filesystem-dev:i386 \
    libgtk-3-dev:i386

ENV CPATH=/usr/lib/i386-linux-gnu/;/usr/local/lib/;/usr/local/include/;/usr/include;/usr/include/eigen3/;/usr/include/i386-linux-gnu/

RUN umask 666

WORKDIR /

RUN apt-get install -y python3 python3-pip
RUN python3 -m pip install scons

RUN apt-get install -y wget