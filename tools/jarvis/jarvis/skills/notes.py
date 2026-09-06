"""메모 / 장기 기억 — 세션이 끝나도 남는 개인 노트."""
from __future__ import annotations

import json
import time
from datetime import datetime
from pathlib import Path

from .. import log
from ..config import DATA_DIR
from .base import Ctx, Tool, ToolError, clip, obj

NOTES_FILE = DATA_DIR / "notes.json"


def _load() -> list[dict]:
    if not NOTES_FILE.exists():
        return []
    try:
        data = json.loads(NOTES_FILE.read_text(encoding="utf-8"))
    except OSError as exc:
        log.warn(f"메모 파일을 읽지 못했습니다: {exc}")
        return []
    except json.JSONDecodeError:
        # 빈 목록을 돌려주면 다음 저장이 원본을 덮어써 기억이 통째로 사라진다.
        backup = NOTES_FILE.with_name(f"notes.corrupt-{int(time.time())}.json")
        try:
            NOTES_FILE.rename(backup)
            log.warn(f"메모 파일이 손상되어 {backup.name} 으로 보존했습니다.")
        except OSError:
            log.error("메모 파일이 손상되었고 백업도 실패했습니다. 수동 확인이 필요합니다.")
        return []
    return data if isinstance(data, list) else []


def _save(notes: list[dict]) -> None:
    NOTES_FILE.parent.mkdir(parents=True, exist_ok=True)
    tmp = NOTES_FILE.with_suffix(".json.tmp")
    tmp.write_text(json.dumps(notes, ensure_ascii=False, indent=2), encoding="utf-8")
    tmp.replace(NOTES_FILE)


def note_add(ctx: Ctx, text: str, tags: list[str] | None = None) -> str:  # noqa: ARG001
    if not text.strip():
        raise ToolError("저장할 내용이 비어 있습니다.")
    notes = _load()
    note = {
        "id": int(time.time() * 1000),
        "text": text.strip(),
        "tags": [str(t) for t in (tags or [])],
        "created": datetime.now().isoformat(timespec="seconds"),
    }
    notes.append(note)
    _save(notes)
    return f"기억했습니다. (id {note['id']})"


def note_search(ctx: Ctx, query: str, limit: int = 20) -> str:  # noqa: ARG001
    needle = query.strip().lower()
    notes = _load()
    hits = [
        n for n in notes
        if needle in n.get("text", "").lower()
        or any(needle in str(t).lower() for t in n.get("tags", []))
    ]
    if not hits:
        return f"'{query}' 로 저장된 메모가 없습니다."
    hits = hits[-max(1, int(limit)):]
    lines = [f"[{n['id']}] {n['created']} {n['text']}" for n in hits]
    return clip("\n".join(lines))


def note_list(ctx: Ctx, limit: int = 20) -> str:  # noqa: ARG001
    notes = _load()[-max(1, int(limit)):]
    if not notes:
        return "저장된 메모가 없습니다."
    return clip("\n".join(f"[{n['id']}] {n['created']} {n['text']}" for n in notes))


def note_delete(ctx: Ctx, note_id: int) -> str:  # noqa: ARG001
    notes = _load()
    remaining = [n for n in notes if int(n.get("id", 0)) != int(note_id)]
    if len(remaining) == len(notes):
        raise ToolError(f"id {note_id} 인 메모가 없습니다.")
    _save(remaining)
    return f"메모 {note_id} 를 지웠습니다."


TOOLS = [
    Tool(
        "note_add",
        "사용자가 기억해 달라고 한 내용이나 나중에 필요할 정보를 저장한다. 취향·습관·반복 일정처럼 다음 대화에서도 쓸 사실은 적극적으로 저장할 것.",
        obj({
            "text": {"type": "string", "description": "저장할 내용"},
            "tags": {"type": "array", "items": {"type": "string"}, "description": "분류 태그 (선택)"},
        }, ["text"]),
        note_add,
    ),
    Tool(
        "note_search",
        "저장해 둔 메모를 검색한다. 사용자가 예전에 말한 내용을 물으면 먼저 여기를 찾아본다.",
        obj({
            "query": {"type": "string", "description": "검색어"},
            "limit": {"type": "integer", "description": "최대 개수 (기본 20)"},
        }, ["query"]),
        note_search,
    ),
    Tool(
        "note_list",
        "최근 메모를 순서대로 본다.",
        obj({"limit": {"type": "integer", "description": "최대 개수 (기본 20)"}}),
        note_list,
    ),
    Tool(
        "note_delete",
        "메모를 id로 지운다.",
        obj({"note_id": {"type": "integer", "description": "지울 메모의 id"}}, ["note_id"]),
        note_delete,
    ),
]
