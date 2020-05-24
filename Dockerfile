FROM i386/ubuntu

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update

RUN apt-get install -y \
    libassimp-dev \
    libfcl-dev

RUN apt-get install -y \
    libglew-dev \
    libglm-dev \
    libsdl2-dev \
    libsdl2-ttf-dev \
    libsdl2-mixer-dev \
    libreadline-dev \
    libcurl4-openssl-dev

RUN apt-get install -y \
    gcc-8 \
    g++-8 \
    curl \
    make \
    cmake
    
RUN apt-get install -y gcc-8 g++-8
    
COPY . /opengml

RUN cd /opengml && cmake . -DCMAKE_CXX_COMPILER=g++-8 -DCMAKE_C_COMPILER=gcc-8 -DCMAKE_CXX_FLAGS="-static-libgcc -static-libstdc++" && make -j 2

RUN cd /opengml && ./ogm-test