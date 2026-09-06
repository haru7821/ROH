"""config.toml 로드 및 접근."""
from __future__ import annotations

import os
import tomllib
from pathlib import Path
from typing import Any

PACKAGE_DIR = Path(__file__).resolve().parent
APP_DIR = PACKAGE_DIR.parent          # tools/jarvis
DATA_DIR = APP_DIR / "data"


class ConfigError(RuntimeError):
    pass


class Config:
    def __init__(self, data: dict[str, Any], path: Path):
        self._data = data
        self.path = path

    @classmethod
    def load(cls, path: Path | None = None) -> "Config":
        path = path or (APP_DIR / "config.toml")
        if not path.exists():
            example = APP_DIR / "config.example.toml"
            raise ConfigError(
                f"설정 파일이 없습니다: {path}\n"
                f"  {example.name} 을 config.toml 로 복사한 뒤 값을 채워주세요."
            )
        with path.open("rb") as fh:
            data = tomllib.load(fh)
        return cls(data, path)

    def get(self, dotted: str, default: Any = None) -> Any:
        """'stt.model_size' 같은 점 표기로 값을 읽는다."""
        node: Any = self._data
        for part in dotted.split("."):
            if not isinstance(node, dict) or part not in node:
                return default
            node = node[part]
        return node

    # --- 자주 쓰는 값들 -------------------------------------------------

    @property
    def api_key(self) -> str | None:
        # 설정 파일보다 환경변수를 우선한다(키를 파일에 안 두는 쪽이 안전).
        return os.environ.get("ANTHROPIC_API_KEY") or (self.get("api.api_key") or None)

    @property
    def allowed_roots(self) -> list[Path]:
        roots: list[Path] = []
        for raw in self.get("safety.allowed_roots", []) or []:
            roots.append(Path(str(raw)).expanduser())
        if not roots:
            roots.append(Path.home())
            repo = self.get("dev.repo_path")
            if repo:
                roots.append(Path(str(repo)).expanduser())
        resolved: list[Path] = []
        for root in roots:
            try:
                resolved.append(root.resolve())
            except OSError:
                continue
        return resolved

    @property
    def confirm_tools(self) -> set[str]:
        return {str(name) for name in (self.get("safety.confirm_tools", []) or [])}

    def enabled_skills(self) -> set[str]:
        block = self.get("skills", {}) or {}
        return {name for name, on in block.items() if on}
