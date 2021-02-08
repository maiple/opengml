ARG base_image
FROM ${base_image}

ARG artifacts="artifacts"
ARG cmake_args=""
ARG make_args="-j4"

COPY . /opengml

WORKDIR /opengml

RUN mkdir out && mkdir out/lib && mkdir out/bin

RUN cmake . ${cmake_args} \
    -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} \
    -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=./out \
    -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=./out \
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=./out \
    -DNO_FCL=ON \
    -DLINK_ORIGIN=ON \
    -DRELEASE=ON \
    -DCMAKE_CXX_FLAGS="-static-libgcc -static-libstdc++"

# run these at different steps for better caching purposes
RUN make ${make_args} ogmi
RUN make ${make_args} soloud
RUN make ${make_args} ogm-project
RUN make ${make_args} ogm-test
RUN make

RUN ls out/
RUN cp out/gig.so . && ./out/ogm-test

RUN apt-get install -y --no-remove python patchelf
RUN mkdir -p ./out/lib
RUN ldd ./out/ogm | xargs -d\\n -n1 bash docker/copy_lib.sh ./out/lib

# some libraries must be manually removed to prevent errors
RUN rm out/lib/librt.*
RUN rm out/lib/libpthread.*
RUN rm out/lib/libgcc_*
RUN rm out/lib/libc.*
RUN rm out/lib/libm.* || true
RUN rm out/lib/libdl.* || true

# patch DT_NEEDED on ogm so that it includes all its dependencies.
# (this is needed for the $ORIGIN/lib/ rpath to find all dependencies locally at runtime)
RUN cd out/lib && ls | xargs -d\\n -n1 -I '{}' patchelf --add-needed {} ../ogm
RUN patchelf --set-rpath "\$ORIGIN/lib" out/ogm