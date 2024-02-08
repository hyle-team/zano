call configure_local_paths.cmd

;; MSVC version-specific paths
SET QT_MSVC_PATH=%QT_PREFIX_PATH%\msvc2017_64

SET ACHIVE_NAME_PREFIX=zano-win-x64-
SET MY_PATH=%~dp0
SET SOURCES_PATH=%MY_PATH:~0,-7%

IF NOT [%build_prefix%] == [] (
  SET ACHIVE_NAME_PREFIX=%ACHIVE_NAME_PREFIX%%build_prefix%-
)

IF "%testnet%" == "true" (
  SET TESTNET_DEF=-D TESTNET=TRUE
  SET TESTNET_LABEL=testnet 
  SET ACHIVE_NAME_PREFIX=%ACHIVE_NAME_PREFIX%testnet-
)

SET PARAM=%~1
IF "%PARAM%"=="--skip-build" ( GOTO skip_build )

@echo on

set BOOST_ROOT=%LOCAL_BOOST_PATH%
set BOOST_LIBRARYDIR=%LOCAL_BOOST_LIB_PATH%



@echo "---------------- PREPARING BINARIES ---------------------------"
@echo "---------------------------------------------------------------"



cd %SOURCES_PATH%

@echo "---------------- BUILDING APPLICATIONS ------------------------"
@echo "---------------------------------------------------------------"




rmdir build /s /q
mkdir build
cd build

cmake %TESTNET_DEF% -D OPENSSL_ROOT_DIR="%OPENSSL_ROOT_DIR%" -D CMAKE_PREFIX_PATH="%QT_MSVC_PATH%" -D BUILD_GUI=TRUE -D STATIC=FALSE -DBOOST_ROOT="%BOOST_ROOT%" -DBOOST_LIBRARYDIR="%BOOST_ROOT%\lib64-msvc-14.2" -G "Visual Studio 16 2019" -A x64 -T host=x64 ..
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" x86_amd64
echo on
cd %SOURCES_PATH%\build

msbuild version.vcxproj /p:SubSystem="CONSOLE,5.02"  /p:Configuration=Release /t:Build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

msbuild src/daemon.vcxproj /p:SubSystem="CONSOLE,5.02"  /p:Configuration=Release /t:Build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

msbuild src/simplewallet.vcxproj /p:SubSystem="CONSOLE,5.02"  /p:Configuration=Release /t:Build
IF %ERRORLEVEL% NEQ 0 (
  goto error
)

msbuild src/Zano.vcxproj /p:SubSystem="WINDOWS,5.02" /p:Configuration=Release /t:Build

IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo on
echo "sources are built successfully"




:skip_build
cd %SOURCES_PATH%/build

set cmd=src\Release\simplewallet.exe --version
FOR /F "tokens=3" %%a IN ('%cmd%') DO set version=%%a  
set version=%version:~0,-2%
echo '%version%'

set build_zip_filename=%ACHIVE_NAME_PREFIX%%version%.zip
set build_zip_path=%BUILDS_PATH%\builds\%build_zip_filename%

del /F /Q %build_zip_path%

cd src\release

call :sign_file Zano.exe || goto error
call :sign_file zanod.exe || goto error
call :sign_file simplewallet.exe || goto error

@echo on


mkdir bunch

copy /Y Zano.exe bunch
copy /Y zanod.exe bunch
copy /Y simplewallet.exe bunch
copy /Y *.pdb bunch

%QT_MSVC_PATH%\bin\windeployqt.exe bunch\Zano.exe || goto error

cd bunch

zip -r %build_zip_path% *.*
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "Add html"

cd %SOURCES_PATH%\src\gui\qt-daemon\layout
zip -x html/package.json html/gulpfile.js html/less/* -r %build_zip_path% html
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "Add runtime stuff"


cd %ETC_BINARIES_PATH%
zip -r %build_zip_path% *.*
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


unzip -o %build_zip_path% -d installer_src
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


"%INNOSETUP_PATH%"  /dBinariesPath=../build/installer_src /DMyAppVersion=%version% /o%BUILDS_PATH%\builds\ /f%ACHIVE_NAME_PREFIX%%version%-installer ..\utils\setup_64.iss 
IF %ERRORLEVEL% NEQ 0 (
  goto error
)


@echo "---------------------------------------------------------------"
@echo "---------------------------------------------------------------"

set installer_file=%ACHIVE_NAME_PREFIX%%version%-installer.exe
set installer_path=%BUILDS_PATH%\builds\%installer_file%

@echo "   SIGNING the installer ...."

call :sign_file %installer_path% || goto error

@echo "   UPLOADING TO SERVER ...."

pscp -load zano_build_server %installer_path% %ZANO_BUILDS_HOST%:/var/www/html/builds
IF %ERRORLEVEL% NEQ 0 (
  @echo "FAILED TO UPLOAD EXE TO SERVER"
  goto error
)
call :sha256 %installer_path% installer_checksum

pscp -load zano_build_server %build_zip_path% %ZANO_BUILDS_HOST%:/var/www/html/builds
IF %ERRORLEVEL% NEQ 0 (
  @echo "FAILED TO UPLOAD ZIP TO SERVER"
  goto error
)
call :sha256 %build_zip_path% build_zip_checksum

set mail_msg="New %build_prefix% %TESTNET_LABEL%build for win-x64:<br>INST: <a href='https://build.zano.org/builds/%installer_file%'>https://build.zano.org/builds/%installer_file%</a> <br>sha256: %installer_checksum%<br><br>ZIP:  <a href='https://build.zano.org/builds/%build_zip_filename%'>https://build.zano.org/builds/%build_zip_filename%</a> <br>sha256: %build_zip_checksum%<br>"

echo %mail_msg%

python ../utils/build_mail.py "Zano win-x64 %build_prefix% %TESTNET_LABEL%build %version%" "%emails%" %mail_msg%

goto success

:error
echo "BUILD FAILED"
exit /B %ERRORLEVEL%

:success
echo "BUILD SUCCESS"

cd ..

EXIT /B %ERRORLEVEL%


:: functions

:sha256
@setlocal enabledelayedexpansion
@set /a count=1 
@for /f "skip=1 delims=:" %%a in ('CertUtil -hashfile %1 SHA256') do @(
  @if !count! equ 1 set "hash=%%a"
  @set /a count+=1
)
@(
 @endlocal
 @set "%2=%hash: =%
)
@exit /B 0


:sign_file
@echo Signing %1...
@call %ZANO_SIGN_CMD% %1
@if %ERRORLEVEL% neq 0 (
  @echo ERROR: failed to sign %1
  @exit /B 1
)
@exit /B 0
