# Tutorial on installing Wine 9

This tutorial is used to install the wine version that can run the differential tool. The currently verified version is wine9, and the operating systems are ubuntu 22.04 and centos 7.

## Installation tutorial under CentOS 7

Create a directory and download the installation files

```bash
mkdir /opt/wine
cd /opt/wine

# Download source code package
wget https://dl.winehq.org/wine/source/9.0/wine-9.0.tar.xz
# or
wget http://43.129.160.151/wine/source/9.0/wine-9.0.tar.xz

tar xf wine-9.0.tar.xz
```

Install dependency packages

```bash
yum install gcc g++ flex bison make freetype-devel freetype-devel.i686
yum install gcc-multilib g++-multilib
yum install libgcc.i686 glibc-devel.i686 libstdc++-devel.i686
```

Configure and compile

```bash
./configure --without-x --without-freetype --enable-win64
make -j16
```

Install to /usr/local

```bash
make install
```

Verify execution

```bash
cd luatos-soc-2022/tools/dtools
# Prepare 2 binpkg files old.binpkg new.binpkg
# Then execute
wine ./FotaToolkit.exe -d config/ec618.json BINPKG diff.par old.binpkg new.binpkg
```

## Installation tutorial under Ubuntu 22.04

Unlike centos, ubuntu can be installed directly from the mirror source, which is faster and does not require compiling source code.

```bash
sed -i 's/security.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list
sed -i 's/archive.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list
apt-get update -y && apt install -y wget python3-pip
wget -nc -O /usr/share/keyrings/winehq-archive.key https://dl.winehq.org/wine-builds/winehq.key
# COPY winehq-archive.key /usr/share/keyrings/winehq-archive.key
echo "deb [arch=amd64,i386 signed-by=/usr/share/keyrings/winehq-archive.key] https://mirrors.tuna.tsinghua.edu.cn/wine-builds/ubuntu/ jammy main" > /etc/apt/sources.list.d/winehq.list
dpkg --add-architecture i386 &&\
apt-get update &&\
apt-get install --install-recommends -y winehq-stable &&\
apt clean -y
```
