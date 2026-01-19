import json
import sys
from pathlib import Path


def fail(msg: str) -> None:
    raise SystemExit(msg)


def require(obj, key, type_name: str) -> None:
    if key not in obj:
        fail(f"missing field: {key}")
    if type_name == "str" and not isinstance(obj[key], str):
        fail(f"field {key} must be string")
    if type_name == "bool" and not isinstance(obj[key], bool):
        fail(f"field {key} must be bool")
    if type_name == "num" and not isinstance(obj[key], (int, float)):
        fail(f"field {key} must be number")
    if type_name == "obj" and not isinstance(obj[key], dict):
        fail(f"field {key} must be object")


def validate_stream_json(lines):
    seen_calls = set()
    for i, raw in enumerate(lines, start=1):
        raw = raw.strip()
        if not raw:
            continue
        try:
            obj = json.loads(raw)
        except Exception as e:
            fail(f"line {i}: invalid json: {e}")
        if not isinstance(obj, dict):
            fail(f"line {i}: must be object")
        require(obj, "type", "str")
        t = obj["type"]

        if t == "init":
            pass
        elif t == "message":
            require(obj, "role", "str")
            require(obj, "content", "str")
        elif t == "tool_call":
            require(obj, "call_id", "str")
            require(obj, "name", "str")
            if "arguments" in obj and not isinstance(obj["arguments"], dict):
                fail(f"line {i}: arguments must be object")
            seen_calls.add(obj["call_id"])
        elif t == "tool_result":
            require(obj, "call_id", "str")
            require(obj, "name", "str")
            require(obj, "ok", "bool")
            if obj["call_id"] not in seen_calls:
                fail(f"line {i}: tool_result call_id without tool_call")
            if obj["ok"]:
                if "result" not in obj:
                    fail(f"line {i}: ok=true requires result")
            else:
                if "error" not in obj:
                    fail(f"line {i}: ok=false requires error")
        elif t == "error":
            require(obj, "message", "str")
        elif t == "result":
            require(obj, "status", "str")
        else:
            fail(f"line {i}: unknown type: {t}")


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    fixtures = repo / "docs" / "ai_agent" / "fixtures" / "stream-json-sample.jsonl"
    if not fixtures.exists():
        fail(f"missing fixture file: {fixtures}")

    lines = fixtures.read_text(encoding="utf-8").splitlines()
    validate_stream_json(lines)
    print("ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

