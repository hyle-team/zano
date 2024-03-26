call configure_local_paths_msvs2022.cmd

cd ..
@mkdir build_msvc2022_64_tn
cd build_msvc2022_64_tn

cmake -D TESTNET=TRUE -D USE_PCH=TRUE -D BUILD_TESTS=TRUE -D OPENSSL_ROOT_DIR="%OPENSSL_ROOT_DIR%" -D CMAKE_PREFIX_PATH="%QT_PREFIX_PATH%"\msvc2019_64 -D BUILD_GUI=TRUE -D STATIC=FALSE -DBOOST_ROOT="%BOOST_ROOT%" -DBOOST_LIBRARYDIR="%BOOST_ROOT%\lib64-msvc-14.3" -G "Visual Studio 17 2022" -A x64 -T host=x64 ".."
