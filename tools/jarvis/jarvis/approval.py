"""위험한 동작 실행 전 사용자 확인."""
from __future__ import annotations

import time

from . import log

# 'a'(계속 허용)의 유효 시간. 무기한이면 한 번의 승인이 영구 백지수표가 된다.
BLANKET_ALLOW_SECONDS = 600.0


class Approver:
    def __init__(self, confirm_tools: set[str], speaker=None):
        self.confirm_tools = confirm_tools
        self.speaker = speaker
        self.session_allow: dict[str, float] = {}

    def needs_confirm(self, tool_name: str) -> bool:
        if tool_name not in self.confirm_tools:
            return False
        granted_until = self.session_allow.get(tool_name)
        if granted_until is not None and time.monotonic() < granted_until:
            return False
        self.session_allow.pop(tool_name, None)
        return True

    def ask(self, tool_name: str, description: str) -> bool:
        """y = 이번만 허용, a = 이번 실행 동안 계속 허용, 그 외 = 거부."""
        log.warn(f"확인 필요 — {tool_name}")
        print(f"    {description}")
        if self.speaker is not None:
            self.speaker.speak("확인이 필요합니다.")
        try:
            answer = input("    실행할까요? [y=예 / a=앞으로 계속 / n=아니오] ").strip().lower()
        except EOFError:
            return False
        if answer == "a":
            self.session_allow[tool_name] = time.monotonic() + BLANKET_ALLOW_SECONDS
            log.info(f"{int(BLANKET_ALLOW_SECONDS // 60)}분 동안 {tool_name} 확인을 생략합니다.")
            return True
        return answer in {"y", "yes", "ㅛ", "네"}
