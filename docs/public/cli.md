# Command-line interface

On Windows with Visual Studio:

```powershell
.\build\Release\vna.exe --help
.\build\Release\vna.exe --version
.\build\Release\vna.exe 192.168.1.100
.\build\Release\vna.exe 192.168.1.100 5025 --prompt
.\build\Release\vna.exe 192.168.1.100 --command "SYST:ERR?"
.\build\Release\vna.exe 192.168.1.100 --file "commands.scpi"
```

For a single-configuration Unix build:

```sh
./build/vna --help
./build/vna --version
./build/vna 192.168.1.100
./build/vna 192.168.1.100 5025 --prompt
./build/vna 192.168.1.100 --command "SYST:ERR?"
./build/vna 192.168.1.100 --file "commands.scpi"
```

No arguments, `--help`, and `-h` display help. `--version` and `-v` print
`libvna {{ libvna_version }}`. Success writes to stdout and returns zero.
Unsupported arguments, including extra arguments after a supported option,
write an error and help to stderr and return a nonzero exit code.

## Connecting

```text
vna <ip> [port] [--prompt | --command <command> | --file <file_path>]
```

The IP must be a numeric IPv4 or IPv6 address (no hostname or URL).
The port must be between 1 and 65535; omitting it selects **5025**.
Without `--command` or `--file`, the CLI connects, sends `*IDN?` with an LF terminator, prints the response
without its CR/LF terminator, then closes the connection.

`--prompt` may appear before or after the address/port. After a successful
IDN query it enters a `vna> ` prompt. Enter single-line SCPI commands:

```text
vna> *CLS
vna> SYST:ERR?
0,"No error"
vna> .exit
```

Commands containing `?` outside single- or double-quoted strings are queries:
the CLI prints one LF-terminated response line. Other commands are sent
without waiting for a response, since SCPI setters normally return nothing.
Empty lines are ignored. The up and down arrow keys browse commands entered
during the current interactive session. `.exit`, `exit`, `quit`, or EOF closes
the connection.
Binary block and multiline responses are not supported.

`--command <command>` sends one SCPI command directly, without an automatic
`*IDN?` query, and then
exits. Queries print one LF-terminated response line; other commands are sent
without waiting for a response. Quote the command argument when it contains
spaces. This option cannot be combined with `--prompt`.

## Command files

`--file <file_path>` executes a text file without an automatic `*IDN?` query, then
exits. It may appear before or after the address/port and cannot be combined
with `--prompt` or `--command`. Quote paths containing spaces.
Explicit `*IDN?` commands in either mode are executed normally.

```text
# Comment line 1
# Comment line 2
*IDN?
SYST:PRES                  // Inline comment
CALC1:DATA? FDATA
```

Each nonempty line is one SCPI command. Blank lines and lines whose first
non-whitespace character is `#` are ignored. `//` starts an inline comment
only outside single- or double-quoted strings. Leading and trailing whitespace
is removed; quoted text (including doubled quote escapes) is preserved.
LF and CRLF line endings and a final line without a newline are supported.

Query responses are printed as in `--command`. After every file command,
including the last one, the CLI sends `*OPC?` and waits for a completion
response of `1` (also accepting `+1` and surrounding whitespace) before
proceeding. These internal completion responses are not printed.
`*OPC?` itself waits on the instrument for pending operations; the CLI does
not repeatedly poll it. A file command containing `*OPC?` is still executed
normally and its response is printed.

An unreadable file, invalid input (such as a NUL byte), command failure,
unexpected completion response, or timeout stops execution with an error
and a nonzero exit code. Execution errors include the file path and line
number; subsequent commands are not sent.

Each connect, write, and response-read stage has a 5-second timeout.
This also applies to the automatic `*OPC?` queries in file mode.
Network failures/timeouts report an error to stderr, disconnect, and exit
with a nonzero code. A failed IDN query never enters prompt mode.

Set `LIBVNA_BUILD_CLI=OFF` to build only the library.
