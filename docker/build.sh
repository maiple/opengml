# builds linux docker

set -e

if [ $# -lt 2 ]
then
    echo "docker build script requires architecture and os to be provided as the first and second arguments."
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
if [ ! -f .dockerignore ]
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

container="container-${os}-${arc}"
docker container rm "${container}" || true
docker create -ti --name "${container}" "${image}" bash
docker cp "${container}:/opengml/out" "${artifacts}"
docker container rm "${container}"