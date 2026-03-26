"""
CLI tool for communicating with the UnrealClientProtocol TCP plugin.

Accepts a single JSON command via command-line argument or stdin.
Connects to the UE editor via TCP (4-byte LE length-prefixed framing).

Usage:
    python UCP.py '<json>'
    echo <json> | python UCP.py

Environment:
    UE_HOST    (default 127.0.0.1)
    UE_PORT    (default 9876)
    UE_TIMEOUT (default 30)
"""

import struct
import socket
import json
import sys
import os

UE_HOST = os.environ.get("UE_HOST", "127.0.0.1")
UE_PORT = int(os.environ.get("UE_PORT", "9876"))
TIMEOUT = float(os.environ.get("UE_TIMEOUT", "30"))


def send_receive(sock: socket.socket, payload: dict) -> dict:
    body = json.dumps(payload).encode("utf-8")
    sock.sendall(struct.pack("<I", len(body)) + body)
    raw_len = _recv_exact(sock, 4)
    resp_len = struct.unpack("<I", raw_len)[0]
    raw_body = _recv_exact(sock, resp_len)
    return json.loads(raw_body.decode("utf-8"))


def _recv_exact(sock: socket.socket, n: int) -> bytes:
    buf = bytearray()
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError("UE connection closed unexpectedly")
        buf.extend(chunk)
    return bytes(buf)


def _simplify(resp: dict):
    """Strip success/result envelope, preserve log field.
    Success -> return result value directly (+ log if present).
    Failure -> return {error, expected?, log?} only."""
    if resp.get("success"):
        out = resp.get("result")
        if "log" in resp:
            if isinstance(out, dict):
                out["log"] = resp["log"]
            else:
                out = {"result": out, "log": resp["log"]}
        return out
    out = {"error": resp.get("error", "Unknown error")}
    if "expected" in resp:
        out["expected"] = resp["expected"]
    if "log" in resp:
        out["log"] = resp["log"]
    return out


def execute(command: dict) -> str:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(TIMEOUT)
    try:
        sock.connect((UE_HOST, UE_PORT))
        resp = send_receive(sock, command)
        return json.dumps(_simplify(resp), indent=2, ensure_ascii=False)
    except (ConnectionError, ConnectionRefusedError, OSError) as e:
        return json.dumps({"error": f"Cannot connect to UE ({e}). Is the editor running?"}, ensure_ascii=False)
    finally:
        sock.close()


def main():
    raw = sys.argv[1] if len(sys.argv) > 1 else sys.stdin.read()

    try:
        data = json.loads(raw)
    except json.JSONDecodeError as e:
        print(json.dumps({"error": f"Invalid JSON: {e}"}))
        sys.exit(1)

    if not isinstance(data, dict):
        print(json.dumps({"error": "Expected a JSON object, not an array or primitive"}))
        sys.exit(1)

    print(execute(data))


if __name__ == "__main__":
    main()
