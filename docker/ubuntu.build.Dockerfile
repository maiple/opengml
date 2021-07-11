ARG base_image
FROM ${base_image}

COPY . /opengml

WORKDIR /opengml

# required for running appimages
ENV APPIMAGE_EXTRACT_AND_RUN=1

# download linuxdeploy tool for creating appimages
# (the SConstruct file should do this on its own, but it seems to be having trouble for some reason.)
RUN wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-i386.AppImage && chmod u+x linuxdeploy-i386.AppImage

# try link test first to detect build environment errors early, before build proper.
RUN scons build-dir="out" linktest=1 architecture=${ARCHITECTURE}
RUN ls out
RUN out/ogm-linktest
# clean up
RUN scons build-dir="out" linktest=1 -c architecture=${ARCHITECTURE}

# ogm build proper with deb package and appimage
RUN scons build-dir="out" release=0 allow-gpl=0 appimage=1 deb=1 -j 4 architecture=${ARCHITECTURE}

# run the tests
RUN ./out/ogm-test