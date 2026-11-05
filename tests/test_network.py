import contextlib
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import threading
import time
import unittest


API, CLI = sys.argv[1:3]
del sys.argv[1:3]
IDN = b"Mock,VNA,123,1.0\r\n"


@contextlib.contextmanager
def instrument(exchanges, host="127.0.0.1", port=0):
    family = socket.AF_INET6 if ":" in host else socket.AF_INET
    with socket.socket(family, socket.SOCK_STREAM) as listener:
        listener.bind((host, port))
        listener.listen(1)
        listener.settimeout(12)
        errors = []

        def serve():
            try:
                with listener.accept()[0] as connection:
                    connection.settimeout(12)
                    with connection.makefile("rb", buffering=0) as stream:
                        for expected, chunks in exchanges:
                            actual = stream.readline()
                            if actual != expected:
                                raise AssertionError((expected, actual))
                            if chunks == "timeout":
                                # Wait for the client's timeout to close the socket.
                                if stream.read(1) != b"":
                                    raise AssertionError("Expected client disconnect")
                            elif chunks == "pending":
                                connection.settimeout(0.1)
                                try:
                                    connection.recv(1)
                                except socket.timeout:
                                    pass
                                else:
                                    raise AssertionError("Client did not wait for *OPC?")
                                finally:
                                    connection.settimeout(12)
                                connection.sendall(b"1\n")
                            else:
                                for chunk in chunks:
                                    if chunk is None:
                                        return
                                    connection.sendall(chunk)
                                    if len(chunks) > 1:
                                        time.sleep(0.01)
                        if stream.read(1) != b"":
                            raise AssertionError("Unexpected extra command")
            except BaseException as error:
                errors.append(error)

        worker = threading.Thread(target=serve)
        worker.start()
        try:
            yield listener.getsockname()[1]
        finally:
            worker.join(14)
            if worker.is_alive():
                raise AssertionError("Mock instrument did not finish")
            if errors:
                raise errors[0]


def run(*args, input=""):
    return subprocess.run(
        args, input=input, text=True, capture_output=True, timeout=12
    )


