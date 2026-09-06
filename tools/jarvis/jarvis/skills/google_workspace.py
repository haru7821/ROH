"""Gmail / 구글 캘린더. 최초 1회 브라우저 인증이 필요하다."""
from __future__ import annotations

import base64
import re
from datetime import datetime, timedelta
from email.message import EmailMessage
from pathlib import Path

from ..config import APP_DIR
from .base import Ctx, Tool, ToolError, clip, obj

SCOPES = [
    "https://www.googleapis.com/auth/gmail.modify",
    "https://www.googleapis.com/auth/calendar",
]

_SERVICES: dict[str, object] = {}
_TAG_RE = re.compile(r"<[^>]+>")


def _resolve(raw: str) -> Path:
    path = Path(str(raw)).expanduser()
    return path if path.is_absolute() else (APP_DIR / path)


def _credentials(ctx: Ctx):
    try:
        from google.auth.transport.requests import Request
        from google.oauth2.credentials import Credentials
        from google_auth_oauthlib.flow import InstalledAppFlow
    except ImportError as exc:
        raise ToolError(
            "구글 라이브러리가 없습니다. setup.bat 을 다시 실행하거나 "
            "pip install google-api-python-client google-auth-oauthlib 를 실행하세요."
        ) from exc

    token_path = _resolve(ctx.config.get("google.token_file", "secrets/token.json"))
    creds_path = _resolve(ctx.config.get("google.credentials_file", "secrets/credentials.json"))

    creds = None
    if token_path.exists():
        try:
            creds = Credentials.from_authorized_user_file(str(token_path), SCOPES)
        except ValueError:
            creds = None
    if creds and creds.valid:
        return creds
    if creds and creds.expired and creds.refresh_token:
        creds.refresh(Request())
    else:
        if not creds_path.exists():
            raise ToolError(
                f"구글 OAuth 클라이언트 파일이 없습니다: {creds_path}\n"
                "구글 클라우드 콘솔에서 '데스크톱 앱' 자격증명을 만들어 이 위치에 두세요."
            )
        flow = InstalledAppFlow.from_client_secrets_file(str(creds_path), SCOPES)
        creds = flow.run_local_server(port=0)
    token_path.parent.mkdir(parents=True, exist_ok=True)
    token_path.write_text(creds.to_json(), encoding="utf-8")
    return creds


def _service(ctx: Ctx, api: str, version: str):
    key = f"{api}:{version}"
    if key not in _SERVICES:
        try:
            from googleapiclient.discovery import build
        except ImportError as exc:
            raise ToolError("google-api-python-client 가 설치되어 있지 않습니다.") from exc
        _SERVICES[key] = build(api, version, credentials=_credentials(ctx), cache_discovery=False)
    return _SERVICES[key]


def _call(request):
    """구글 API 오류를 도구 오류로 바꿔 모델이 이해할 수 있게 한다."""
    try:
        from googleapiclient.errors import HttpError
    except ImportError:  # pragma: no cover
        return request.execute()
    try:
        return request.execute()
    except HttpError as exc:
        raise ToolError(f"구글 API 오류: {exc}") from exc


# --- Gmail -----------------------------------------------------------------

def _headers(message: dict) -> dict[str, str]:
    return {
        h.get("name", "").lower(): h.get("value", "")
        for h in message.get("payload", {}).get("headers", [])
    }


def _body_text(payload: dict) -> str:
    """본문에서 text/plain 을 찾고, 없으면 html 을 태그 제거해 쓴다."""
    stack = [payload]
    html_fallback = ""
    while stack:
        part = stack.pop(0)
        mime = part.get("mimeType", "")
        data = part.get("body", {}).get("data")
        if data:
            decoded = base64.urlsafe_b64decode(data.encode()).decode("utf-8", errors="replace")
            if mime == "text/plain":
                return decoded
            if mime == "text/html" and not html_fallback:
                html_fallback = _TAG_RE.sub(" ", decoded)
        stack.extend(part.get("parts", []) or [])
    return html_fallback


