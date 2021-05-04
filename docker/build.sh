# builds linux docker

set -e

if [ $# -lt 2 ]
then
    echo "docker build script requires os and architecture to be provided as the first and second arguments."
    exit 1
fi

#os
os="$1"
arc="$2"

dockerfile_build="docker/${os}.build.Dockerfile"

cmake_args=""
if [ "${arc}" == "x86" ]
then
    dockerfile_env="docker/${os}.env.32.Dockerfile"
else
    cmake_args="-DX64=ON"
    dockerfile_env="docker/${os}.env.64.Dockerfile"
fi

# dockerignore
if [ ! -f .dockerignore ] || [ ! grep -q "^.git/$" .dockerignore ]
then
    cp .gitignore .dockerignore
    echo "" >> .dockerignore
    echo 'docker/*.Dockerfile' >> .dockerignore
    echo 'docker/build.sh' >> .dockerignore
    echo ".git/" >> .dockerignore
fi

# build docker image
image="opengml/${os}-${arc}"

docker build -t "${image}-env" -f "${dockerfile_env}" \
    .

docker build -t "${image}" -f "${dockerfile_build}" \
    --build-arg "base_image=${image}-env"                 \
    --build-arg "cmake_args=${cmake_args}"                \
    .

# extract artifacts
artifacts="out-${os}-${arc}"
[ -d "${artifacts}" ] && rm -r "${artifacts}"

set -x

# set container name
container="container-${os}-${arc}"

# remove existing container if it exists
docker container rm "${container}" > /dev/null 2>&1 || true

# create new container using the image we've created
docker create -ti --name "${container}" "${image}" bash

# copy the build artifacts to ${artifacts}
docker cp "${container}:/opengml/out" "${artifacts}"

# remove the container we just created
docker container rm "${container}"

echo "produced artifacts at ${artifacts}"