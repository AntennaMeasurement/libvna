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
cmake -S . -B build -DBUILD_TESTING=ON
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
| `BUILD_SHARED_LIBS` | `OFF` | Build a shared library instead of a static library |
| `BUILD_TESTING` | `OFF` | Build and register the CTest checks |

Tests now default to OFF, like libamcore. Existing build caches retain their
previous setting; enable tests explicitly for development. Package tests
also require a C++ compiler and verify relocated C and C++ consumers.

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

The default library is static. Its only public function is `vna_version()`,
which returns a static version string owned by the library. It does not expose
connection or SCPI APIs yet. Both source-tree and installed CMake consumption
are supported; see below.

## Layout

- `include/libvna/`: public C headers
- `src/`: library implementation
- `cli/`: command-line entry point
- `tests/`: library version, libuv event-loop smoke test, and CLI checks
- `cmake/`: installed CMake package configuration
- `docs/`: MkDocs pages and version hook
- `.github/workflows/`: cross-platform build, test and release packaging
- `VERSION`: single source of the project version

The build is intended for Windows, Linux, and macOS. Tests do not require a
connected instrument.

## Shared builds, installation and packages

The default build remains static. Shared libraries and installed CMake
packages are also supported:

```sh
cmake -S . -B build-shared -DBUILD_SHARED_LIBS=ON -DBUILD_TESTING=ON
cmake --build build-shared --config Release
ctest --test-dir build-shared -C Release --output-on-failure
cmake --install build-shared --config Release
cpack --config build-shared/CPackConfig.cmake -C Release -B build-shared/packages
```

Consumers of an installed SDK can use:

```cmake
find_package(libvna CONFIG REQUIRED)
target_link_libraries(my_application PRIVATE libvna::vna)
```

Set `CMAKE_PREFIX_PATH` to the installation prefix when needed. The default
prefix is `%LOCALAPPDATA%\Programs\libvna` on Windows and `$HOME/.local` on
Unix. Override it with `CMAKE_INSTALL_PREFIX`. SDKs include libuv's static
library, development files and notices; static consumers resolve it
automatically. Shared consumers on Windows need the installed `bin` directory
on `PATH`.

`VERSION` is the single version source. Public symbol visibility and the
`vna_status` conventions live in `include/libvna/utility.h`; no connection
or measurement APIs have been introduced.

## Documentation and releases

```sh
python -m pip install -r requirements-docs.txt
python -m mkdocs build --strict
python -m mkdocs serve
```

The documentation reads the version from `VERSION`. See
[build/install](docs/public/getting-started.md), [C API](docs/public/api.md),
[CLI](docs/public/cli.md), [CMake integration](docs/public/cmake-integration.md)
and [releases](docs/public/releases.md).

For a manual release, update `VERSION` and `CHANGELOG.md`, commit all changes,
then run `bash release.sh --dry-run` followed by `bash release.sh` using
Bash (including Git Bash on Windows). The script requires a clean branch,
an `origin` remote and authenticated GitHub CLI. It creates an annotated tag,
atomically pushes branch and tag, and creates a release without modifying
files or creating commits. CI attaches tested static/shared SDK archives for
Windows, Linux and macOS. No release is published merely by changing VERSION.

Run isolated release-script checks with `bash tests/test_release.sh`; these
use local repositories and a stub GitHub CLI rather than publishing anything.
