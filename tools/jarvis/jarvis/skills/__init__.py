"""사용 가능한 도구를 설정에 따라 모아준다."""
from __future__ import annotations

from typing import Any

from ..config import Config
from .base import Ctx, Tool, ToolError

# Anthropic 서버에서 실행되는 도구 — 우리 쪽 구현이 필요 없다.
SERVER_TOOLS: list[dict[str, Any]] = [
    {"type": "web_search_20260209", "name": "web_search", "max_uses": 8},
    {"type": "web_fetch_20260209", "name": "web_fetch", "max_uses": 5},
]


def build_registry(config: Config) -> tuple[list[dict[str, Any]], dict[str, Tool]]:
    enabled = config.enabled_skills()
    tools: list[Tool] = []

    if "pc" in enabled:
        from . import pc
        tools += pc.TOOLS
    if "notes" in enabled:
        from . import notes
        tools += notes.TOOLS
    if "dev" in enabled:
        from . import dev
        tools += dev.TOOLS
    if "google" in enabled:
        from . import google_workspace
        tools += google_workspace.TOOLS

    specs: list[dict[str, Any]] = [tool.spec() for tool in tools]
    if "web" in enabled:
        specs += SERVER_TOOLS
    return specs, {tool.name: tool for tool in tools}


__all__ = ["build_registry", "Ctx", "Tool", "ToolError", "SERVER_TOOLS"]
