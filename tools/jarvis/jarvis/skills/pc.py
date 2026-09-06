"""PC 제어 — 앱 실행, 파일 찾기/읽기, 명령 실행, 클립보드."""
from __future__ import annotations

import fnmatch
import os
import subprocess
import sys
import time
import webbrowser
from pathlib import Path

from .. import log
from .base import Ctx, Tool, ToolError, clip, obj

# 훑지 않을 디렉터리 (용량만 크고 찾을 게 없는 곳)
SKIP_DIRS = {
    ".git", "node_modules", "__pycache__", ".venv", "venv", "Binaries",
    "Intermediate", "Saved", "DerivedDataCache", "AppData", "$Recycle.Bin",
    "Windows", "System Volume Information", ".gradle", "build", "dist",
}
SEARCH_TIME_BUDGET = 12.0  # 초
MAX_READ_BYTES = 2_000_000
# 열기만 해도 코드가 실행되는 확장자 — 확인 없이 열지 않는다.
EXECUTABLE_SUFFIXES = {
    ".exe", ".bat", ".cmd", ".com", ".ps1", ".vbs", ".js", ".jse",
    ".msi", ".scr", ".lnk", ".reg", ".hta", ".wsf",
}


def _decode(raw: bytes) -> str:
    for encoding in ("utf-8", "cp949", "latin-1"):
        try:
            return raw.decode(encoding)
        except UnicodeDecodeError:
            continue
    return raw.decode("utf-8", errors="replace")


def _is_url(value: str) -> bool:
    return value.startswith(("http://", "https://"))


def _launch(target: str) -> None:
    """셸을 절대 거치지 않는다. shell=True 로 폴백하면 open_app 이
    확인 없는 임의 명령 실행 통로가 된다."""
    if _is_url(target):
        webbrowser.open(target)
        return
    path = Path(target).expanduser()
    if path.exists():
        target = str(path)
    starter = getattr(os, "startfile", None)
    if starter is not None:
        try:
            starter(target)  # type: ignore[misc]
            return
        except OSError:
            # PATH 에만 있는 실행 파일 (예: code). 셸 없이 그대로 띄운다.
            subprocess.Popen([target], shell=False)
        return
    opener = "open" if sys.platform == "darwin" else "xdg-open"
    subprocess.Popen([opener, target])


def open_app(ctx: Ctx, name: str) -> str:
    aliases = ctx.config.get("pc.apps", {}) or {}
    target = aliases.get(name)
    if target is None:
        # 별칭에 없으면 부분 일치로 한 번 더 찾아본다.
        lowered = name.strip().lower()
        for alias, value in aliases.items():
            if lowered and (lowered in alias.lower() or alias.lower() in lowered):
                target, name = value, alias
                break
    if target is None:
        # 등록되지 않은 이름은 URL 이나 실제 존재하는 파일일 때만 허용한다.
        # 그러지 않으면 임의 명령 문자열이 이 도구로 흘러들 수 있다.
        candidate = name.strip()
        if _is_url(candidate):
            target = candidate
        elif Path(candidate).expanduser().exists():
            target = str(ctx.safe_path(candidate))
        else:
            raise ToolError(
                f"'{name}' 은(는) 등록된 앱이 아닙니다. "
                "config.toml 의 [pc.apps] 에 추가하거나, 명령 실행이 필요하면 "
                "run_command 를 쓰세요."
            )
    try:
        _launch(str(target))
    except Exception as exc:  # noqa: BLE001
        raise ToolError(f"'{name}' 실행 실패: {exc}") from exc
    return f"'{name}' 을(를) 실행했습니다. (대상: {target})"


def open_path(ctx: Ctx, path: str) -> str:
    resolved = ctx.safe_path(path)
    if resolved.suffix.lower() in EXECUTABLE_SUFFIXES:
        if not ctx.approver.ask("open_path", f"실행 파일을 엽니다: {resolved}"):
            return "사용자가 실행을 거부했습니다."
    try:
        _launch(str(resolved))
    except Exception as exc:  # noqa: BLE001
        raise ToolError(f"열기 실패: {exc}") from exc
    return f"열었습니다: {resolved}"


def list_dir(ctx: Ctx, path: str, limit: int = 100) -> str:
    resolved = ctx.safe_path(path)
    if not resolved.is_dir():
        raise ToolError(f"디렉터리가 아닙니다: {resolved}")
    entries = []
    for index, entry in enumerate(sorted(resolved.iterdir(), key=lambda p: (p.is_file(), p.name.lower()))):
        if index >= limit:
            entries.append("…")
            break
        kind = "DIR " if entry.is_dir() else "FILE"
        try:
            size = entry.stat().st_size if entry.is_file() else 0
        except OSError:
            size = 0
        entries.append(f"{kind} {entry.name}" + (f" ({size:,}B)" if size else ""))
    return f"{resolved}\n" + ("\n".join(entries) if entries else "(비어 있음)")


def search_files(ctx: Ctx, query: str, root: str | None = None, limit: int = 40) -> str:
    if not query.strip():
        raise ToolError("검색어가 비어 있습니다.")
    roots = [ctx.safe_path(root)] if root else ctx.allowed_roots()
    pattern = f"*{query.strip().lower()}*"
    hits: list[str] = []
    started = time.monotonic()
    timed_out = False
    for base in roots:
        if not base.exists():
            continue
        for dirpath, dirnames, filenames in os.walk(base, onerror=lambda _e: None):
            if time.monotonic() - started > SEARCH_TIME_BUDGET:
                timed_out = True
                break
            dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS and not d.startswith(".")]
            for filename in filenames:
                if fnmatch.fnmatch(filename.lower(), pattern):
                    hits.append(str(Path(dirpath) / filename))
                    if len(hits) >= limit:
                        break
            if len(hits) >= limit:
                break
        if len(hits) >= limit or timed_out:
            break
    if not hits:
        return f"'{query}' 와(과) 일치하는 파일을 찾지 못했습니다."
    suffix = "\n(시간 제한으로 일부만 검색했습니다.)" if timed_out else ""
    return f"{len(hits)}건:\n" + "\n".join(hits) + suffix


