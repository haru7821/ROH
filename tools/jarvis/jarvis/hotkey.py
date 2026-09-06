"""전역 단축키 — 누름/뗌을 각각 잡아 '눌러서 말하기'를 만든다."""
from __future__ import annotations

import os
from typing import Callable

from pynput import keyboard

from . import log

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


# 윈도우에서 실제 키가 눌려 있는지 재확인하기 위한 가상 키 코드
_VK_BY_KEY = {
    keyboard.Key.ctrl: 0x11,
    keyboard.Key.alt: 0x12,
    keyboard.Key.shift: 0x10,
    keyboard.Key.cmd: 0x5B,
}


def _physically_down(key) -> bool | None:
    """None = 확인 불가(윈도우가 아니거나 조회 실패)."""
    vk = _VK_BY_KEY.get(key)
    if vk is None or os.name != "nt":
        return None
    try:
        import ctypes

        return bool(ctypes.windll.user32.GetAsyncKeyState(vk) & 0x8000)
    except Exception:  # noqa: BLE001
        return None


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

    def _drop_stale_modifiers(self) -> None:
        """창 전환·UAC 등으로 뗌 이벤트를 놓쳐 남아 있는 수정자를 털어낸다.
        이게 없으면 'ctrl 이 눌린 것으로 착각한 상태에서 q 를 타이핑' 만으로
        종료 단축키가 발동한다."""
        for key in list(self._pressed):
            if _physically_down(key) is False:
                self._pressed.discard(key)

    def _fire(self, callback: Callable[[], None] | None) -> None:
        """콜백 예외가 pynput 리스너 스레드로 새면 단축키가 조용히 죽는다."""
        if callback is None:
            return
        try:
            callback()
        except Exception as exc:  # noqa: BLE001
            log.error(f"단축키 처리 중 오류: {exc!r}")

    def _on_press(self, key) -> None:
        try:
            resolved = self._canonical(key)
            self._pressed.add(resolved)
            self._drop_stale_modifiers()
            fired = [
                combo for combo in self._combos
                if not combo.engaged and combo.keys <= self._pressed
            ]
            for combo in fired:
                combo.engaged = True
        except Exception as exc:  # noqa: BLE001
            log.error(f"단축키 상태 갱신 실패: {exc!r}")
            return
        for combo in fired:
            self._fire(combo.on_press)

    def _on_release(self, key) -> None:
        try:
            resolved = self._canonical(key)
            self._pressed.discard(resolved)
            released = [
                combo for combo in self._combos
                if combo.engaged and resolved in combo.keys
            ]
            for combo in released:
                combo.engaged = False
        except Exception as exc:  # noqa: BLE001
            log.error(f"단축키 상태 갱신 실패: {exc!r}")
            return
        for combo in released:
            self._fire(combo.on_release)

    def reset_state(self) -> None:
        """감시자가 이상 상태를 발견했을 때 키 상태를 초기화한다."""
        self._pressed.clear()
        for combo in self._combos:
            combo.engaged = False

    def start(self) -> None:
        self._listener = keyboard.Listener(
            on_press=self._on_press, on_release=self._on_release
        )
        self._listener.start()

    def stop(self) -> None:
        if self._listener is not None:
            self._listener.stop()
            self._listener = None
