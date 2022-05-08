call configure_local_paths_msvs2019.cmd

cd ..
@mkdir build_msvc2019_64_tn
cd build_msvc2019_64_tn

cmake -D TESTNET=TRUE -D USE_PCH=TRUE -D BUILD_TESTS=TRUE -D OPENSSL_ROOT_DIR="%OPENSSL_ROOT_DIR%" -D CMAKE_PREFIX_PATH="%QT_PREFIX_PATH%"\msvc2019_64 -D BUILD_GUI=TRUE -D STATIC=FALSE -DBOOST_ROOT="%BOOST_ROOT%" -DBOOST_LIBRARYDIR="%BOOST_ROOT%\lib64-msvc-14.2" -G "Visual Studio 16 2019" -A x64 -T host=x64 ".."
