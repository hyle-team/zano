[![Coverity Scan](https://scan.coverity.com/projects/18767/badge.svg)](https://scan.coverity.com/projects/zanoproject)
[![Discord](https://img.shields.io/discord/538361472691077130?label=discord&logo=discord)](https://discord.gg/wE3rmYY)

## Cloning

Be sure to clone the repository properly:\
`$ git clone --recursive https://github.com/hyle-team/zano.git`

# Building
--------


### Dependencies
| component / version | minimum <br>(not recommended but may work) | recommended | most recent of what we have ever tested |
|--|--|--|--|
| gcc (Linux) | 5.4.0 | 7.4.0 | 8.3.0 |
| llvm/clang (Linux) | UNKNOWN | 7.0.1 | 8.0.0 |
| [MSVC](https://visualstudio.microsoft.com/downloads/) (Windows) | 2015 (14.0 update 1) | 2017 (15.9.0) | 2019 |
| [XCode](https://developer.apple.com/downloads/) (macOS) | 9.2 | 12.3 | 12.3 |
| [CMake](https://cmake.org/download/) | 2.8.6 | 3.15.5 | 3.20 |
| [Boost](https://www.boost.org/users/download/) | 1.56 | 1.68 | 1.76 |
| [Qt](https://download.qt.io/archive/qt/) (*only for GUI*) | 5.8.0 | 5.11.2 | 5.15.2 |

Note:\
[*server version*] denotes steps required for building command-line tools (daemon, simplewallet, etc.).\
[*GUI version*] denotes steps required for building Zano executable with GUI.

<br />

### Linux

Recommended OS version: Ubuntu 18.04 LTS.

1. Prerequisites

   [*server version*]
   
       sudo apt-get install -y build-essential g++ python-dev autotools-dev libicu-dev libbz2-dev cmake git screen
          
   [*GUI version*]

       sudo apt-get install -y build-essential g++ python-dev autotools-dev libicu-dev libbz2-dev cmake git screen mesa-common-dev libglu1-mesa-dev

2. Download and build Boost

       wget https://boostorg.jfrog.io/artifactory/main/release/1.68.0/source/boost_1_68_0.tar.bz2
       tar -xjf boost_1_68_0.tar.bz2
       cd boost_1_68_0
       ./bootstrap.sh --with-libraries=system,filesystem,thread,date_time,chrono,regex,serialization,atomic,program_options,locale,timer
       ./b2

3. Install Qt\
(*GUI version only, skip this step if you're building server version*)

    [*GUI version*]

       wget https://download.qt.io/new_archive/qt/5.11/5.11.2/qt-opensource-linux-x64-5.11.2.run
       chmod +x qt-opensource-linux-x64-5.11.2.run
       ./qt-opensource-linux-x64-5.11.2.run
    Then follow the instructions in Wizard. Don't forget to tick the WebEngine module checkbox!

4. Set environment variables properly\
For instance, by adding the following lines to `~/.bashrc`

    [*server version*]

       export BOOST_ROOT=/home/user/boost_1_68_0  


    [*GUI version*]

       export BOOST_ROOT=/home/user/boost_1_68_0  
       export QT_PREFIX_PATH=/home/user/Qt5.11.2/5.11.2/gcc_64



5. Building binaries
   1. Building daemon and simplewallet:

          cd zano/ && make -j1
      or 
   
          cd zano && mkdir build && cd build
          cmake ..
          make -j1 daemon simplewallet

      **NOTICE**: If you are building on a machine with a relatively high amount of RAM or with the proper setting of virtual memory, then you can use `-j2` or `-j` option to speed up the building process. Use with caution.
   
   1. Building GUI:

          cd zano
          utils/build_sript_linux.sh

7. Look for the binaries in `build` folder

<br />

### Windows
Recommended OS version: Windows 7 x64.
1. Install required prerequisites (Boost, Qt, CMake).
2. Edit paths in `utils/configure_local_paths.cmd`.
3. Run `utils/configure_win64_msvs2015_gui.cmd` or `utils/configure_win64_msvs2017_gui.cmd` according to your MSVC version.
4. Go to the build folder and open generated Zano.sln in MSVC.
5. Build.

In order to correctly deploy Qt GUI application, you also need to do the following:

6. Copy Zano.exe to a folder (e.g. `depoy`). 
7. Run  `PATH_TO_QT\bin\windeployqt.exe deploy\Zano.exe`.
8. Copy folder `\src\gui\qt-daemon\html` to `deploy\html`.
9. Now you can run `Zano.exe`

<br />

### macOS
Recommended OS version: macOS Sierra 10.15.4 x64.
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
    h. Unfold the certificate in Keychain Access window and double click the underlying private key "Zano". Select "Access Control" tab, then select "Allow all applications to access this item". Click "Save Changes".
2. Revise building script, comment out unwanted steps and run it:  `utils/build_script_mac_osx.sh`
3. The application should be here: `/buid_mac_osx_64/release/src`

