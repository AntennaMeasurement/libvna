# Build and install

Requirements: CMake 3.24+, a C11 compiler (MSVC, GCC or Clang), and internet
access on the first configure. Tests additionally require a C++ compiler for
the installed-package compatibility checks.

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release
```

For single-configuration generators, also configure with
`-DCMAKE_BUILD_TYPE=Release`.

| Option | Default | Meaning |
| --- | --- | --- |
| `BUILD_SHARED_LIBS` | `OFF` | Build a shared library instead of a static library |
| `BUILD_TESTING` | `OFF` | Build/register library, backend, CLI and installed-package tests |
| `LIBVNA_BUILD_CLI` | `ON` | Build and install the CLI |

Tests were previously enabled by default. Existing CMake caches retain their
chosen value; use `-DBUILD_TESTING=ON` explicitly for development.

Default installation is `%LOCALAPPDATA%\Programs\libvna` on Windows and
`$HOME/.local` on Unix. Override it with `-DCMAKE_INSTALL_PREFIX=<directory>`.
Installation includes the library, public headers, optional CLI, CMake package
and bundled libuv development files and notices.

The pinned libuv 1.51.0 archive is SHA256-verified and built statically, even
when libvna is shared. On Unix it is compiled as position-independent code.
For an offline build:

```sh
cmake -S . -B build -DFETCHCONTENT_SOURCE_DIR_LIBUV=<extracted-libuv-1.51.0>
```

Do not disable TLS verification to work around certificate errors. Repair the
network/certificate configuration or obtain the pinned archive through a
trusted HTTPS client and use the offline option.

Create a native SDK archive after building:

```sh
cpack --config build/CPackConfig.cmake -C Release -B build/packages
```

Archives use ZIP on Windows and TGZ elsewhere. They contain installable SDK
files, not tests or documentation sources. Shared-library applications on
Windows must make the installed `bin` directory available on `PATH`.

CTest also installs into a temporary build-tree prefix, relocates it, and
compiles/runs independent C and C++ consumers using `find_package`.
