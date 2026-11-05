# Command-line interface

On Windows with Visual Studio:

```powershell
.\build\Release\vna.exe --help
.\build\Release\vna.exe --version
```

For a single-configuration Unix build:

```sh
./build/vna --help
./build/vna --version
```

No arguments, `--help`, and `-h` display help. `--version` and `-v` print
`libvna {{ libvna_version }}`. Success writes to stdout and returns zero.
Unsupported arguments, including extra arguments after a supported option,
write an error and help to stderr and return a nonzero exit code.

No command opens a network connection. Connection and measurement commands
will be added alongside their library APIs. Set `LIBVNA_BUILD_CLI=OFF` to
build only the library.
