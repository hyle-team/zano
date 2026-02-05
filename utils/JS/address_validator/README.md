# Building the WASM Address Validator (Windows / Linux)

## Windows

### 1) Install and enable Emscripten SDK
```bat
git clone https://github.com/emscripten-core/emsdk.git C:\dev\emsdk
cd /d C:\dev\emsdk

emsdk install latest
emsdk activate latest
emsdk_env.bat
```

### 2) Configure & build the WASM target
```bat
cd /d C:\Users\user\zano-fork

rmdir /s /q build_wasm_addr_validator
mkdir build_wasm_addr_validator
cd build_wasm_addr_validator

emcmake cmake -S ..\utils\JS\address_validator -B . ^
  -D Boost_INCLUDE_DIR=C:\local\boost_1_84_0 ^
  -D CMAKE_BUILD_TYPE=Release

cmake --build . --config Release -j
```

### 3) Inspect exported function names (Node.js)
```bat
node -e "require('./address_validator_wasm.js')().then(m=>console.log(Object.keys(m)))"
```

---

## Linux

### 1) Install and enable Emscripten SDK

```bash
git clone https://github.com/emscripten-core/emsdk.git ~/dev/emsdk
cd ~/dev/emsdk

./emsdk install latest
./emsdk activate latest
```

### 2) Configure & build the WASM target
```bash
cd /home/user/zano

rm -rf build-wasm
mkdir build-wasm
cd build-wasm

source ~/dev/emsdk/emsdk_env.sh

cmake ..   -DCMAKE_TOOLCHAIN_FILE="$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"   -DBoost_INCLUDE_DIR="$HOME/boost_1_84_0"   -DCMAKE_BUILD_TYPE=Release

cmake --build . -j
```

### 3) Inspect exported function names (Node.js)

```bash
node -e "require('./address_validator_wasm.js')().then(m=>console.log(Object.keys(m)))"
```

---

## Build artifacts
- `address_validator_wasm.js`
- `address_validator_wasm.wasm`

# Usage

## Function signature
```c
uint8_t zano_validate_address(const char* addr_cstr);
```
## Return codes
| Code | Name |
|---:|---|
| 0 | `ok` |
| 1 | `invalid` |
| 2 | `bad_args` |
| 3 | `zano_integrated_ok` |
| 4 | `wrapped_like_ok` |
