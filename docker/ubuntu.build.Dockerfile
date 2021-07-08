ARG base_image
FROM ${base_image}

COPY . /opengml

WORKDIR /opengml

# required for running appimages
ENV APPIMAGE_EXTRACT_AND_RUN=1

RUN apt-get install -y gdb

# try link test first to detect build environment errors early, before build proper.
RUN scons build-dir="out" linktest=1
RUN ls out
RUN out/ogm-linktest
# clean up
RUN scons build-dir="out" linktest=1 -c

# ogm build proper with deb package and appimage
RUN scons build-dir="out" release=0 allow-gpl=0 appimage=1 deb=1 -j 4

# run the tests
RUN ./out/ogm-test