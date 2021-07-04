ARG base_image
FROM ${base_image}

ARG artifacts="artifacts"
ARG scons_args=""
ARG make_args="-j4"

COPY . /opengml

WORKDIR /opengml

RUN mkdir out && mkdir out/lib && mkdir out/bin

# try link test first to detect build environment errors early
RUN scons build-dir="out" linktest=1
RUN out/ogm-linktest

# ogm build proper
RUN scons build-dir="out" release=1 -j 3

RUN ls out/
RUN cp out/gig.so . && ./out/ogm-test

#RUN apt-get install -y --no-remove patchelf

# copy dependencies to out/lib (so as to put them in the archive later)
RUN mkdir -p ./out/lib
RUN ldd ./out/ogm
RUN ldd ./out/ogm | xargs -d\\n -n1 bash docker/copy_lib.sh ./out/lib

# some libraries must be manually removed to prevent errors
RUN rm out/lib/librt.*
RUN rm out/lib/libpthread.*
RUN rm out/lib/libgcc_*
RUN rm out/lib/libc.*
RUN rm out/lib/libm.* || true
RUN rm out/lib/libdl.* || true

RUN rm out/lib/libGL.*

# patch DT_NEEDED on ogm so that it includes all its dependencies.
# (this is needed for the $ORIGIN/lib/ rpath to find all dependencies locally at runtime)
#RUN cd out/lib && ls *.so | xargs -d\\n -n1 -I '{}' patchelf --add-needed {} ../ogm
#RUN patchelf --set-rpath "\$ORIGIN/lib" out/ogm