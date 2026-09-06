"""텍스트 → 음성. edge(온라인 뉴럴) 우선, 실패하면 sapi(윈도우 내장)로 폴백."""
from __future__ import annotations

import asyncio
import io
import re
import threading

from . import log

_CODE_FENCE = re.compile(r"```.*?```", re.DOTALL)
_INLINE_CODE = re.compile(r"`([^`]*)`")
_MARKDOWN = re.compile(r"[*_#>|]+")
_URL = re.compile(r"https?://\S+")


def spoken_form(text: str, max_chars: int) -> str:
    """화면용 텍스트를 '읽기 좋은' 형태로 다듬는다."""
    cleaned = _CODE_FENCE.sub(" 코드는 화면을 봐주세요. ", text)
    cleaned = _INLINE_CODE.sub(r"\1", cleaned)
    cleaned = _URL.sub(" 링크 ", cleaned)
    cleaned = _MARKDOWN.sub("", cleaned)
    cleaned = re.sub(r"\s+", " ", cleaned).strip()
    if max_chars > 0 and len(cleaned) > max_chars:
        cut = cleaned[:max_chars]
        # 문장 중간에서 끊기지 않도록 마지막 문장부호까지만
        boundary = max(cut.rfind("."), cut.rfind("!"), cut.rfind("?"), cut.rfind("다 "))
        if boundary > max_chars // 2:
            cut = cut[: boundary + 1]
        cleaned = cut + " 나머지는 화면에 적어두었습니다."
    return cleaned


class Speaker:
    def __init__(
        self,
        engine: str = "edge",
        voice: str = "ko-KR-SunHiNeural",
        rate: str = "+0%",
        sapi_rate: int = 190,
        max_chars: int = 700,
    ):
        self.engine = (engine or "off").lower()
        self.voice = voice
        self.rate = rate
        self.sapi_rate = sapi_rate
        self.max_chars = max_chars
        self._lock = threading.Lock()

    def speak(self, text: str) -> None:
        if self.engine == "off":
            return
        line = spoken_form(text, self.max_chars)
        if not line:
            return
        with self._lock:
            if self.engine == "edge":
                try:
                    self._speak_edge(line)
                    return
                except Exception as exc:  # noqa: BLE001 - 어떤 실패든 내장 음성으로 계속
                    log.warn(f"edge 음성 실패({exc}). 윈도우 내장 음성으로 전환합니다.")
                    self.engine = "sapi"
            self._speak_sapi(line)

    # --- 엔진별 구현 ---------------------------------------------------

    def _speak_edge(self, text: str) -> None:
        import edge_tts
        import numpy as np
        import sounddevice as sd
        import soundfile as sf

        async def synth() -> bytes:
            comm = edge_tts.Communicate(text, self.voice, rate=self.rate)
            buffer = bytearray()
            async for chunk in comm.stream():
                if chunk.get("type") == "audio" and chunk.get("data"):
                    buffer.extend(chunk["data"])
            return bytes(buffer)

        mp3 = asyncio.run(synth())
        if not mp3:
            raise RuntimeError("빈 오디오 응답")
        data, samplerate = sf.read(io.BytesIO(mp3), dtype="float32", always_2d=False)
        sd.play(np.asarray(data), samplerate)
        sd.wait()

    def _speak_sapi(self, text: str) -> None:
        try:
            import pyttsx3
        except ImportError:
            log.warn("pyttsx3 가 없어 음성 출력을 건너뜁니다.")
            return
        try:
            engine = pyttsx3.init()
            engine.setProperty("rate", self.sapi_rate)
            for candidate in engine.getProperty("voices"):
                blob = f"{getattr(candidate, 'id', '')} {getattr(candidate, 'name', '')}".lower()
                if "korean" in blob or "ko-kr" in blob or "heami" in blob:
                    engine.setProperty("voice", candidate.id)
                    break
            engine.say(text)
            engine.runAndWait()
            engine.stop()
        except Exception as exc:  # noqa: BLE001
            log.warn(f"음성 출력 실패: {exc}")
