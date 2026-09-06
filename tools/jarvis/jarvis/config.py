"""config.toml 로드 및 접근."""
from __future__ import annotations

import os
import tomllib
from pathlib import Path
from typing import Any

PACKAGE_DIR = Path(__file__).resolve().parent
APP_DIR = PACKAGE_DIR.parent          # tools/jarvis
DATA_DIR = APP_DIR / "data"

# safety.confirm_tools 키가 없을 때 적용되는 기본 확인 대상
DEFAULT_CONFIRM_TOOLS = frozenset(
    {"run_command", "build_project", "gmail_send", "calendar_create_event"}
)
# safety.allowed_roots 를 비워뒀을 때 홈에서 열어줄 하위 폴더
DEFAULT_HOME_SUBDIRS = ("Documents", "Downloads", "Desktop", "문서", "다운로드", "바탕 화면")


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
        try:
            with path.open("rb") as fh:
                data = tomllib.load(fh)
        except tomllib.TOMLDecodeError as exc:
            raise ConfigError(
                f"config.toml 문법 오류입니다: {exc}\n"
                "  값은 따옴표로 감싸야 합니다. 예: repo_path = \"C:/Projects/ROH\""
            ) from exc
        except UnicodeDecodeError as exc:
            raise ConfigError(
                "config.toml 을 UTF-8 로 읽을 수 없습니다.\n"
                "  메모장에서 열어 '다른 이름으로 저장 → 인코딩: UTF-8' 로 다시 저장하세요."
            ) from exc
        except OSError as exc:
            raise ConfigError(f"config.toml 을 읽을 수 없습니다: {exc}") from exc
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
            # 홈 전체를 열면 .ssh/AppData 까지 노출된다. 실제로 쓰는 곳만 연다.
            home = Path.home()
            for name in DEFAULT_HOME_SUBDIRS:
                candidate = home / name
                if candidate.exists():
                    roots.append(candidate)
            repo = self.get("dev.repo_path")
            if repo:
                roots.append(Path(str(repo)).expanduser())
            if not roots:
                roots.append(home)
        resolved: list[Path] = []
        for root in roots:
            try:
                resolved.append(root.resolve())
            except OSError:
                continue
        return resolved

    @property
    def confirm_tools(self) -> set[str]:
        configured = self.get("safety.confirm_tools", None)
        # 키가 아예 없으면(지웠거나 오타) 안전장치가 풀리면 안 되므로 기본값을 쓴다.
        if configured is None:
            return set(DEFAULT_CONFIRM_TOOLS)
        return {str(name) for name in configured}

    def enabled_skills(self) -> set[str]:
        block = self.get("skills", {}) or {}
        return {name for name, on in block.items() if on}
