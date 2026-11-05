# Command-line interface

On Windows with Visual Studio:

```powershell
.\build\Release\vna.exe --help
.\build\Release\vna.exe --version
.\build\Release\vna.exe 192.168.1.100
.\build\Release\vna.exe 192.168.1.100 5025 --prompt
```

For a single-configuration Unix build:

```sh
./build/vna --help
./build/vna --version
./build/vna 192.168.1.100
./build/vna 192.168.1.100 5025 --prompt
```

No arguments, `--help`, and `-h` display help. `--version` and `-v` print
`libvna {{ libvna_version }}`. Success writes to stdout and returns zero.
Unsupported arguments, including extra arguments after a supported option,
write an error and help to stderr and return a nonzero exit code.

## Connecting

```text
vna <ip> [port] [--prompt]
```

The IP must be a numeric IPv4 or IPv6 address (no hostname or URL).
The port must be between 1 and 65535; omitting it selects **5025**.
The CLI connects, sends `*IDN?` with an LF terminator, prints the response
without its CR/LF terminator, then closes the connection.

`--prompt` may appear before or after the address/port. After a successful
IDN query it enters a `vna> ` prompt. Enter single-line SCPI commands:

```text
vna> *CLS
vna> SYST:ERR?
0,"No error"
vna> exit
```

Commands containing `?` outside single- or double-quoted strings are queries:
the CLI prints one LF-terminated response line. Other commands are sent
without waiting for a response, since SCPI setters normally return nothing.
Empty lines are ignored. `exit`, `quit`, or EOF closes the connection.
Binary block and multiline responses are not supported.

Each connect, write, and response-read stage has a 5-second timeout.
Network failures/timeouts report an error to stderr, disconnect, and exit
with a nonzero code. A failed IDN query never enters prompt mode.

Set `LIBVNA_BUILD_CLI=OFF` to build only the library.
