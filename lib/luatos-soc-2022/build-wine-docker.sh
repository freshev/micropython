#! /bin/sh

docker buildx build --platform linux/386 -t debian-wine32 .