class NetworkTests(unittest.TestCase):
    def assert_success(self, result):
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")

    def test_api_validation(self):
        self.assert_success(run(API))

    def test_api_normal(self):
        with instrument([
            (b"*IDN?\n", [IDN[:3], IDN[3:]]),
            (b"*CLS\n", []),
            (b"VALUE?\n", [b"42\n"]),
        ]) as port:
            self.assert_success(run(API, "127.0.0.1", str(port), "normal"))

    def test_api_buffered_lines(self):
        with instrument([
            (b"*IDN?\n", [b"first\nsecond\r\n"]),
            (b"VALUE?\n", []),
        ]) as port:
            self.assert_success(run(API, "127.0.0.1", str(port), "buffered"))

    def test_api_binary_block(self):
        payload = b"\x00\n\xffabc"
        with instrument([
            (b"*IDN?\n", [IDN]),
            (b'MMEM:DATA? "C:\\screen.png"\n',
             [b"#", b"1", b"6", payload[:2], payload[2:], b"\r", b"\n"]),
            (b"VALUE?\n", [b"42\n"]),
        ]) as port:
            self.assert_success(run(API, "127.0.0.1", str(port), "block"))

    def test_api_binary_block_rejects_indefinite_length(self):
        with instrument([
            (b"*IDN?\n", [IDN]),
            (b"DATA?\n", [b"#0"]),
        ]) as port:
            self.assert_success(
                run(API, "127.0.0.1", str(port), "invalid-block")
            )

    def test_api_eof(self):
        with instrument([(b"*IDN?\n", [b"incomplete", None])]) as port:
            self.assert_success(run(API, "127.0.0.1", str(port), "eof"))

    def test_api_timeout(self):
        with instrument([(b"*IDN?\n", "timeout")]) as port:
            start = time.monotonic()
            self.assert_success(run(API, "127.0.0.1", str(port), "timeout"))
            self.assertGreaterEqual(time.monotonic() - start, 4.5)
            self.assertLess(time.monotonic() - start, 9)

    def test_cli_idn(self):
        with instrument([(b"*IDN?\n", [IDN])]) as port:
            result = run(CLI, "127.0.0.1", str(port))
            self.assert_success(result)
            self.assertEqual(result.stdout, IDN.decode().strip() + "\n")

    def test_cli_prompt(self):
        with instrument([
            (b"*IDN?\n", [IDN]),
            (b"*CLS\n", []),
            (b'DISP:TEXT "why?"\n', []),
            (b"VALUE?\n", [b"42\r", b"\n"]),
        ]) as port:
            result = run(
                CLI, "--prompt", "127.0.0.1", str(port),
                input='\n*CLS\nDISP:TEXT "why?"\nVALUE?\n.exit\n',
            )
            self.assert_success(result)
            self.assertIn("42\n", result.stdout)
            self.assertEqual(result.stdout.count("vna> "), 5)

    def test_cli_command(self):
        with instrument([
            (b'DISP:TEXT "ready?"\n', []),
        ]) as port:
            result = run(
                CLI, "127.0.0.1", str(port),
                "--command", 'DISP:TEXT "ready?"',
            )
            self.assert_success(result)
            self.assertEqual(result.stdout, "")

    def test_cli_command_query(self):
        with instrument([
            (b"VALUE?\n", [b"42\n"]),
        ]) as port:
            result = run(
                CLI, "--command", "VALUE?", "127.0.0.1", str(port)
            )
            self.assert_success(result)
            self.assertEqual(result.stdout, "42\n")

    def test_cli_command_explicit_idn(self):
        with instrument([(b"*IDN?\n", [IDN])]) as port:
            result = run(CLI, "127.0.0.1", str(port), "--command", "*IDN?")
            self.assert_success(result)
            self.assertEqual(result.stdout, IDN.decode().strip() + "\n")

    def test_cli_file(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "commands with spaces.scpi"
            path.write_bytes(
                b"# Comment 1\r\n  # Comment 2\r\n\r\n \t\r\n"
                b"*IDN?\r\nSYST:PRES  // Inline comment\r\n"
                b"// Comment only\r\nCALC1:DATA? FDATA"
            )
            with instrument([
                (b"*IDN?\n", [IDN]),
                (b"*OPC?\n", "pending"),
                (b"SYST:PRES\n", []),
                (b"*OPC?\n", [b"1\r", b"\n"]),
                (b"CALC1:DATA? FDATA\n", [b"1,2,3\n"]),
                (b"*OPC?\n", [b" +1 \n"]),
            ]) as port:
                result = run(CLI, "--file", str(path), "127.0.0.1", str(port))
            self.assert_success(result)
            self.assertEqual(result.stdout, IDN.decode().strip() + "\n1,2,3\n")

    def test_cli_file_quoted_comments_and_long_line(self):
        commands = [
            'DISP:TEXT "https://host/#path? ""//"" done"',
            "DISP:TEXT 'it''s // a # test?'",
            'DISP:TEXT "' + "x" * 10000 + '"',
        ]
        exchanges = []
        for command in commands:
            exchanges.extend([
                ((command + "\n").encode(), []),
                (b"*OPC?\n", [b"1\n"]),
            ])
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "commands.scpi"
            path.write_text(
                "\n".join("\t" + command + " // comment" for command in commands),
                encoding="utf-8",
            )
            with instrument(exchanges) as port:
                result = run(CLI, "127.0.0.1", str(port), "--file", str(path))
            self.assert_success(result)
            self.assertEqual(result.stdout, "")

    def test_cli_file_empty(self):
        for content in (b"", b"\n # comment\n // comment\n \t"):
            with self.subTest(content=content), tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / "empty.scpi"
                path.write_bytes(content)
                with instrument([]) as port:
                    result = run(CLI, "127.0.0.1", str(port), "--file", str(path))
                self.assert_success(result)
                self.assertEqual(result.stdout, "")

    def test_cli_file_missing(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "missing.scpi"
            result = run(CLI, "127.0.0.1", "--file", str(path))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("could not open command file", result.stderr)
        self.assertEqual(result.stdout, "")

    def test_cli_file_nul(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "invalid.scpi"
            path.write_bytes(b"# comment\nBAD\0COMMAND\n*CLS\n")
            with instrument([]) as port:
                result = run(CLI, "127.0.0.1", str(port), "--file", str(path))
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("NUL byte", result.stderr)
            self.assertIn("line 2 failed", result.stderr)

    def test_cli_file_completion_failure(self):
        for response in ([b"0\n"], [b"unexpected\n"], [b"11\n"],
                         [None], "timeout"):
            with self.subTest(response=response), tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / "commands.scpi"
                path.write_bytes(b"# comment\n*CLS\nNEXT\n")
                with instrument([
                    (b"*CLS\n", []),
                    (b"*OPC?\n", response),
                ]) as port:
                    result = run(CLI, "127.0.0.1", str(port), "--file", str(path))
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("*OPC?", result.stderr)
                self.assertIn("line 2 failed", result.stderr)
                self.assertEqual(result.stdout, "")

    def test_cli_file_query_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "commands.scpi"
            path.write_bytes(b"VALUE?\nNEXT\n")
            with instrument([
                (b"VALUE?\n", [None]),
            ]) as port:
                result = run(CLI, "127.0.0.1", str(port), "--file", str(path))
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("query failed", result.stderr)
            self.assertIn("line 1 failed", result.stderr)

    def test_cli_file_explicit_idn_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "commands.scpi"
            path.write_bytes(b"*IDN?\n*CLS\n")
            with instrument([(b"*IDN?\n", [None])]) as port:
                result = run(CLI, "127.0.0.1", str(port), "--file", str(path))
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("query failed", result.stderr)
            self.assertEqual(result.stdout, "")

    def test_cli_prompt_eof(self):
        with instrument([(b"*IDN?\n", [IDN]), (b"VALUE?\n", [b"42\n"])]) as port:
            result = run(
                CLI, "127.0.0.1", str(port), "--prompt", input="VALUE?"
            )
            self.assert_success(result)
            self.assertIn("42\n", result.stdout)

    def test_cli_prompt_quit(self):
        with instrument([(b"*IDN?\n", [IDN])]) as port:
            self.assert_success(run(
                CLI, "127.0.0.1", str(port), "--prompt", input="quit\n"
            ))

    def test_cli_prompt_read_failure(self):
        with instrument([
            (b"*IDN?\n", [IDN]), (b"VALUE?\n", [None]),
        ]) as port:
            result = run(
                CLI, "127.0.0.1", str(port), "--prompt", input="VALUE?\n"
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("query failed", result.stderr)

    def test_cli_prompt_invalid_input(self):
        with instrument([(b"*IDN?\n", [IDN])]) as port:
            result = run(
                CLI, "127.0.0.1", str(port), "--prompt", input="bad\0input\n"
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("NUL byte", result.stderr)

    def test_cli_large_response(self):
        response = b"x" * 100000 + b"\n"
        with instrument([(b"*IDN?\n", [response])]) as port:
            result = run(CLI, "127.0.0.1", str(port))
            self.assert_success(result)
            self.assertEqual(result.stdout, response.decode())

    def test_cli_default_port(self):
        # Do not interfere with a real service using the SCPI port.
        try:
            with socket.socket() as probe:
                probe.bind(("127.0.0.1", 5025))
        except OSError:
            self.skipTest("Port 5025 is already in use")
        with instrument([(b"*IDN?\n", [IDN])], port=5025):
            self.assert_success(run(CLI, "127.0.0.1"))
        with instrument([
            (b"*IDN?\n", [IDN]), (b"*CLS\n", []), (b"VALUE?\n", [b"42\n"]),
        ], port=5025):
            self.assert_success(run(API, "127.0.0.1", "0", "normal"))

    def test_cli_ipv6(self):
        try:
            with socket.socket(socket.AF_INET6) as probe:
                probe.bind(("::1", 0))
        except OSError:
            self.skipTest("IPv6 loopback unavailable")
        with instrument([(b"*IDN?\n", [IDN])], host="::1") as port:
            self.assert_success(run(CLI, "::1", str(port)))

    def test_cli_refused(self):
        with socket.socket() as reserved:
            reserved.bind(("127.0.0.1", 0))
            result = run(CLI, "127.0.0.1", str(reserved.getsockname()[1]))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("connection failed", result.stderr)
        self.assertEqual(result.stdout, "")

    def test_cli_invalid_ip(self):
        result = run(CLI, "not-an-ip")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Invalid argument", result.stderr)

    def test_cli_timeout(self):
        with instrument([(b"*IDN?\n", "timeout")]) as port:
            result = run(CLI, "127.0.0.1", str(port), "--prompt")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("timed out", result.stderr)
            self.assertNotIn("vna> ", result.stdout)


if __name__ == "__main__":
    unittest.main()
