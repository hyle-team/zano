call configure_local_paths.cmd

cd ..
@mkdir build_msvc2015_32
cd build_msvc2015_32

cmake -D CMAKE_PREFIX_PATH="%QT_PREFIX_PATH%"\msvc2015 -D BUILD_GUI=TRUE  -D STATIC=FALSE -DBOOST_ROOT="%BOOST_ROOT%" -DBOOST_LIBRARYDIR="%BOOST_ROOT%\lib32-msvc-14.0" -G "Visual Studio 14 2015" ".."
