call configure_local_paths.cmd

cd ..
@mkdir build_msvc2015_64
cd build_msvc2015_64

cmake -D USE_PCH=TRUE -D BUILD_TESTS=TRUE -D OPENSSL_ROOT_DIR="%OPENSSL_ROOT_DIR%" -D CMAKE_PREFIX_PATH="%QT_PREFIX_PATH%"\msvc2015_64 -D BUILD_GUI=TRUE -D STATIC=FALSE -DBOOST_ROOT="%BOOST_ROOT%" -DBOOST_LIBRARYDIR="%BOOST_ROOT%\lib64-msvc-14.0" -G "Visual Studio 14 2015 Win64" -T host=x64 ".."