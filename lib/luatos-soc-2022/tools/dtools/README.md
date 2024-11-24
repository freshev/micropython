# Differential package generation tool

This folder is the tool needed to generate a differential package, divided into 2 parts:

1. Windows native tools
2. Docker image that can run native tools

Supported differential functions:

1. Binpkg difference produced by CSDK, function name csdk
2. Difference and function name of Hezhou series AT firmware at
3. Difference of QAT series firmware, function name qat
4. Differences between soc files of LuatOS firmware, function name soc
5. Binpkg difference of original SDK, function name org
6. CSDK full/full package upgrade package, full function name

Demo address: https://ecfota-ec618.k8s.air32.cn/

## Usage of native tools

### Run through script

If it is the soc file difference of LuatOS, you need to install the dependencies first

```
pip3 install -i https://pypi.tuna.tsinghua.edu.cn/simple py7zr
```

Differential command format and examples

```
python3 main.py mode Old version firmware path New version firmware path Path to output differential package

python3 main.py csdk old.binpkg new.binpkg diff.bin
```

in
* Mode, optional values   are `csdk` `at` `qat` `org` `soc` `full`
* Non-LuatOS firmware transfers the binpkg path, LuatOS firmware transfers the soc file path

|Differential mode|File suffix|Description|
|-------|-------|----|
|org |binpkg |Difference package of the core-shifting original sdk|
|at |binpkg |Hezhou AT firmware differential package, including LSAT, AUAT, LPAT firmware|
|qat |binpkg |Hezhou QAT firmware differential package|
|csdk |binpkg |Difference package used by this CSDK|
|soc |soc |Difference package of LuatOS firmware|
|full |binpkg |CSDK full/full package upgrade package|

## Docker image description

Since most servers are Linux systems, and the fota tool does not have a Linux system, docker images are provided here.

Pull the latest image (optional)
```
docker pull wendal/ecfota-ec618
```

Or build a local image (optional)
```
docker build -t wendal/ecfota-ec618 .
```

Run the image, expose port 9000, and create an http api
```
docker run -it --rm -p 9000:9000 wendal/ecfota-ec618
```

This directory also provides docker-compose and k8s deployment files

### http api calling rules

```
URL         /api/diff/<mode>
METHOD      POST
Use file upload method

Available parameters:
old old version of the file
new new version of the file
oldurl is the URL of the old version. The old version of the file will be obtained through this URL.
newurl is the URL of the new version, through which the new version of the file will be obtained.
mode replaces the mode parameter in the URL, the same as the command line mode
<mode> URL parameter is the mode, which is the same as the command line mode.
```

Note that old and oldurl need to provide one of them, new and newurl also need to provide one of them.

Note: The web service of the docker image is also provided by `main.py`, which is not required. Other programming languages     can also call the difference tool directly within the image.

## Other information

* [Tutorial on installing wine9](install_wine.md)

For matters not covered, please consult FAE or sales

## Update log

# 1.0.7

1. Support the full upgrade package of csdk

# 1.0.5

1. Prometheus data is integrated into the web service and does not require a separate port.

# 1.0.4

1. Support multi-threaded requests
2. Support heartbeat detection
3. Support prometheus

### 1.0.3

1. Add Tencent Cloud COS integration to support differential packet reuse
2. Add oldurl and newurl parameters

### 1.0.1

1. Add mode parameter to replace the mode parameter in URL
