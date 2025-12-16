

export PROJECT_NAME=example
export LSPD_MODE=disable
#rem you can set your gcc path
# export GCC_PATH=/opt/gcc_mcu
export ROOT_PATH=`pwd`
if test "$1" != ""; then
	export PROJECT_NAME=$1
else
	echo "PROJECT not set"
fi


if test "$2" != "" ;then 
	export LSPD_MODE=$2
else
	if test "$1" = "luatos"; then
		export LSPD_MODE=enable
	else
		echo "LSPD_MODE not set"
	fi
fi


# check csdk rndis
export RNDIS_MARK="csdk_rndis"
if [[ -f "$FILE" ]]; then
# 	@echo This is CSDK with RNDIS
	export EC618_RNDIS=enable
	export LSPD_MODE=disable
fi

echo "=============================="
echo "AirM2M https://openluat.com"
echo "=============================="
echo "PROJECT   : $PROJECT_NAME " 
echo "LSPD_MODE : $LSPD_MODE  "
echo "RNDIS     : $EC618_RNDIS"
echo "=============================="

if test "$3" = "-v"; then
	xmake --root -v
else
	xmake --root -w
fi

echo "-------------------------"
echo "--------DONE-------------"
echo "-------------------------"


