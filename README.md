
[![Coverity Scan](https://scan.coverity.com/projects/18767/badge.svg)](https://scan.coverity.com/projects/zanoproject)

Building
--------

### Cloning

Be sure to properly clone the repository:

`$ git clone --recursive https://github.com/hyle-team/zano.git`

### Dependencies
| component / version | minimum <br>(not recommended but may work) | recommended | most recent of what we have ever tested |
|--|--|--|--|
| gcc (Linux) | 5.4.0 | 7.2.0 | 8.3.0 |
| llvm/clang (Linux) | UNKNOWN | 7.0.1 | 8.0.0 |
| [MSVC](https://visualstudio.microsoft.com/downloads/) (Windows) | 2015 (14.0 update 1) | 2015 (14.0 update 3) | 2017 (15.5.7) |
| [XCode](https://developer.apple.com/downloads/) (macOS) | 7.3.1 | 9.2 | 9.2 |
| [CMake](https://cmake.org/download/) | 2.8.6 | 3.4.1 | 3.11.0 |
| [Boost](https://www.boost.org/users/download/) | 1.56 | 1.60 | 1.66 |
| [Qt](https://download.qt.io/archive/qt/) (only for GUI) | 5.8.0 | 5.9.1 | 5.10.1 |

### Linux

Recommended OS version: Ubuntu 17.04 LTS.

1. For server version: \
`$ sudo apt-get install -y build-essential g++ python-dev autotools-dev libicu-dev libbz2-dev cmake git libboost-all-dev screen`\
For GUI version:\
`$ sudo apt-get install -y build-essential g++ python-dev autotools-dev libicu-dev libbz2-dev cmake git libboost-all-dev screen mesa-common-dev libglu1-mesa-dev qt5-default qtwebengine5-dev`

2. Building binaries \
  2.1. Building daemon and simplewallet: \
   &nbsp;&nbsp;`$ cd zano/ && make -j` \
   &nbsp;&nbsp;or  \
   &nbsp;&nbsp;`$ cd zano && mkdir build && cd build `\
   &nbsp;&nbsp;`$ cmake .. `\
   &nbsp;&nbsp;`$ make -j daemon simplewallet` \ 
  2.2. Building GUI: \
   &nbsp;&nbsp;`$ cd zano/ && make -j gui ` \
   &nbsp;&nbsp;or \
   &nbsp;&nbsp;`$ cd zano && mkdir build && cd build `\
   &nbsp;&nbsp;`$ cmake -D BUILD_GUI=ON .. `\
   &nbsp;&nbsp;`$ make -j Zano` \
NOTICE: If you are building on machine with relatively small anount of RAM(small VPS for example, less then 16GB) and without proper setting of virtual memory, then be careful with setting `-j` option, this may cause compiller crashes. 
3. Look for the binaries, including the `Zano` GUI, in the build directory

### Windows
Recommended OS version: Windows 7 x64.
1. Install required prerequisites.
2. Edit paths in `utils/configure_local_paths.cmd`.
3. Run `utils/configure_win64_msvs2015_gui.cmd` or `utils/configure_win64_msvs2017_gui.cmd` according to your MSVC version.
4. Go to the build folder and open generated Zano.sln in MSVC.
5. Build.

In order to correctly deploy Qt GUI application you also need to do the following:
6. Copy Zano.exe to a folder (e.g. `depoy`). 
7. Run  `PATH_TO_QT\bin\windeployqt.exe deploy/Zano.exe`.
8. Copy folder `\src\gui\qt-daemon\html` to `deploy\html`.

### macOS
Recommended OS version: macOS Sierra 10.12.6 x64.
1. Install required prerequisites.
2. Set environment variables as stated in `utils/macosx_build_config.command`.
3.  `mkdir build` <br> `cd build` <br> `cmake ..` <br> `make`

To build GUI application:

1. Create self-signing certificate via Keychain Access:\
    a. Run Keychain Access.\
    b. Choose Keychain Access > Certificate Assistant > Create a Certificate.\
    c. Use “Zano” (without quotes) as certificate name.\
    d. Choose “Code Signing” in “Certificate Type” field.\
    e. Press “Create”, then “Done”.\
    f. Make sure the certificate was added to keychain "System". If not—move it to "System".\
    g. Double click the certificate you've just added, enter the trust section and under "When using this certificate" select "Always trust".\
    h. Unfold the certificate in Keychain Access window and double click underlying private key "Zano". Select "Access Control" tab, then select "Allow all applications to access this item". Click "Save Changes".
2. Revise building script, comment out unwanted steps and run it:  `utils/build_script_mac_osx.sh`
3. The application should be here: `/buid_mac_osx_64/release/src`
