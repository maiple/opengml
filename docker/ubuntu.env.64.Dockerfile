FROM ubuntu

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get  update

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

RUN apt-get install -y build-essential

RUN apt-get install -y \
    gcc-8 \
    g++-8 \
    curl \
    make

RUN apt-get install -y --no-remove \
    libboost-filesystem-dev \
    libgtk-3-dev

RUN apt-get install -y python3 python3-pip

RUN python3 -m pip install scons

RUN umask 666

RUN apt-get install -y git nano