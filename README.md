# libvna

A cross-platform C library and CLI for controlling vector network analyzers
over TCP using SCPI. The project is currently a buildable skeleton; device
connections, SCPI commands, queries, and measurement functions will be added
later.

## Requirements

- CMake 3.24 or newer
- A C11 compiler (MSVC, GCC, or Clang)
- Internet access on the first configure to download libuv

CMake downloads the pinned libuv 1.51.0 source archive, verifies its SHA256
checksum, and builds it as a static dependency. No separate libuv installation
or Git executable is required. libuv headers are not part of the public API.

## Build and test

Run from the repository root on Windows, Linux, or macOS:

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

For single-configuration generators such as Ninja or Unix Makefiles, set
`-DCMAKE_BUILD_TYPE=Release` in the configure command.

Windows with Visual Studio's generator:

```powershell
.\build\Release\vna.exe --help
.\build\Release\vna.exe --version
```

Linux/macOS with a single-configuration generator:

```sh
./build/vna --help
./build/vna --version
```

Calling the CLI without arguments displays help. Unsupported arguments fail
with a nonzero exit code and an error on stderr. No network operations are
performed yet.

### Options

| CMake option | Default | Description |
| --- | --- | --- |
| `LIBVNA_BUILD_CLI` | `ON` | Build the `vna` executable |
| `BUILD_TESTING` | `ON` | Build and register the CTest checks |

For an offline build, provide an already extracted libuv 1.51.0 source tree:

```sh
cmake -S . -B build -DFETCHCONTENT_SOURCE_DIR_LIBUV=<libuv-source-directory>
```

On Windows, a `CRYPT_E_NO_REVOCATION_CHECK` download error indicates that the
system cannot check certificate revocation. Fix the certificate/network
configuration, or download and extract the pinned source through a trusted
working HTTPS client and use the offline option above. Do not disable TLS
certificate verification.

## Use in another CMake project

```cmake
add_subdirectory(libvna)
target_link_libraries(my_application PRIVATE libvna::vna)
```

```c
#include <libvna/vna.h>

const char *version = vna_version();
```

The initial library is static. Its only public function is `vna_version()`,
which returns a static version string owned by the library. It does not expose
connection or SCPI APIs yet. Installation and package export rules are not
provided at this stage; use `add_subdirectory` to consume the library.

## Layout

- `include/libvna/`: public C headers
- `src/`: library implementation
- `cli/`: command-line entry point
- `tests/`: library version, libuv event-loop smoke test, and CLI checks

The build is intended for Windows, Linux, and macOS. Tests do not require a
connected instrument.
