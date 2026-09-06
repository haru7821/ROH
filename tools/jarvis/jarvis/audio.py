"""마이크 녹음 — 단축키를 누르는 동안 프레임을 모은다."""
from __future__ import annotations

import threading
import time

import numpy as np
import sounddevice as sd

SAMPLE_RATE = 16_000  # Whisper 가 기대하는 샘플레이트


def resolve_input_device(name_fragment: str | None):
    """장치 이름 일부로 입력 장치 인덱스를 찾는다. 못 찾으면 None(기본 장치)."""
    if not name_fragment:
        return None
    fragment = name_fragment.lower()
    for index, dev in enumerate(sd.query_devices()):
        if dev.get("max_input_channels", 0) > 0 and fragment in str(dev.get("name", "")).lower():
            return index
    return None


class Recorder:
    """스레드 안전한 녹음기. start() 후 stop() 이 모노 float32 배열을 돌려준다."""

    def __init__(self, device=None, samplerate: int = SAMPLE_RATE, max_seconds: float = 60.0):
        self.device = device
        self.samplerate = samplerate
        self.max_frames = int(samplerate * max_seconds)
        self.max_seconds = max_seconds
        self._frames: list[np.ndarray] = []
        self._frame_count = 0
        self._stream: sd.InputStream | None = None
        self._started_at: float | None = None
        self._lock = threading.Lock()          # 프레임 버퍼용
        self._stream_lock = threading.RLock()  # 스트림 생성/종료용

    @property
    def active(self) -> bool:
        return self._stream is not None

    @property
    def elapsed(self) -> float:
        started = self._started_at
        return 0.0 if started is None else time.monotonic() - started

    def _callback(self, indata, frames, time_info, status):  # noqa: ARG002
        # 사운드 콜백은 별도 스레드에서 돈다. 반드시 복사해서 보관할 것.
        with self._lock:
            if self._frame_count < self.max_frames:
                chunk = indata[:, 0].copy()
                self._frames.append(chunk)
                self._frame_count += len(chunk)

    def start(self) -> None:
        with self._stream_lock:
            if self._stream is not None:
                return
            with self._lock:
                self._frames = []
                self._frame_count = 0
            stream = sd.InputStream(
                samplerate=self.samplerate,
                channels=1,
                dtype="float32",
                device=self.device,
                callback=self._callback,
                blocksize=0,
            )
            # 스트림을 먼저 등록해야 start() 가 느릴 때 두 번 열리지 않는다.
            self._stream = stream
            self._started_at = time.monotonic()
            try:
                stream.start()
            except Exception:
                self._stream = None
                self._started_at = None
                stream.close()
                raise

    def stop(self) -> np.ndarray:
        with self._stream_lock:
            stream, self._stream = self._stream, None
            self._started_at = None
            if stream is not None:
                try:
                    stream.stop()
                finally:
                    stream.close()
        with self._lock:
            frames, self._frames = self._frames, []
            self._frame_count = 0
        if not frames:
            return np.zeros(0, dtype=np.float32)
        return np.concatenate(frames).astype(np.float32, copy=False)

    def abort(self) -> None:
        self.stop()

    def overran(self) -> bool:
        """max_seconds 를 넘겼는지. 뗌 이벤트를 놓쳐도 감시자가 끊을 수 있게 한다."""
        return self.active and self.elapsed > self.max_seconds
