FROM debian:stable-slim

RUN sed -i 's/deb.debian.org/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list.d/debian.sources

RUN cat /etc/apt/sources.list.d/debian.sources

ENV TZ=Asia/Shanghai \
    DEBIAN_FRONTEND=noninteractive

RUN apt update &&\
    apt install -y wine32 &&\
    apt clean -y

# 调用一次 wine,初始化wine环境,不然启动耗时会很长
RUN wine -h || true
