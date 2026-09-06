"""개발 비서 — ROH 저장소 상태 확인, 코드 검색, 빌드."""
from __future__ import annotations

import subprocess
from pathlib import Path

from .. import log
from .base import Ctx, Tool, ToolError, clip, obj
from .pc import _decode

GIT_TIMEOUT = 60


def _repo(ctx: Ctx) -> Path:
    raw = ctx.config.get("dev.repo_path")
    if not raw:
        raise ToolError("설정에 dev.repo_path 가 없습니다.")
    path = Path(str(raw)).expanduser()
    if not path.exists():
        raise ToolError(f"저장소 경로가 존재하지 않습니다: {path}")
    return path


def _git(ctx: Ctx, args: list[str], timeout: int = GIT_TIMEOUT) -> str:
    repo = _repo(ctx)
    try:
        proc = subprocess.run(
            ["git", *args], cwd=str(repo), capture_output=True, timeout=timeout
        )
    except FileNotFoundError as exc:
        raise ToolError("git 을 찾을 수 없습니다. PATH 를 확인하세요.") from exc
    except subprocess.TimeoutExpired:
        raise ToolError("git 명령이 시간 안에 끝나지 않았습니다.") from None
    if proc.returncode != 0:
        raise ToolError(f"git {' '.join(args)} 실패:\n{_decode(proc.stderr).strip()}")
    return _decode(proc.stdout)


def git_status(ctx: Ctx) -> str:
    branch = _git(ctx, ["rev-parse", "--abbrev-ref", "HEAD"]).strip()
    status = _git(ctx, ["status", "--short", "--branch"]).strip()
    return f"브랜치: {branch}\n{clip(status or '(변경 없음)')}"


def git_log(ctx: Ctx, count: int = 10) -> str:
    count = max(1, min(int(count), 50))
    out = _git(ctx, ["log", f"-{count}", "--pretty=format:%h %ad %s", "--date=short"])
    return clip(out.strip() or "(커밋 없음)")


def git_diff(ctx: Ctx, path: str | None = None, staged: bool = False) -> str:
    args = ["diff"]
    if staged:
        args.append("--cached")
    args.append("--stat")
    stat = _git(ctx, args)
    detail_args = ["diff"] + (["--cached"] if staged else [])
    if path:
        detail_args += ["--", path]
    detail = _git(ctx, detail_args)
    return clip(f"{stat.strip()}\n\n{detail.strip()}" if detail.strip() else stat.strip() or "(차이 없음)")


def repo_search(ctx: Ctx, pattern: str, glob: str | None = None, limit: int = 60) -> str:
    if not pattern.strip():
        raise ToolError("검색어가 비어 있습니다.")
    args = ["grep", "-n", "-I", "--untracked", "-e", pattern]
    if glob:
        args += ["--", glob]
    try:
        out = _git(ctx, args)
    except ToolError as exc:
        # git grep 은 결과가 없으면 종료 코드 1 을 낸다.
        if "실패" in str(exc) and "fatal" not in str(exc):
            return f"'{pattern}' 검색 결과가 없습니다."
        raise
    lines = out.splitlines()[: max(1, int(limit))]
    return clip("\n".join(lines) or f"'{pattern}' 검색 결과가 없습니다.")


def build_project(ctx: Ctx) -> str:
    repo = _repo(ctx)
    command = str(ctx.config.get("dev.build_command", "update.bat"))
    timeout = int(ctx.config.get("dev.build_timeout_seconds", 3600))
    if ctx.approver.needs_confirm("build_project"):
        if not ctx.approver.ask("build_project", f"빌드 실행: {command} (위치 {repo})"):
            return "사용자가 빌드를 거부했습니다."
    log.tool("build_project", f"{command} @ {repo}")
    try:
        proc = subprocess.run(
            command, shell=True, cwd=str(repo), capture_output=True, timeout=timeout
        )
    except subprocess.TimeoutExpired:
        raise ToolError(f"빌드가 {timeout}초를 넘겨 중단했습니다.") from None
    out = _decode(proc.stdout) + (("\n[stderr]\n" + _decode(proc.stderr)) if proc.stderr else "")
    tail = "\n".join(out.strip().splitlines()[-60:])
    verdict = "성공" if proc.returncode == 0 else f"실패 (코드 {proc.returncode})"
    return f"빌드 {verdict}\n--- 마지막 출력 ---\n{clip(tail)}"


TOOLS = [
    Tool("git_status", "ROH 저장소의 현재 브랜치와 변경 사항을 본다.", obj({}), git_status),
    Tool(
        "git_log",
        "최근 커밋 목록을 본다.",
        obj({"count": {"type": "integer", "description": "가져올 커밋 수 (기본 10)"}}),
        git_log,
    ),
    Tool(
        "git_diff",
        "작업 중인 변경 내용을 본다.",
        obj({
            "path": {"type": "string", "description": "특정 파일만 볼 때의 경로 (선택)"},
            "staged": {"type": "boolean", "description": "스테이징된 변경만 볼지 (기본 false)"},
        }),
        git_diff,
    ),
    Tool(
        "repo_search",
        "ROH 소스에서 문자열/정규식을 검색한다 (git grep).",
        obj({
            "pattern": {"type": "string", "description": "찾을 패턴"},
            "glob": {"type": "string", "description": "대상 경로 패턴 (예: Source/**/*.cpp)"},
            "limit": {"type": "integer", "description": "최대 줄 수 (기본 60)"},
        }, ["pattern"]),
        repo_search,
    ),
    Tool(
        "build_project",
        "ROH 프로젝트를 빌드한다(update.bat). 오래 걸리므로 사용자가 명시적으로 요청했을 때만 쓸 것.",
        obj({}),
        build_project,
    ),
]
