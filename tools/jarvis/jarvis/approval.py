"""위험한 동작 실행 전 사용자 확인."""
from __future__ import annotations

from . import log


class Approver:
    def __init__(self, confirm_tools: set[str], speaker=None):
        self.confirm_tools = confirm_tools
        self.speaker = speaker
        self.session_allow: set[str] = set()

    def needs_confirm(self, tool_name: str) -> bool:
        return tool_name in self.confirm_tools and tool_name not in self.session_allow

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
            self.session_allow.add(tool_name)
            return True
        return answer in {"y", "yes", "ㅛ", "네"}
