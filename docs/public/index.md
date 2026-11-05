# libvna

Version: **{{ libvna_version }}**

libvna is a cross-platform C library and CLI for controlling vector network
analyzers over TCP using SCPI. Numeric IPv4/IPv6 connections, text commands,
queries, and an interactive CLI are supported. Measurement-specific APIs
are not implemented yet.

The project separates public headers, library implementation, CLI and tests.
The CLI depends on the library, not the other way around. The libuv backend is
private; public headers do not expose libuv types.

The infrastructure follows libamcore's library/CLI architecture, including
static and shared libraries, installed CMake packages and native SDK archives.
C11 and libuv are retained for this project's networking requirements; there is
no WebAssembly target.

See [build and install](getting-started.md), [the C API](api.md),
[the CLI](cli.md), [CMake integration](cmake-integration.md), and
[releases](releases.md).
