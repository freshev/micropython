CSDTK 4 Instructions for Use
================

CSDTK 4 is a compilation environment based on pure Windows applications and is no longer based on cygwin. So it will be better for different Windows versions
compatibility and can be easily integrated into various other development environments.

CSDTK 4 provides a compressed package that can be decompressed and used anywhere (hereinafter, C:\CSDTK4 is used as an example).

CSDTK 4 only contains tools required for compilation, and does not include version management software such as `svn, git`.

The path to CSDTK 4 and the project should not contain special characters such as spaces, Chinese characters, etc.

The previous code may not compile under CSDTK 4, and the changes in the new code need to be merged, as long as `compilerules.mk` and
`usrgen, resgen` related changes.

Using CSDTK 4 under cmd.exe
---------------------

Example of compiling a BAT file:

```
call C:\CSDTK4\CSDTKvars.bat

set SOFT_WORKDIR=D:/projects/project/soft
set PATH=D:\projects\project\soft\env\utils;D:\projects\project\soft\env\win32;%PATH%
make -r -j4 CT_TARGET=target ......
```

`C:\CSDTK4\CSDTKvars.bat` will set the environment variables required for compilation, and then call `make`. Due to `work, ctmake`
Requires bash environment and cannot be used in cmd.exe environment.

`SOFT_WORKDIR` must use `/`, not `\`.

Using CSDTK 4 under git-bash
----------------------

Add: under `$HOME/.bashrc`:

```
export PROJ_ROOT=$(cygpath -au d:/projects) # or any other path
source $(cygpath -au c:/CSDTK4/CSDTKvars.sh)
```

Use the same as before:

```
$ work <project>
$ . env/launch.sh
$ ctmake ......
```

Since some applications under `C:\CSDTK4\make` will conflict with git-bash, they will not be added to PATH.

Use `ctmake` to compile, not `make`.

Except for the git command itself, CSDTK 4 does not depend on other applications in git-bash, so upgrading git-bash will not affect compilation.
It is recommended to install the latest version of git-bash.

Using CSDTK 4 under msys2
----------------------

The same method as under git-bash.

Using CSDTK 4 under cygwin
----------------------

Add: under `$HOME/.bashrc`:

```
export PROJ_ROOT=$(cygpath -au d:/projects) # or any other path
source $(cygpath -au c:/CSDTK4/CSDTKvars.sh)
```

Use the same as before:

```
$ work <project>
$ . env/launch.sh
$ ctmake ......
```

Since some applications under `C:\CSDTK4\make` conflict with cygwin, they will not be added to PATH.

Use `ctmake` to compile, not `make`. The `make` that comes with cygwin cannot be compiled normally.

Except for git and svn commands, CSDTK 4 does not depend on other applications in cygwin, so upgrading cygwin will not affect compilation.
And it can work under both cygwin and cygwin64. It is recommended to update cygwin regularly, and in 64-bit Windows
Use cygwin64 on the operating system.

Environment variable description
----------

`CSDTKVER`: In order to be compatible with different versions of CSDTK, it should be set to `4` under CSDTK 4.

`CSDTK4INSTALLDIR`: The path of CSDTK 4, such as `C:\CSDTK4`.

