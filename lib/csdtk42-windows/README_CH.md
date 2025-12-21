CSDTK 4 使用说明
================

CSDTK 4是基于纯Windows应用的编译环境，不再基于cygwin。因此对不同的Windows版本会有更好
的兼容性，而且可以很方便地集成到其他各种开发环境中。

CSDTK 4提供的是一个压缩包，可以解压到任何地方使用（以下以C:\CSDTK4为例）。

CSDTK 4只包含编译需要的工具，不包含`svn, git`等版本管理软件。

CSDTK 4以及工程所在的路径不要包含空格，中文字符等特殊字符。

以前的代码可能不能在CSDTK 4下编译，需要合并新代码中的改动，只要是`compilerules.mk`以及
`usrgen, resgen`相关的改动。

在cmd.exe下使用CSDTK 4
---------------------

编译BAT文件的示例：

```
call C:\CSDTK4\CSDTKvars.bat

set SOFT_WORKDIR=D:/projects/project/soft
set PATH=D:\projects\project\soft\env\utils;D:\projects\project\soft\env\win32;%PATH%
make -r -j4 CT_TARGET=target ......
```

`C:\CSDTK4\CSDTKvars.bat`会设置编译所需的环境变量，之后调用`make`即可。由于`work, ctmake`
需要bash环境，在cmd.exe的环境中无法使用。

`SOFT_WORKDIR`必须使用`/`，不能用`\`。

在git-bash下使用CSDTK 4
----------------------

在`$HOME/.bashrc`下加上：

```
export PROJ_ROOT=$(cygpath -au d:/projects) # 或者其他任意路径
source $(cygpath -au c:/CSDTK4/CSDTKvars.sh)
```

使用时和之前一样：

```
$ work <project>
$ . env/launch.sh
$ ctmake ......
```

由于`C:\CSDTK4\make`下的一些应用和git-bash会有冲突，因此不会加到PATH里去。

要使用`ctmake`来编译，不要使用`make`。

除了git命令本身，CSDTK 4不依赖git-bash中的其他应用，因此升级git-bash不会影响编译。
建议安装最新版的git-bash。

在msys2下使用CSDTK 4
----------------------

和在git-bash下的使用方法一样。

在cygwin下使用CSDTK 4
----------------------

在`$HOME/.bashrc`下加上：

```
export PROJ_ROOT=$(cygpath -au d:/projects) # 或者其他任意路径
source $(cygpath -au c:/CSDTK4/CSDTKvars.sh)
```

使用时和之前一样：

```
$ work <project>
$ . env/launch.sh
$ ctmake ......
```

由于`C:\CSDTK4\make`下的一些应用和cygwin会有冲突，因此不会加到PATH里去。

要使用`ctmake`来编译，不要使用`make`。cygwin自带的`make`不能正常编译。

除了git，svn命令，CSDTK 4不依赖cygwin中的其他应用，因此升级cygwin不会影响编译。
而且在cygwin和cygwin64下都可以工作。建议定期更新cygwin，而且在64位的Windows
操作系统上使用cygwin64。

环境变量说明
----------

`CSDTKVER`：为了兼容不同版本的CSDTK，在CSDTK 4下应该设置为`4`。

`CSDTK4INSTALLDIR`：CSDTK 4的路径，例如`C:\CSDTK4`。

