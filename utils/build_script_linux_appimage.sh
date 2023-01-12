#!/bin/bash -x

# Environment prerequisites:
# 1) QT_PREFIX_PATH should be set to Qt libs folder
# 2) BOOST_ROOT should be set to the root of Boost
# 3) OPENSSL_ROOT_DIR should be set to the root of OpenSSL
#
# for example, place these lines to the end of your ~/.bashrc :
#
# export BOOST_ROOT=/home/user/boost_1_66_0
# export QT_PREFIX_PATH=/home/user/Qt5.10.1/5.10.1/gcc_64
# export OPENSSL_ROOT_DIR=/home/user/openssl
# export LINUX_DEPLOY_QT=/home/user/QtDeployment.appimage


ARCHIVE_NAME_PREFIX=zano-linux-x64-

: "${BOOST_ROOT:?BOOST_ROOT should be set to the root of Boost, ex.: /home/user/boost_1_66_0}"
: "${QT_PREFIX_PATH:?QT_PREFIX_PATH should be set to Qt libs folder, ex.: /home/user/Qt5.10.1/5.10.1/gcc_64}"
: "${OPENSSL_ROOT_DIR:?OPENSSL_ROOT_DIR should be set to OpenSSL root folder, ex.: /home/user/openssl}"

if [ -n "$build_prefix" ]; then
  ARCHIVE_NAME_PREFIX=${ARCHIVE_NAME_PREFIX}${build_prefix}-
  build_prefix_label="$build_prefix "
fi

if [ "$testnet" == true ]; then
  testnet_def="-D TESTNET=TRUE"
  testnet_label="testnet "
  ARCHIVE_NAME_PREFIX=${ARCHIVE_NAME_PREFIX}testnet-
fi

if [ "$testnet" == true ] || [ -n "$qt_dev_tools" ]; then
  copy_qt_dev_tools=true
  copy_qt_dev_tools_label="devtools "
  ARCHIVE_NAME_PREFIX=${ARCHIVE_NAME_PREFIX}devtools-
fi


prj_root=$(pwd)

echo "---------------- BUILDING PROJECT ----------------"
echo "--------------------------------------------------"

echo "Building...." 

rm -rf build; mkdir -p build/release; cd build/release; 
cmake $testnet_def -D STATIC=true -D ARCH=x86-64 -D BUILD_GUI=TRUE -D OPENSSL_ROOT_DIR="$OPENSSL_ROOT_DIR" -D CMAKE_PREFIX_PATH="$QT_PREFIX_PATH" -D CMAKE_BUILD_TYPE=Release ../..
if [ $? -ne 0 ]; then
    echo "Failed to run cmake"
    exit 1
fi

make -j2 daemon simplewallet connectivity_tool
if [ $? -ne 0 ]; then
    echo "Failed to make!"
    exit 1
fi

make -j1 Zano
if [ $? -ne 0 ]; then
    echo "Failed to make!"
    exit 1
fi


read version_str <<< $(./src/zanod --version | awk '/^Zano/ { print $2 }')
version_str=${version_str}
echo $version_str

rm -rf Zano;
mkdir -p Zano/usr/bin;
mkdir -p Zano/usr/lib;
mkdir -p Zano/usr/share/applications;
mkdir -p Zano/usr/share/icons/hicolor/scalable;

rsync -a ../../src/gui/qt-daemon/layout/html ./Zano/usr/bin --exclude less --exclude package.json --exclude gulpfile.js

cp -Rv src/zanod src/Zano src/simplewallet  src/connectivity_tool ./Zano/usr/bin
cp -Rv ../../ultils/Zano.desktop ./Zano/usr/share/application/Zano.desktop
cp -Rv ../../resources/app_icon.svg ./Zano/usr/share/icons/hicolor/scalable/Zano.svg

echo "Exec=$prj_root/build/release/Zano/usr/bin/Zano --deeplink-params=%u" >> ./Zano/usr/share/application/Zano.desktop

$LINUX_DEPLOY_QT ./Zano/usr/share/applications/Zano.desktop  -appimage -qmake=$QT_PREFIX_PATH/bin/qmake

rm -f ./Zano-x86_64.AppImage

package_filename=${ARCHIVE_NAME_PREFIX}${version_str}.AppImage

mv ./Zano-x86_64.AppImage ./$package_filename

echo "Build success"

if [ -z "$upload_build" ]; then
    exit 0
fi

echo "Uploading..."

scp $package_filename zano_build_server:/var/www/html/builds
if [ $? -ne 0 ]; then
    echo "Failed to upload to remote server"
    exit $?
fi

read checksum <<< $(sha256sum $package_filename | awk '/^/ { print $1 }' )

mail_msg="New ${build_prefix_label}${testnet_label}${copy_qt_dev_tools_label}build for linux-x64:<br>
https://build.zano.org/builds/$package_filename<br>
sha256: $checksum"

echo "$mail_msg"

echo "$mail_msg" | mail -s "Zano linux-x64 ${build_prefix_label}${testnet_label}${copy_qt_dev_tools_label}build $version_str" ${emails}

exit 0
