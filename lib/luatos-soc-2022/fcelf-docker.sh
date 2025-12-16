#! /bin/bash

WINE_IMAGE=debian-wine32

docker images | grep $WINE_IMAGE > /dev/null
if [ $? -ne 0 ]; then
    echo docker image "$WINE_IMAGE" not found
    exit 1
fi

build="
cd /mnt
wine ./PLAT/tools/fcelf.exe $@
"

# docker buildx build --platform linux/386 -t debian-wine32 .

# docker run --privileged --platform linux/386 --rm -it -v "$PWD:/mnt" debian:stable-slim /bin/bash -c "$build"
docker run --privileged --platform linux/386 --rm -it -v "$PWD:/mnt" $WINE_IMAGE /bin/bash -c "$build"