def gmail_search(ctx: Ctx, query: str = "in:inbox", max_results: int = 10) -> str:
    service = _service(ctx, "gmail", "v1")
    count = max(1, min(int(max_results), 25))
    listed = _call(service.users().messages().list(userId="me", q=query, maxResults=count))
    ids = [m["id"] for m in listed.get("messages", [])]
    if not ids:
        return f"'{query}' 에 해당하는 메일이 없습니다."
    lines = []
    for msg_id in ids:
        msg = _call(service.users().messages().get(
            userId="me", id=msg_id, format="metadata",
            metadataHeaders=["From", "Subject", "Date"],
        ))
        head = _headers(msg)
        lines.append(
            f"[{msg_id}] {head.get('date', '')}\n"
            f"  보낸이: {head.get('from', '(없음)')}\n"
            f"  제목: {head.get('subject', '(없음)')}\n"
            f"  요약: {msg.get('snippet', '').strip()}"
        )
    return clip("\n".join(lines))


def gmail_read(ctx: Ctx, message_id: str) -> str:
    service = _service(ctx, "gmail", "v1")
    msg = _call(service.users().messages().get(userId="me", id=message_id, format="full"))
    head = _headers(msg)
    body = _body_text(msg.get("payload", {})).strip()
    return clip(
        f"제목: {head.get('subject', '')}\n"
        f"보낸이: {head.get('from', '')}\n"
        f"받는이: {head.get('to', '')}\n"
        f"날짜: {head.get('date', '')}\n\n{body or '(본문 없음)'}",
        8000,
    )


def _raw_message(to: str, subject: str, body: str) -> dict:
    mail = EmailMessage()
    mail["To"] = to
    mail["Subject"] = subject
    mail.set_content(body)
    return {"raw": base64.urlsafe_b64encode(mail.as_bytes()).decode()}


def gmail_draft(ctx: Ctx, to: str, subject: str, body: str) -> str:
    service = _service(ctx, "gmail", "v1")
    draft = _call(service.users().drafts().create(
        userId="me", body={"message": _raw_message(to, subject, body)}
    ))
    return f"임시보관함에 초안을 만들었습니다. (id {draft.get('id')}) 받는이 {to} / 제목 '{subject}'"


def gmail_send(ctx: Ctx, to: str, subject: str, body: str) -> str:
    if ctx.approver.needs_confirm("gmail_send"):
        preview = body if len(body) <= 400 else body[:400] + "…"
        detail = f"받는이: {to}\n    제목: {subject}\n    본문: {preview}"
        if not ctx.approver.ask("gmail_send", detail):
            return "사용자가 발송을 거부했습니다. 초안으로 남기려면 gmail_draft 를 쓰세요."
    service = _service(ctx, "gmail", "v1")
    sent = _call(service.users().messages().send(userId="me", body=_raw_message(to, subject, body)))
    return f"메일을 보냈습니다. (id {sent.get('id')})"


# --- Calendar --------------------------------------------------------------

def _rfc3339(value: str | None, default: datetime) -> str:
    if not value:
        moment = default
    else:
        try:
            moment = datetime.fromisoformat(str(value).replace("Z", "+00:00"))
        except ValueError as exc:
            raise ToolError(f"시간 형식을 이해할 수 없습니다: {value} (예: 2026-09-07T14:00)") from exc
    if moment.tzinfo is None:
        moment = moment.astimezone()
    return moment.isoformat()


def calendar_list_events(ctx: Ctx, time_min: str | None = None,
                         time_max: str | None = None, max_results: int = 15) -> str:
    service = _service(ctx, "calendar", "v3")
    now = datetime.now().astimezone()
    start = _rfc3339(time_min, now)
    end = _rfc3339(time_max, now + timedelta(days=7))
    calendar_id = str(ctx.config.get("google.calendar_id", "primary"))
    events = _call(service.events().list(
        calendarId=calendar_id, timeMin=start, timeMax=end,
        maxResults=max(1, min(int(max_results), 50)),
        singleEvents=True, orderBy="startTime",
    )).get("items", [])
    if not events:
        return f"{start[:16]} ~ {end[:16]} 사이에 일정이 없습니다."
    lines = []
    for event in events:
        when = event.get("start", {}).get("dateTime") or event.get("start", {}).get("date", "")
        lines.append(f"- {when[:16].replace('T', ' ')} {event.get('summary', '(제목 없음)')}"
                     + (f" @ {event['location']}" if event.get("location") else ""))
    return clip("\n".join(lines))


