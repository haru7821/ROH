"""음성 → 텍스트 (faster-whisper, 로컬 실행이라 API 키가 필요 없다)."""
from __future__ import annotations

import numpy as np

from . import log


class Transcriber:
    def __init__(self, model_size: str, device: str, compute_type: str, language: str | None):
        self.model_size = model_size
        self.device = device
        self.compute_type = compute_type
        self.language = language or None
        self._model = None

    def load(self) -> None:
        """모델을 미리 올려둔다(첫 인식 지연을 없애기 위해 시작 시 호출)."""
        if self._model is not None:
            return
        try:
            from faster_whisper import WhisperModel
        except ImportError as exc:  # pragma: no cover - 설치 안내용
            raise RuntimeError(
                "faster-whisper 가 설치되어 있지 않습니다. setup.bat 을 실행하세요."
            ) from exc
        log.info(f"음성 인식 모델 로드 중… ({self.model_size}/{self.device})")
        self._model = WhisperModel(
            self.model_size, device=self.device, compute_type=self.compute_type
        )

    def transcribe(self, audio: np.ndarray) -> str:
        if audio.size == 0:
            return ""
        self.load()
        assert self._model is not None
        segments, _info = self._model.transcribe(
            audio,
            language=self.language,
            beam_size=5,
            vad_filter=True,
            condition_on_previous_text=False,
        )
        return " ".join(segment.text.strip() for segment in segments).strip()
