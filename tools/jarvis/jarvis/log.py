"""콘솔 출력 — 색상은 윈도우 터미널 기준 ANSI."""
from __future__ import annotations

import os
import sys

def _enable_windows_ansi() -> bool:
    """구형 conhost 는 VT 처리가 꺼져 있어 색상 코드가 쓰레기 문자로 보인다."""
    if os.name != "nt":
        return True
    try:
        import ctypes

        kernel32 = ctypes.windll.kernel32
        handle = kernel32.GetStdHandle(-11)
        mode = ctypes.c_uint32()
        if not kernel32.GetConsoleMode(handle, ctypes.byref(mode)):
            return False
        return bool(kernel32.SetConsoleMode(handle, mode.value | 0x0004))
    except Exception:  # noqa: BLE001
        return False


_ENABLED = (
    os.environ.get("JARVIS_NO_COLOR") != "1"
    and sys.stdout.isatty()
    and _enable_windows_ansi()
)


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