def calendar_create_event(ctx: Ctx, summary: str, start: str, end: str | None = None,
                          description: str | None = None, location: str | None = None) -> str:
    start_iso = _rfc3339(start, datetime.now().astimezone())
    if end:
        end_iso = _rfc3339(end, datetime.now().astimezone())
    else:
        end_iso = (datetime.fromisoformat(start_iso) + timedelta(hours=1)).isoformat()
    if ctx.approver.needs_confirm("calendar_create_event"):
        detail = f"일정: {summary}\n    시작: {start_iso}\n    종료: {end_iso}"
        if not ctx.approver.ask("calendar_create_event", detail):
            return "사용자가 일정 등록을 거부했습니다."
    service = _service(ctx, "calendar", "v3")
    body = {
        "summary": summary,
        "start": {"dateTime": start_iso},
        "end": {"dateTime": end_iso},
    }
    if description:
        body["description"] = description
    if location:
        body["location"] = location
    created = _call(service.events().insert(
        calendarId=str(ctx.config.get("google.calendar_id", "primary")), body=body
    ))
    return f"일정을 등록했습니다: {summary} ({start_iso[:16].replace('T', ' ')})"


TOOLS = [
    Tool(
        "gmail_search",
        "Gmail 을 검색한다. query 는 Gmail 검색 문법을 그대로 쓴다 (예: 'is:unread', 'from:kim newer_than:3d').",
        obj({
            "query": {"type": "string", "description": "Gmail 검색어 (기본 'in:inbox')"},
            "max_results": {"type": "integer", "description": "최대 개수 (기본 10, 최대 25)"},
        }),
        gmail_search,
    ),
    Tool(
        "gmail_read",
        "메일 하나의 본문을 읽는다. gmail_search 로 얻은 id 를 넣는다.",
        obj({"message_id": {"type": "string", "description": "메일 id"}}, ["message_id"]),
        gmail_read,
    ),
    Tool(
        "gmail_draft",
        "메일 초안을 임시보관함에 만든다. 보내기 전에 사용자가 검토하게 하려면 이걸 쓴다.",
        obj({
            "to": {"type": "string", "description": "받는 사람 주소"},
            "subject": {"type": "string", "description": "제목"},
            "body": {"type": "string", "description": "본문"},
        }, ["to", "subject", "body"]),
        gmail_draft,
    ),
    Tool(
        "gmail_send",
        "메일을 실제로 보낸다. 사용자가 분명히 '보내'라고 했을 때만 쓸 것.",
        obj({
            "to": {"type": "string", "description": "받는 사람 주소"},
            "subject": {"type": "string", "description": "제목"},
            "body": {"type": "string", "description": "본문"},
        }, ["to", "subject", "body"]),
        gmail_send,
    ),
    Tool(
        "calendar_list_events",
        "구글 캘린더 일정을 조회한다. 기본은 지금부터 7일간.",
        obj({
            "time_min": {"type": "string", "description": "시작 시각 ISO (예: 2026-09-07T00:00)"},
            "time_max": {"type": "string", "description": "끝 시각 ISO"},
            "max_results": {"type": "integer", "description": "최대 개수 (기본 15)"},
        }),
        calendar_list_events,
    ),
    Tool(
        "calendar_create_event",
        "구글 캘린더에 일정을 등록한다.",
        obj({
            "summary": {"type": "string", "description": "일정 제목"},
            "start": {"type": "string", "description": "시작 시각 ISO (예: 2026-09-07T14:00)"},
            "end": {"type": "string", "description": "끝 시각 ISO (생략하면 1시간)"},
            "description": {"type": "string", "description": "설명 (선택)"},
            "location": {"type": "string", "description": "장소 (선택)"},
        }, ["summary", "start"]),
        calendar_create_event,
    ),
]
