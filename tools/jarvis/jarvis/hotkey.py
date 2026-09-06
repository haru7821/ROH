"""전역 단축키 — 누름/뗌을 각각 잡아 '눌러서 말하기'를 만든다."""
from __future__ import annotations

from typing import Callable

from pynput import keyboard

# pynput 이 좌/우 수정자를 일반 수정자로 바꾸지 못하는 경우를 대비한 보정표
_MODIFIER_ALIASES = {
    keyboard.Key.ctrl_l: keyboard.Key.ctrl,
    keyboard.Key.ctrl_r: keyboard.Key.ctrl,
    keyboard.Key.alt_l: keyboard.Key.alt,
    keyboard.Key.alt_r: keyboard.Key.alt,
    keyboard.Key.alt_gr: keyboard.Key.alt,
    keyboard.Key.shift_l: keyboard.Key.shift,
    keyboard.Key.shift_r: keyboard.Key.shift,
    keyboard.Key.cmd_l: keyboard.Key.cmd,
    keyboard.Key.cmd_r: keyboard.Key.cmd,
}


class _Combo:
    def __init__(self, spec: str, on_press: Callable[[], None] | None,
                 on_release: Callable[[], None] | None):
        self.spec = spec
        self.keys = set(keyboard.HotKey.parse(spec))
        self.on_press = on_press
        self.on_release = on_release
        self.engaged = False


class HotkeyManager:
    def __init__(self) -> None:
        self._combos: list[_Combo] = []
        self._pressed: set = set()
        self._listener: keyboard.Listener | None = None

    def add(self, spec: str, on_press=None, on_release=None) -> None:
        self._combos.append(_Combo(spec, on_press, on_release))

    def _canonical(self, key):
        if self._listener is not None:
            try:
                key = self._listener.canonical(key)
            except Exception:  # noqa: BLE001 - 캐노니컬 실패 시 원본 사용
                pass
        return _MODIFIER_ALIASES.get(key, key)

    def _on_press(self, key) -> None:
        resolved = self._canonical(key)
        self._pressed.add(resolved)
        for combo in self._combos:
            if not combo.engaged and combo.keys <= self._pressed:
                combo.engaged = True
                if combo.on_press:
                    combo.on_press()

    def _on_release(self, key) -> None:
        resolved = self._canonical(key)
        self._pressed.discard(resolved)
        for combo in self._combos:
            if combo.engaged and resolved in combo.keys:
                combo.engaged = False
                if combo.on_release:
                    combo.on_release()

    def start(self) -> None:
        self._listener = keyboard.Listener(
            on_press=self._on_press, on_release=self._on_release
        )
        self._listener.start()

    def stop(self) -> None:
        if self._listener is not None:
            self._listener.stop()
            self._listener = None
