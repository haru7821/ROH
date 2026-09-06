"""Claude 대화 루프 — 도구 호출을 직접 돌린다(수동 에이전트 루프)."""
from __future__ import annotations

from datetime import datetime
from typing import Any

import anthropic

from . import log
from .skills import build_registry
from .skills.base import Ctx, ToolError

SYSTEM_PROMPT = """\
당신은 사용자의 개인 비서 '{name}' 입니다. 사용자를 '{address}'라고 부릅니다.
답변은 음성으로 읽히므로 다음을 지키세요.

말투와 형식
- 한국어 존댓말, 간결하게. 기본 1~3문장.
- 목록이 꼭 필요하면 3개 이하로. 마크다운 기호·이모지·표는 쓰지 않습니다.
- 코드나 긴 자료는 "화면에 띄웠습니다" 라고만 말하고 본문에 짧게 덧붙입니다.
- 모르면 모른다고 말합니다. 추측을 사실처럼 말하지 않습니다.

도구 사용
- 필요하면 먼저 도구로 사실을 확인하고 답합니다. 짐작으로 답하지 않습니다.
- 여러 정보를 동시에 확인할 수 있으면 한 번에 여러 도구를 호출합니다.
- 파일 경로, 메일 주소, 시각처럼 틀리면 곤란한 값은 도구 결과에서 확인한 것만 씁니다.
- 사용자가 기억해 달라고 하거나 다음에도 쓸 사실은 note_add 로 저장합니다.
- 예전에 한 이야기를 물으면 note_search 를 먼저 확인합니다.

안전
- 삭제, 설치, 전송, 결제처럼 되돌리기 어려운 일은 실행 전에 무엇을 할지 한 문장으로 말합니다.
- 확인 절차에서 사용자가 거부하면 그대로 멈추고 다른 방법을 제안합니다.
- 사용자가 요청하지 않은 파일 변경이나 메일 발송은 하지 않습니다.
"""

MAX_RESTARTS = 4  # 서버측 도구가 pause_turn 으로 멈췄을 때 재개 횟수 상한


