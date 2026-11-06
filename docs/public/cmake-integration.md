# CMake integration

## Installed SDK

```cmake
find_package(libvnascpi CONFIG REQUIRED)
target_link_libraries(my_application PRIVATE libvnascpi::vna)
```

Configure the consuming application with
`-DCMAKE_PREFIX_PATH=<libvnascpi-install-prefix>` if the prefix is not already
searched by CMake. The target carries include directories, the C11 requirement
and shared-library import definitions.

Static libvnascpi packages locate the bundled libuv CMake package automatically
and propagate its linker requirements without exposing libuv in public
headers. Shared packages do not require consumers to locate libuv.
Keep the SDK directory layout intact when relocating it.

Version checks are supported:

```cmake
find_package(libvnascpi 0.1.0 CONFIG REQUIRED)
```

The package uses CMake's `SameMajorVersion` policy, like libamcore. The API is
still pre-1.0; this package selection policy is not a promise of stable ABI.

## Source integration

```cmake
add_subdirectory(libvnascpi)
target_link_libraries(my_application PRIVATE libvnascpi::vna)
```

Set `LIBVNASCPI_BUILD_CLI=OFF` and `BUILD_TESTING=OFF` before adding the directory
if those targets are not needed. `BUILD_SHARED_LIBS` selects libvnascpi's library
type; the libuv backend remains static.
