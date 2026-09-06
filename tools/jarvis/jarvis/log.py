"""콘솔 출력 — 색상은 윈도우 터미널 기준 ANSI."""
from __future__ import annotations

import os
import sys

_ENABLED = os.environ.get("JARVIS_NO_COLOR") != "1" and sys.stdout.isatty()


def _c(code: str, text: str) -> str:
    return f"\033[{code}m{text}\033[0m" if _ENABLED else text


def banner(text: str) -> None:
    print(_c("96;1", text))


def info(text: str) -> None:
    print(_c("90", text))


def user(text: str) -> None:
    print(_c("93", f"\n[나] {text}"))


def assistant(text: str) -> None:
    print(_c("92", f"[자비스] {text}\n"))


def tool(name: str, detail: str = "") -> None:
    line = f"  · {name}" + (f" — {detail}" if detail else "")
    print(_c("36", line))


def warn(text: str) -> None:
    print(_c("33", f"[!] {text}"))


def error(text: str) -> None:
    print(_c("91", f"[X] {text}"))
