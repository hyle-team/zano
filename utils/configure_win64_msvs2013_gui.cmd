call configure_local_paths.cmd

cd ..
@mkdir build_msvc2013_64
cd build_msvc2013_64

cmake -D CMAKE_PREFIX_PATH="%QT_PREFIX_PATH%"\msvc2013_64 -D BUILD_GUI=TRUE  -D STATIC=FALSE -DBOOST_ROOT="%BOOST_ROOT%" -DBOOST_LIBRARYDIR="%BOOST_ROOT%\lib64-msvc-12.0" -G "Visual Studio 12 2013 Win64" ".."