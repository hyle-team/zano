# Building the WASM Address Validator (Windows / Linux)

This guide covers only the steps needed to produce the **WASM + JS loader** output (e.g. `address_validator_wasm.wasm` and `address_validator_wasm.js`) using **Emscripten (emsdk)**.

> Notes
> - Paths below are examples. Replace `user` and directories to match your environment.
> - Boost is used as **headers only** here — provide the correct `Boost_INCLUDE_DIR` for your system.

---

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

From your repository root (example: `C:\Users\user\zano-fork`):

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

This prints only the exported symbol names (you typically cannot see function bodies from the JS wrapper):

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

From your repository root (example: `/home/user/zano`):

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

## Output

After a successful build, you should have the generated WASM + JS loader artifacts in the build directory, typically including:

- `address_validator_wasm.js`
- `address_validator_wasm.wasm`

Exact filenames/locations may vary depending on your CMake target and build setup.

# Usage

## C/WASM signature
```c
uint8_t zano_validate_address(const char* addr_cstr);
```

## Input
- `addr_cstr`: UTF-8 null-terminated string containing the address.
- If `addr_cstr` is `NULL`, the function returns `2` (`bad_args`).

## Return codes
| Code | Name | Meaning |
|---:|---|---|
| 0 | `ok` | Address is valid and **does not** contain a payment id (regular address). |
| 1 | `invalid` | Address is invalid (parsing/validation failed). |
| 2 | `bad_args` | Invalid arguments (e.g. null pointer). |
| 3 | `zano_integrated_ok` | Address is valid and **contains** a payment id (integrated address). |
| 4 | `wrapped_like_ok` | Input string is classified as **wrapped-like** by `currency::is_address_like_wrapped`. This is a separate classification step and is not the same as full address validation. |

## Notes
- The function performs the wrapped-like check first. If the input is wrapped-like, it returns `4` without attempting to parse it as a standard Zano address.
- For non-wrapped-like inputs, the function attempts to parse and validate the address and extract a payment id. If parsing fails, it returns `1`.