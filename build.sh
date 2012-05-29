#!/bin/bash
# Copied from the UNIXPKG Build Script
#

if [ -z "$WORKSPACE" ]
then
  echo Initializing workspace
  if [ ! -e `pwd`/edksetup.sh ]
  then
    cd ..
  fi
# This version is for the tools in the BaseTools project.
# this assumes svn pulls have the same root dir
#  export EDK_TOOLS_PATH=`pwd`/../BaseTools
# This version is for the tools source in edk2
  export EDK_TOOLS_PATH=`pwd`/BaseTools
  echo $EDK_TOOLS_PATH
  source edksetup.sh BaseTools
else
  echo Building from: $WORKSPACE
fi

#
# Pick a default tool type for a given OS
#
TARGET_TOOLS=MYTOOLS
PKG_TOOLS=GCC46
NETWORK_SUPPORT=
case `uname` in
  CYGWIN*) echo Cygwin not fully supported yet. ;;
  Darwin*)
      Major=$(uname -r | cut -f 1 -d '.')
      if [[ $Major == 9 ]]
      then
        echo ShEfiPkg requires Snow Leopard or later OS
        exit 1
      else
        TARGET_TOOLS=XCODE43
        PKG_TOOLS=XCLANG
      fi
      NETWORK_SUPPORT="-D NETWORK_SUPPORT"
      ;;
  Linux*) TARGET_TOOLS=GCC46 ;;

esac

BUILD_ROOT_ARCH=$WORKSPACE/Build/ShEfiPkg/DEBUG_"$PKG_TOOLS"/X64

if  [[ ! -f `which build` || ! -f `which GenFv` ]];
then
  # build the tools if they don't yet exist. Bin scheme
  echo Building tools as they are not in the path
  make -C $WORKSPACE/BaseTools
elif [[ ( -f `which build` ||  -f `which GenFv` )  && ! -d  $EDK_TOOLS_PATH/Source/C/bin ]];
then
  # build the tools if they don't yet exist. BinWrapper scheme
  echo Building tools no $EDK_TOOLS_PATH/Source/C/bin directory
  make -C $WORKSPACE/BaseTools
else
  echo using prebuilt tools
fi


for arg in "$@"
do
  if [[ $arg == run ]]; then
    case `uname` in
      Darwin*)
        #
        # On Darwin we can't use dlopen, so we have to load the real PE/COFF images.
        # This .gdbinit script sets a breakpoint that loads symbols for the PE/COFFEE
        # images that get loaded in SecMain
        #
        cp $WORKSPACE/ShEfiPkg/.gdbinit $WORKSPACE/Build/ShEfiPkg/DEBUG_"$PKG_TOOLS"/X64
        ;;
    esac

    /usr/bin/gdb $BUILD_ROOT_ARCH/SecMain -q -cd=$BUILD_ROOT_ARCH
    exit
  fi

  if [[ $arg == cleanall ]]; then
    make -C $WORKSPACE/BaseTools clean
    build -p $WORKSPACE/ShEfiPkg/ShEfiPkg.dsc -a X64 -t $TARGET_TOOLS -D SEC_ONLY -n 3 clean
    build -p $WORKSPACE/ShEfiPkg/ShEfiPkg.dsc -a X64 -t $PKG_TOOLS -n 3 clean
    exit $?
  fi

  if [[ $arg == clean ]]; then
    build -p $WORKSPACE/ShEfiPkg/ShEfiPkg.dsc -a X64 -t $TARGET_TOOLS -D SEC_ONLY -n 3 clean
    build -p $WORKSPACE/ShEfiPkg/ShEfiPkg.dsc -a X64 -t $PKG_TOOLS -n 3 clean
    exit $?
  fi

  if [[ $arg == shell ]]; then
    build -p $WORKSPACE/GccShellPkg/GccShellPkg.dsc -a X64 -t $PKG_TOOLS -n 3 $2 $3 $4 $5 $6 $7 $8
    exit $?
  fi
done


#
# Build the edk2 ShEfiPkg
#
echo $PATH
echo `which build`
build -p $WORKSPACE/ShEfiPkg/ShEfiPkg.dsc      -a X64 -t $TARGET_TOOLS -D SEC_ONLY -n 3 $1 $2 $3 $4 $5 $6 $7 $8  modules

#
#We don't use the EDK UNIX Emulator
#
#cp $WORKSPACE/Build/ShEfiPkg/DEBUG_"$TARGET_TOOLS"/X64/SecMain $WORKSPACE/Build/ShEfiPkg/DEBUG_"$PKG_TOOLS"/X64

exit $?

