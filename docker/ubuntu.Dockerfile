ARG base_image=ubuntu
FROM ${base_image}

ARG artifacts="artifacts"
ARG cmake_args=""

ENV DEBIAN_FRONTEND=noninteractive

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

WORKDIR /opengml

RUN mkdir out && mkdir out/lib && mkdir out/bin

RUN cmake . ${cmake_args} \
    -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=./out \
    -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=./out \
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=./out \
    -DCMAKE_CXX_COMPILER=g++-8 -DCMAKE_C_COMPILER=gcc-8 -DCMAKE_CXX_FLAGS="-static-libgcc -static-libstdc++" && \
    make

RUN ls out/
RUN cp out/gig.so . && ./out/ogm-test

RUN ldd ./out/ogm | xargs bash docker/copy_lib.sh ./out