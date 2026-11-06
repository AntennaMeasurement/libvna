# libvnascpi

A cross-platform C library and CLI for controlling vector network analyzers
over TCP using SCPI. Numeric IPv4/IPv6 connections, text commands, queries,
and an interactive CLI are supported. Measurement-specific APIs will be added
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
.\build\Release\vna.exe 192.168.1.100
.\build\Release\vna.exe 192.168.1.100 5025 --prompt
.\build\Release\vna.exe 192.168.1.100 --command "SYST:ERR?"
.\build\Release\vna.exe 192.168.1.100 --file "commands.scpi"
```

Linux/macOS with a single-configuration generator:

```sh
./build/vna --help
./build/vna --version
./build/vna 192.168.1.100
./build/vna 192.168.1.100 5025 --prompt
./build/vna 192.168.1.100 --command "SYST:ERR?"
./build/vna 192.168.1.100 --file "commands.scpi"
```

Calling the CLI without arguments displays help. Unsupported arguments fail
with a nonzero exit code and an error on stderr. With only an IP address/port, the CLI
connects (default port 5025), prints the `*IDN?` response, and disconnects.
`--prompt` keeps the connection open for commands; `.exit`, `exit`, `quit`, or
EOF exits. On an interactive terminal, use the up and down arrow keys to browse
commands entered during the current session.
`--command <command>` sends one SCPI command directly and then exits;
queries print their response. It cannot be combined with `--prompt`.
Queries containing `?` outside quoted strings print one response line.
Other commands are sent without waiting for a response.
`--file <file_path>` runs commands from a text file directly.
Neither mode sends an automatic `*IDN?`; explicit `*IDN?` commands still work.
Blank lines and `#` comment lines are skipped; `//` comments outside quoted
strings are removed. After each command (including the last), the CLI waits
for `*OPC?` to return `1` before continuing; internal completion responses
are not printed. Failures stop execution with a nonzero exit code.
The existing 5-second response timeout also applies to `*OPC?`.
File mode cannot be combined with `--prompt` or `--command`.
See the [CLI guide](docs/public/cli.md#command-files) for the file format.

### Options

| CMake option | Default | Description |
| --- | --- | --- |
| `LIBVNASCPI_BUILD_CLI` | `ON` | Build the `vna` executable |
| `BUILD_SHARED_LIBS` | `OFF` | Build a shared library instead of a static library |
| `BUILD_TESTING` | `OFF` | Build and register the CTest checks |

Tests now default to OFF, like libamcore. Existing build caches retain their
previous setting; enable tests explicitly for development. Package tests
also require a C++ compiler and verify relocated C and C++ consumers.
Network tests with the CLI enabled require Python 3 (standard library only)
and use loopback mock instruments, not physical hardware.

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
add_subdirectory(libvnascpi)
target_link_libraries(my_application PRIVATE libvnascpi::vna)
```

```c
#include <libvnascpi/vna.h>

const char *version = vna_version();

vna_status status;
vna_connection *device = vna_connect("192.168.1.100", 0, &status);
if (device == NULL) {
    /* Report vna_status_string(status) to the caller. */
} else {
    char *response;
    status = vna_query(device, "*IDN?", &response);
    if (status == VNA_STATUS_SUCCESS) {
        /* Use response before releasing it. */
        vna_free_response(response);
    }
    /* Report any failed query using vna_status_string(status). */
    vna_disconnect(device);
}
```

The default library is static. `vna_version()` returns a static version string.
`vna_connect()` returns an opaque connection pointer, or `NULL` with a status
code on failure; port `0` selects 5025. `vna_send()` sends a command and
`vna_query()` returns an allocated text response. Connect, send, and response
read stages each have a 5-second timeout. Calls on the same connection must
not overlap. Both source-tree and installed CMake consumption are supported.

## Layout

- `include/libvnascpi/`: public C headers
- `src/`: library implementation
- `cli/`: command-line entry point
- `tests/`: version, backend, TCP/SCPI, CLI, and installed-package checks
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
find_package(libvnascpi CONFIG REQUIRED)
target_link_libraries(my_application PRIVATE libvnascpi::vna)
```

Set `CMAKE_PREFIX_PATH` to the installation prefix when needed. The default
prefix is `%LOCALAPPDATA%\Programs\libvnascpi` on Windows and `$HOME/.local` on
Unix. Override it with `CMAKE_INSTALL_PREFIX`. SDKs include libuv's static
library, development files and notices; static consumers resolve it
automatically. Shared consumers on Windows need the installed `bin` directory
on `PATH`.

`VERSION` is the single version source. Public symbol visibility and the
`vna_status` conventions live in `include/libvnascpi/utility.h`; connection APIs
live in `include/libvnascpi/connection.h` and are included by the umbrella header.

## Documentation and releases

```sh
python -m pip install -r docs/requirements.txt
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
