SET QT32_PREFIX_PATH=C:\Qt\Qt5.9.1\5.9.1\msvc2015
SET INNOSETUP_PATH=C:\Program Files (x86)\Inno Setup 5\ISCC.exe
SET ETC32_BINARIES_PATH=C:\home\deploy\etc-binaries-32
SET BUILDS_PATH=C:\home\deploy\zano
SET ACHIVE_NAME_PREFIX=zano-win-x32-webengine-
SET SOURCES_PATH=C:\home\deploy\zano\src

SET LOCAL_BOOST_PATH_32=C:\local\boost_1_62_0
SET LOCAL_BOOST_LIB_PATH_32=C:\local\boost_1_62_0\lib32-msvc-14.0


@echo on

set BOOST_ROOT=%LOCAL_BOOST_PATH_32%
set BOOST_LIBRARYDIR=%LOCAL_BOOST_LIB_PATH_32%



@echo "---------------- PREPARING BINARIES ---------------------------"
@echo "---------------------------------------------------------------"



cd %SOURCES_PATH%
git pull
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "---------------- BUILDING APPLICATIONS ------------------------"
@echo "---------------------------------------------------------------"




rmdir build /s /q
mkdir build
cd build
cmake -D CMAKE_PREFIX_PATH="%QT32_PREFIX_PATH%" -D BUILD_GUI=TRUE -D STATIC=FALSE -G "Visual Studio 14" ..
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

setLocal 
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86_amd64

msbuild version.vcxproj  /p:Configuration=Release /t:Build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

msbuild src/daemon.vcxproj  /p:Configuration=Release /t:Build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

msbuild src/simplewallet.vcxproj  /p:Configuration=Release /t:Build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

msbuild src/zano.vcxproj  /p:Configuration=Release /t:Build

IF %ERRORLEVEL% NEQ 0 (
  goto error
)


endlocal

@echo on
echo "sources are built"




 
cd %SOURCES_PATH%/build

set cmd=src\Release\simplewallet.exe --version
FOR /F "tokens=3" %%a IN ('%cmd%') DO set version=%%a  
set version=%version:~0,-2%
echo '%version%'

cd src\release



@echo on


mkdir bunch

copy /Y zano.exe bunch
copy /Y zanod.exe bunch
copy /Y simplewallet.exe bunch
copy /Y zano.exe bunch
%QT32_PREFIX_PATH%\bin\windeployqt.exe bunch\Zano.exe

cd bunch

zip -r %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%%version%.zip *.*
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "Add html"

cd %SOURCES_PATH%\src\gui\qt-daemon\
zip -r %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%%version%.zip html
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "Add runtime stuff"


cd %ETC32_BINARIES_PATH%
zip -r %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%%version%.zip *.*
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


cd %SOURCES_PATH%\build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "---------------------------------------------------------------"
@echo "-------------------Building installer--------------------------"
@echo "---------------------------------------------------------------"

mkdir installer_src


unzip %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%%version%.zip -d installer_src
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


"%INNOSETUP_PATH%"  /dBinariesPath=../build/installer_src  /DMyAppVersion=%version% /o%BUILDS_PATH%\builds\ /f%ACHIVE_NAME_PREFIX%%version%-installer ..\utils\setup_32.iss 
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "---------------------------------------------------------------"
@echo "---------------------------------------------------------------"

@echo "   UPLOADING TO SERVER ...."

pscp -i C:\Users\Administrator\.ssh\putty.ppk %BUILDS_PATH%\builds\%ACHIVE_NAME_PREFIX%%version%-installer.exe root@95.216.11.16:/usr/share/nginx/html/%ACHIVE_NAME_PREFIX%%version%-installer.exe
IF %ERRORLEVEL% NEQ 0 (
  @echo "FAILED TO UPLOAD TO SERVER"
  goto error
)

echo "New build available at loaction: http://95.216.11.16/%ACHIVE_NAME_PREFIX%%version%-installer.exe"




goto success

:error
echo "BUILD FAILED"
exit /B %ERRORLEVEL%

:success
echo "BUILD SUCCESS"

cd ..





                                                         