class Agent:
    def __init__(self, config, approver):
        self.config = config
        self.ctx = Ctx(config=config, approver=approver)
        self.model = str(config.get("api.model", "claude-opus-5"))
        self.effort = str(config.get("api.effort", "medium"))
        self.max_tokens = int(config.get("api.max_tokens", 8000))
        self.max_iterations = int(config.get("api.max_tool_iterations", 12))
        self.history_turns = int(config.get("assistant.history_turns", 12))

        self.tool_specs, self.handlers = build_registry(config)
        self.system = SYSTEM_PROMPT.format(
            name=config.get("assistant.name", "자비스"),
            address=config.get("assistant.address", "보스"),
        )
        self.client = anthropic.Anthropic(api_key=config.api_key)
        self.messages: list[dict[str, Any]] = []

        # SDK/모델이 특정 파라미터를 모르면 한 단계씩 낮춰서 계속 동작하게 한다.
        self._use_fallbacks = True
        self._use_effort = True
        self._use_thinking = True

    # --- 공개 API -------------------------------------------------------

    def reset(self) -> None:
        self.messages = []

    def tool_names(self) -> list[str]:
        return [spec["name"] for spec in self.tool_specs]

    def turn(self, user_text: str) -> str:
        stamp = datetime.now().astimezone().strftime("%Y-%m-%d %H:%M (%a)")
        self.messages.append({"role": "user", "content": f"[현재 시각 {stamp}]\n{user_text}"})
        self._trim()

        restarts = 0
        for _ in range(self.max_iterations):
            response = self._request()

            if response.stop_reason == "refusal":
                self.messages.append({"role": "assistant", "content": response.content})
                return "죄송합니다. 그 요청은 처리할 수 없습니다."

            self.messages.append({"role": "assistant", "content": response.content})

            if response.stop_reason == "pause_turn":
                restarts += 1
                if restarts > MAX_RESTARTS:
                    return self._text_of(response) or "작업이 너무 길어져 중단했습니다."
                continue

            tool_uses = [b for b in response.content if getattr(b, "type", None) == "tool_use"]
            if response.stop_reason != "tool_use" or not tool_uses:
                return self._text_of(response)

            results = [self._execute(block) for block in tool_uses]
            self.messages.append({"role": "user", "content": results})

        return "도구를 너무 여러 번 호출해 중단했습니다. 요청을 조금 나눠서 말씀해 주세요."

    # --- 내부 ----------------------------------------------------------

    def _request(self):
        kwargs: dict[str, Any] = {
            "model": self.model,
            "max_tokens": self.max_tokens,
            "system": [{"type": "text", "text": self.system, "cache_control": {"type": "ephemeral"}}],
            "messages": self.messages,
            "tools": self.tool_specs,
        }
        if self._use_thinking:
            kwargs["thinking"] = {"type": "adaptive"}
        if self._use_effort:
            kwargs["output_config"] = {"effort": self.effort}

        while True:
            try:
                if self._use_fallbacks:
                    return self.client.beta.messages.create(
                        betas=["server-side-fallback-2026-07-01"],
                        fallbacks="default",
                        **kwargs,
                    )
                return self.client.messages.create(**kwargs)
            except (TypeError, anthropic.BadRequestError) as exc:
                if not self._degrade(kwargs, exc):
                    raise

    def _degrade(self, kwargs: dict[str, Any], exc: Exception) -> bool:
        """지원되지 않는 파라미터를 하나씩 떼어내며 재시도한다."""
        if self._use_fallbacks:
            self._use_fallbacks = False
            log.info(f"서버측 폴백 미지원 — 일반 요청으로 전환합니다. ({type(exc).__name__})")
            return True
        if self._use_effort:
            self._use_effort = False
            kwargs.pop("output_config", None)
            log.info("effort 설정 미지원 — 기본값으로 전환합니다.")
            return True
        if self._use_thinking:
            self._use_thinking = False
            kwargs.pop("thinking", None)
            log.info("thinking 설정 미지원 — 기본값으로 전환합니다.")
            return True
        return False

    def _execute(self, block) -> dict[str, Any]:
        name = block.name
        params = dict(block.input or {})
        tool = self.handlers.get(name)
        if tool is None:
            log.error(f"알 수 없는 도구: {name}")
            return {
                "type": "tool_result",
                "tool_use_id": block.id,
                "content": f"'{name}' 도구는 사용할 수 없습니다.",
                "is_error": True,
            }

        allowed = set(tool.schema.get("properties", {}))
        filtered = {k: v for k, v in params.items() if k in allowed}
        log.tool(name, ", ".join(f"{k}={v!r}"[:70] for k, v in filtered.items()))

        try:
            output = tool.handler(self.ctx, **filtered)
            return {"type": "tool_result", "tool_use_id": block.id, "content": str(output)}
        except ToolError as exc:
            log.warn(f"{name}: {exc}")
            return {
                "type": "tool_result", "tool_use_id": block.id,
                "content": str(exc), "is_error": True,
            }
        except Exception as exc:  # noqa: BLE001 - 도구 오류로 비서가 죽지 않게
            log.error(f"{name} 실행 중 오류: {exc!r}")
            return {
                "type": "tool_result", "tool_use_id": block.id,
                "content": f"도구 실행 중 오류가 발생했습니다: {exc}", "is_error": True,
            }

    @staticmethod
    def _text_of(response) -> str:
        parts = [b.text for b in response.content if getattr(b, "type", None) == "text"]
        return "\n".join(part.strip() for part in parts if part.strip()).strip()

    def _trim(self) -> None:
        """오래된 대화를 버린다. tool_use/tool_result 짝이 깨지지 않도록
        '새 사용자 발화'(문자열 content) 경계에서만 자른다."""
        starts = [
            i for i, m in enumerate(self.messages)
            if m["role"] == "user" and isinstance(m.get("content"), str)
        ]
        if len(starts) <= self.history_turns:
            return
        cut = starts[-self.history_turns]
        self.messages = self.messages[cut:]
