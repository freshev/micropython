# This file should be sourced in bash

function remove_from_var
{
    local VAR=$1
    local VAL="$2"
    if test -n "$VAL"; then
        export $VAR="$(echo "${!VAR}" | tr ':' '\n' | grep -v "$VAL" | paste -d: -s)"
    fi
}

function prepend_var
{
    local VAR=$1
    local VAL="$2"
    if test -n "$VAL"; then
        remove_from_var $VAR "$VAL"
        if test -n "${!VAR}"; then
            export $VAR="$VAL:${!VAR}"
        else
            export $VAR="$VAL"
        fi
    fi
}

export CSDTKVER=4
export CSDTKVERSION=4.1
export CSDTK4INSTALLDIR=$(cygpath -aw $(dirname $BASH_SOURCE[0]))
prepend_var PATH "$(cygpath -au $CSDTK4INSTALLDIR)/nanopb-0.3.9"
prepend_var PATH "$(cygpath -au $CSDTK4INSTALLDIR)/rv32-elf-5.4.1/bin"
prepend_var PATH "$(cygpath -au $CSDTK4INSTALLDIR)/mips-elf-4.4.2/bin"
prepend_var PATH "$(cygpath -au $CSDTK4INSTALLDIR)/mips-rda-elf-7.1.0/bin"

if test "x$PROCESSOR_ARCHITECTURE" = "xAMD64"; then
    export CSDTK4EXTRAPATH=$(cygpath -au $CSDTK4INSTALLDIR)/make64:$(cygpath -au $CSDTK4INSTALLDIR)/perl/bin:$(cygpath -au $CSDTK4INSTALLDIR)/mingw32/bin:$(cygpath -au $CSDTK4INSTALLDIR)/python27
else
    export CSDTK4EXTRAPATH=$(cygpath -au $CSDTK4INSTALLDIR)/make:$(cygpath -au $CSDTK4INSTALLDIR)/perl/bin:$(cygpath -au $CSDTK4INSTALLDIR)/mingw32/bin:$(cygpath -au $CSDTK4INSTALLDIR)/python27
fi

function work
{
    if test -z "$PROJ_ROOT"; then
        echo "PROJ_ROOT not defined"
    elif test -z "$1"; then
        echo "no project specified"
    else
        export PROJ=$1
        export PROJ_WORKDIR=$PROJ_ROOT/$PROJ
        export SOFT_WORKDIR=$PROJ_WORKDIR/soft
        if test -f $SOFT_WORKDIR/env/set_env.sh; then
            source $SOFT_WORKDIR/env/set_env.sh
        fi
        if cd $SOFT_WORKDIR; then
            echo "Project Switched to $PROJ"
        else
            echo "Failed to switch project to $PROJ"
        fi
    fi
}

_work_complete()
{
    local cur=${COMP_WORDS[COMP_CWORD]}
    local _dir
    local _projects
    COMPREPLY=()
    for _dir in `/bin/ls -d \$PROJ_ROOT/*/soft 2> /dev/null`; do
        _projects="$_projects $(basename $(readlink -f $_dir/..))"
    done
    COMPREPLY=( $(compgen -W "$_projects" -- $cur) )
    return 0
}
complete -o nospace -F _work_complete work

unset -f remove_from_var
unset -f prepend_var

