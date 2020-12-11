# builds linux docker

if [ $# != 2 ]
then
    echo "docker build script requires architecture and os to be provided as the first and second arguments."
    exit 1
fi

#os
os="$1"
arc="$2"

dockerfile="docker/${os}.Dockerfile"

if [ ! -f "$dockerfile" ]
then
    echo "dockerfile ${dockerfile} not found."
    exit 2
fi

base_image="ubuntu"
cmake_args=""
if [ "${arc}" == "x86" ]
then
    base_image="i386/ubuntu"
else
    cmake_args="-DX64=ON"
fi

# build docker image
image="opengml/${os}-${arc}"
docker build -t "${image}" -f "${dockerfile}" \
    --build-arg "base_image=${base_image}"                \
    --build-arg "cmake_args=${cmake_args}"                \
    .

# extract artifacts
artifacts="artifacts-${os}-arc$"
mkdir "${artifacts}"

container="container-${os}-${arc}"
docker create -ti --name "${container}" "${image}" bash
docker cp "${container}:/opengml/out" "${artifacts}"