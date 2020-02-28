set -x #echo on
cd ..
rm -r _builds_android
cmake -S. -B_builds_android  -DCMAKE_SYSTEM_NAME=Android -DCMAKE_SYSTEM_VERSION=21 -DCMAKE_ANDROID_ARCH_ABI=armeabi-v7a -DCMAKE_ANDROID_NDK=/Users/roky/Library/Android/sdk/ndk/18.1.5063045 -DCMAKE_ANDROID_STL_TYPE=c++_static -DCMAKE_install_android_PREFIX=`pwd`/_install_android 
if [ $? -ne 0 ]; then
    echo "Failed to perform command"
    exit 1
fi

cmake --build _builds_android --config Debug --target install -- -j 4
if [ $? -ne 0 ]; then
    echo "Failed to perform command"
    exit 1
fi

rm -r _builds_android
if [ $? -ne 0 ]; then
    echo "Failed to perform command"
    exit 1
fi

cmake -S. -B_builds_android  -DCMAKE_SYSTEM_NAME=Android -DCMAKE_SYSTEM_VERSION=21 -DCMAKE_ANDROID_ARCH_ABI=x86 -DCMAKE_ANDROID_NDK=/Users/roky/Library/Android/sdk/ndk/18.1.5063045 -DCMAKE_ANDROID_STL_TYPE=c++_static -DCMAKE_install_android_PREFIX=`pwd`/_install_android 
if [ $? -ne 0 ]; then
    echo "Failed to perform command"
    exit 1
fi

cmake --build _builds_android --config Debug --target install -- -j 4
if [ $? -ne 0 ]; then
    echo "Failed to perform command"
    exit 1
fi

rm -r _builds_android
if [ $? -ne 0 ]; then
    echo "Failed to perform command"
    exit 1
fi


cmake -S. -B_builds_android  -DCMAKE_SYSTEM_NAME=Android -DCMAKE_SYSTEM_VERSION=21 -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a -DCMAKE_ANDROID_NDK=/Users/roky/Library/Android/sdk/ndk/18.1.5063045 -DCMAKE_ANDROID_STL_TYPE=c++_static -DCMAKE_install_android_PREFIX=`pwd`/_install_android 
if [ $? -ne 0 ]; then
    echo "Failed to perform command"
    exit 1
fi

cmake --build _builds_android --config Debug --target install -- -j 4
if [ $? -ne 0 ]; then
    echo "Failed to perform command"
    exit 1
fi

rm -r _builds_android
if [ $? -ne 0 ]; then
    echo "Failed to perform command"
    exit 1
fi


cmake -S. -B_builds_android  -DCMAKE_SYSTEM_NAME=Android -DCMAKE_SYSTEM_VERSION=21 -DCMAKE_ANDROID_ARCH_ABI=x86_64 -DCMAKE_ANDROID_NDK=/Users/roky/Library/Android/sdk/ndk/18.1.5063045 -DCMAKE_ANDROID_STL_TYPE=c++_static -DCMAKE_install_android_PREFIX=`pwd`/_install_android 
if [ $? -ne 0 ]; then
    echo "Failed to perform command"
    exit 1
fi

cmake --build _builds_android --config Debug --target install -- -j 4
if [ $? -ne 0 ]; then
    echo "Failed to perform command"
    exit 1
fi

rm -r _builds_android
if [ $? -ne 0 ]; then
    echo "Failed to perform command"
    exit 1
fi