def read_text_file(ctx: Ctx, path: str, max_chars: int = 6000) -> str:
    resolved = ctx.safe_path(path)
    if not resolved.is_file():
        raise ToolError(f"파일이 아닙니다: {resolved}")
    if resolved.stat().st_size > MAX_READ_BYTES:
        raise ToolError(f"파일이 너무 큽니다({resolved.stat().st_size:,}B). 일부만 필요한 경우 명령으로 잘라 읽으세요.")
    return clip(_decode(resolved.read_bytes()), max(500, int(max_chars)))


def run_command(ctx: Ctx, command: str, cwd: str | None = None, timeout: int = 120) -> str:
    if not command.strip():
        raise ToolError("명령이 비어 있습니다.")
    workdir = str(ctx.safe_path(cwd)) if cwd else None
    if ctx.approver.needs_confirm("run_command"):
        detail = f"명령: {command}" + (f"\n    위치: {workdir}" if workdir else "")
        if not ctx.approver.ask("run_command", detail):
            return "사용자가 실행을 거부했습니다."
    log.tool("run_command", command)
    try:
        proc = subprocess.run(
            command, shell=True, cwd=workdir, capture_output=True,
            stdin=subprocess.DEVNULL,  # pause / set /p 로 영원히 멈추는 것을 막는다
            timeout=max(1, int(timeout)),
        )
    except subprocess.TimeoutExpired:
        raise ToolError(f"{timeout}초 안에 끝나지 않아 중단했습니다.") from None
    out = _decode(proc.stdout) + (("\n[stderr]\n" + _decode(proc.stderr)) if proc.stderr else "")
    return f"종료 코드 {proc.returncode}\n{clip(out.strip() or '(출력 없음)')}"


def clipboard_get(ctx: Ctx) -> str:  # noqa: ARG001
    try:
        import pyperclip
    except ImportError as exc:
        raise ToolError("pyperclip 이 설치되어 있지 않습니다.") from exc
    return clip(pyperclip.paste() or "(클립보드가 비어 있습니다)", 4000)


def clipboard_set(ctx: Ctx, text: str) -> str:  # noqa: ARG001
    try:
        import pyperclip
    except ImportError as exc:
        raise ToolError("pyperclip 이 설치되어 있지 않습니다.") from exc
    pyperclip.copy(text)
    return f"클립보드에 복사했습니다 ({len(text)}자)."


TOOLS = [
    Tool(
        "open_app",
        "PC에서 프로그램이나 웹사이트를 실행한다. 설정의 별칭(예: '메모장', '크롬')을 쓸 수 있고, 없으면 이름 그대로 실행을 시도한다.",
        obj({"name": {"type": "string", "description": "실행할 앱 별칭 또는 실행 파일 이름"}}, ["name"]),
        open_app,
    ),
    Tool(
        "open_path",
        "파일이나 폴더를 윈도우 기본 프로그램으로 연다.",
        obj({"path": {"type": "string", "description": "열 파일/폴더의 경로"}}, ["path"]),
        open_path,
    ),
    Tool(
        "list_dir",
        "폴더 안의 파일 목록을 본다.",
        obj({
            "path": {"type": "string", "description": "폴더 경로"},
            "limit": {"type": "integer", "description": "최대 항목 수 (기본 100)"},
        }, ["path"]),
        list_dir,
    ),
    Tool(
        "search_files",
        "이름에 특정 문자열이 들어간 파일을 허용된 경로 안에서 찾는다.",
        obj({
            "query": {"type": "string", "description": "파일 이름에 포함될 문자열"},
            "root": {"type": "string", "description": "검색을 시작할 폴더 (생략하면 허용된 전체 경로)"},
            "limit": {"type": "integer", "description": "최대 결과 수 (기본 40)"},
        }, ["query"]),
        search_files,
    ),
    Tool(
        "read_text_file",
        "텍스트 파일의 내용을 읽는다.",
        obj({
            "path": {"type": "string", "description": "파일 경로"},
            "max_chars": {"type": "integer", "description": "가져올 최대 글자 수 (기본 6000)"},
        }, ["path"]),
        read_text_file,
    ),
    Tool(
        "run_command",
        "윈도우 명령을 실행한다. 되돌리기 어려운 작업(삭제/설치/네트워크 전송)은 반드시 사용자에게 먼저 무엇을 할지 말로 설명하고 나서 호출할 것.",
        obj({
            "command": {"type": "string", "description": "실행할 명령줄"},
            "cwd": {"type": "string", "description": "실행 위치 (선택)"},
            "timeout": {"type": "integer", "description": "제한 시간(초), 기본 120"},
        }, ["command"]),
        run_command,
    ),
    Tool(
        "clipboard_get",
        "현재 클립보드 내용을 읽는다.",
        obj({}),
        clipboard_get,
    ),
    Tool(
        "clipboard_set",
        "클립보드에 텍스트를 넣는다.",
        obj({"text": {"type": "string", "description": "복사할 텍스트"}}, ["text"]),
        clipboard_set,
    ),
]
