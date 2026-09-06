"""도구 공용 기반 — 컨텍스트, 예외, 경로 안전 검사."""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable

from ..approval import Approver
from ..config import Config


# 허용 경로 안에 있더라도 절대 읽히면 안 되는 것들.
# 웹/메일 본문에 숨은 지시가 비서를 조종해 자격증명을 빼내는 경로를 막는다.
DENY_DIR_NAMES = {
    ".ssh", ".aws", ".gnupg", ".azure", ".kube", ".docker",
    "appdata", "secrets", ".config", ".git",
}
DENY_FILE_NAMES = {
    "config.toml", "token.json", "credentials.json", ".git-credentials",
    ".env", ".netrc", "id_rsa", "id_ed25519",
}
DENY_SUFFIXES = {".pem", ".key", ".pfx", ".p12", ".ppk", ".env", ".keystore"}


class ToolError(RuntimeError):
    """모델에게 그대로 전달되는 도구 실패. 크래시 대신 이걸 던진다."""


@dataclass
class Ctx:
    config: Config
    approver: Approver

    def allowed_roots(self) -> list[Path]:
        return self.config.allowed_roots

    def safe_path(self, raw: str, must_exist: bool = True) -> Path:
        """허용된 최상위 경로 안에 있는 경로만 통과시킨다."""
        if not raw or not str(raw).strip():
            raise ToolError("경로가 비어 있습니다.")
        candidate = Path(str(raw)).expanduser()
        try:
            resolved = candidate.resolve()
        except OSError as exc:
            raise ToolError(f"경로를 해석할 수 없습니다: {raw} ({exc})") from exc
        roots = self.allowed_roots()
        for root in roots:
            if resolved == root or root in resolved.parents:
                break
        else:
            allowed = ", ".join(str(r) for r in roots)
            raise ToolError(
                f"접근이 허용되지 않은 경로입니다: {resolved}\n허용 경로: {allowed}"
            )
        self._reject_sensitive(resolved)
        if must_exist and not resolved.exists():
            raise ToolError(f"존재하지 않는 경로입니다: {resolved}")
        return resolved

    @staticmethod
    def _reject_sensitive(resolved: Path) -> None:
        lowered_parts = [part.lower() for part in resolved.parts]
        for part in lowered_parts[:-1] if len(lowered_parts) > 1 else []:
            if part in DENY_DIR_NAMES:
                raise ToolError(f"비밀정보가 들어 있을 수 있는 위치라 접근을 막았습니다: {part}")
        name = resolved.name.lower()
        if name in DENY_DIR_NAMES or name in DENY_FILE_NAMES or resolved.suffix.lower() in DENY_SUFFIXES:
            raise ToolError(f"비밀정보일 수 있는 파일이라 접근을 막았습니다: {resolved.name}")


Handler = Callable[..., str]


@dataclass
class Tool:
    name: str
    description: str
    schema: dict[str, Any]
    handler: Handler

    def spec(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "description": self.description,
            "input_schema": self.schema,
        }


def obj(properties: dict[str, Any], required: list[str] | None = None) -> dict[str, Any]:
    return {
        "type": "object",
        "properties": properties,
        "required": required or [],
    }


def clip(text: str, limit: int = 6000) -> str:
    if len(text) <= limit:
        return text
    return text[:limit] + f"\n… (총 {len(text)}자 중 {limit}자만 표시)"
