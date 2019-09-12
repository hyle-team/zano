#!/bin/bash -x

# Environment prerequisites:
# 1) QT_PREFIX_PATH should be set to Qt libs folder
# 2) BOOST_ROOT should be set to the root of Boost
#
# for example, place these lines to the end of your ~/.bashrc :
#
# export BOOST_ROOT=/home/user/boost_1_66_0
# export QT_PREFIX_PATH=/home/user/Qt5.10.1/5.10.1/gcc_64

ARCHIVE_NAME_PREFIX=zano-linux-x64-

: "${BOOST_ROOT:?BOOST_ROOT should be set to the root of Boost, ex.: /home/user/boost_1_66_0}"
: "${QT_PREFIX_PATH:?QT_PREFIX_PATH should be set to Qt libs folder, ex.: /home/user/Qt5.10.1/5.10.1/gcc_64}"

if [ -n "$build_prefix" ]; then
  ARCHIVE_NAME_PREFIX=${ARCHIVE_NAME_PREFIX}${build_prefix}-
  build_prefix_label="$build_prefix "
fi

if [ -n "$testnet" ]; then
  testnet_def="-D TESTNET=TRUE"
  testnet_label="testnet "
  ARCHIVE_NAME_PREFIX=${ARCHIVE_NAME_PREFIX}testnet-
fi


prj_root=$(pwd)

echo "---------------- BUILDING PROJECT ----------------"
echo "--------------------------------------------------"

echo "Building...." 

rm -rf build; mkdir -p build/release; cd build/release; 
cmake $testnet_def -D STATIC=true -D ARCH=x86-64 -D BUILD_GUI=TRUE -D CMAKE_PREFIX_PATH="$QT_PREFIX_PATH" -D CMAKE_BUILD_TYPE=Release ../..
if [ $? -ne 0 ]; then
    echo "Failed to run cmake"
    exit 1
fi

make -j1 daemon Zano;
if [ $? -ne 0 ]; then
    echo "Failed to make!"
    exit 1
fi

make -j1 simplewallet;
if [ $? -ne 0 ]; then
    echo "Failed to make!"
    exit 1
fi

make -j1 connectivity_tool;
if [ $? -ne 0 ]; then
    echo "Failed to make!"
    exit 1
fi



read version_str <<< $(./src/zanod --version | awk '/^Zano/ { print $2 }')
version_str=${version_str}
echo $version_str

rm -rf Zano;
mkdir -p Zano;

rsync -a ../../src/gui/qt-daemon/html ./Zano --exclude less --exclude package.json --exclude gulpfile.js
cp -Rv ../../utils/Zano.sh ./Zano
chmod 777 ./Zano/Zano.sh
mkdir ./Zano/lib
cp $QT_PREFIX_PATH/lib/libicudata.so.56 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libicui18n.so.56 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libicuuc.so.56 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5Core.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5DBus.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5Gui.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5Network.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5OpenGL.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5Positioning.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5PrintSupport.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5Qml.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5Quick.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5Sensors.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5Sql.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5Widgets.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5WebEngine.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5WebEngineCore.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5WebEngineWidgets.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5WebChannel.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5XcbQpa.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/lib/libQt5QuickWidgets.so.5 ./Zano/lib
cp $QT_PREFIX_PATH/libexec/QtWebEngineProcess ./Zano
cp $QT_PREFIX_PATH/resources/qtwebengine_resources.pak ./Zano
cp $QT_PREFIX_PATH/resources/qtwebengine_resources_100p.pak ./Zano
cp $QT_PREFIX_PATH/resources/qtwebengine_resources_200p.pak ./Zano
cp $QT_PREFIX_PATH/resources/icudtl.dat ./Zano


mkdir ./Zano/lib/platforms
cp $QT_PREFIX_PATH/plugins/platforms/libqxcb.so ./Zano/lib/platforms
mkdir ./Zano/xcbglintegrations
cp $QT_PREFIX_PATH/plugins/xcbglintegrations/libqxcb-glx-integration.so ./Zano/xcbglintegrations

cp -Rv src/zanod src/Zano src/simplewallet  src/connectivity_tool ./Zano

package_filename=${ARCHIVE_NAME_PREFIX}${version_str}.tar.bz2

rm -f ./$package_filename
tar -cjvf $package_filename Zano
if [ $? -ne 0 ]; then
    echo "Failed to pack"
    exit 1
fi

echo "Build success"


echo "Uploading..."

scp $package_filename zano_build_server:/var/www/html/builds
if [ $? -ne 0 ]; then
    echo "Failed to upload to remote server"
    exit $?
fi

read checksum <<< $(sha256sum $package_filename | awk '/^/ { print $1 }' )

mail_msg="New ${build_prefix_label}${testnet_label}build for linux-x64:<br>
http://build.zano.org:8081/builds/$package_filename<br>
sha256: $checksum"

echo "$mail_msg"

echo "$mail_msg" | mail -s "Zano linux-x64 ${build_prefix_label}${testnet_label}build $version_str" ${emails}

exit 0
