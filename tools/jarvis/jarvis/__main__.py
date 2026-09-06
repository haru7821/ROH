"""진입점. 단축키를 누르고 말하면 듣고, 답하고, 필요한 일을 대신 한다."""
from __future__ import annotations

import argparse
import queue
import sys
import threading
from pathlib import Path

from . import log
from .agent import Agent
from .approval import Approver
from .config import Config, ConfigError
from .tts import Speaker

QUIT_WORDS = {"종료", "종료해", "종료해줘", "자비스 종료", "그만", "끝"}
RESET_WORDS = {"초기화", "대화 초기화", "새로 시작", "리셋"}


def _normalize(text: str) -> str:
    return text.strip().strip(".!?,~ ").replace(" ", "")


def _is_command(text: str, words: set[str]) -> bool:
    compact = _normalize(text)
    return any(compact == _normalize(word) for word in words)


class Jarvis:
    def __init__(self, config: Config):
        self.config = config
        self.speaker = Speaker(
            engine=str(config.get("tts.engine", "edge")),
            voice=str(config.get("tts.voice", "ko-KR-SunHiNeural")),
            rate=str(config.get("tts.rate", "+0%")),
            sapi_rate=int(config.get("tts.sapi_rate", 190)),
            max_chars=int(config.get("tts.speak_max_chars", 700)),
        )
        self.approver = Approver(config.confirm_tools, speaker=self.speaker)
        self.agent = Agent(config, self.approver)
        self.stop_event = threading.Event()

    # --- 한 번의 요청 처리 ---------------------------------------------

    def handle(self, text: str) -> bool:
        """False 를 돌려주면 프로그램을 끝낸다."""
        if not text.strip():
            return True
        log.user(text)
        if _is_command(text, QUIT_WORDS):
            self.speaker.speak("종료합니다.")
            return False
        if _is_command(text, RESET_WORDS):
            self.agent.reset()
            log.info("대화 기록을 비웠습니다.")
            self.speaker.speak("대화를 새로 시작합니다.")
            return True
        try:
            answer = self.agent.turn(text)
        except Exception as exc:  # noqa: BLE001 - 어떤 실패에도 비서는 살아 있어야 한다
            log.error(f"응답 실패: {exc}")
            self.speaker.speak("처리 중 문제가 생겼습니다. 화면을 확인해 주세요.")
            return True
        if answer:
            log.assistant(answer)
            self.speaker.speak(answer)
        return True

    # --- 텍스트 모드 ----------------------------------------------------

    def run_text(self) -> None:
        log.banner(f"{self.config.get('assistant.name', '자비스')} — 텍스트 모드")
        log.info("메시지를 입력하세요. 종료하려면 '종료' 또는 Ctrl+C.")
        while True:
            try:
                text = input("> ")
            except (EOFError, KeyboardInterrupt):
                print()
                return
            if not self.handle(text):
                return

    # --- 음성 모드 ------------------------------------------------------

    def run_voice(self) -> None:
        from .audio import Recorder, resolve_input_device
        from .hotkey import HotkeyManager
        from .stt import Transcriber

        device = resolve_input_device(str(self.config.get("audio.input_device", "")) or None)
        recorder = Recorder(
            device=device,
            max_seconds=float(self.config.get("audio.max_seconds", 60.0)),
        )
        transcriber = Transcriber(
            model_size=str(self.config.get("stt.model_size", "small")),
            device=str(self.config.get("stt.device", "cpu")),
            compute_type=str(self.config.get("stt.compute_type", "int8")),
            language=str(self.config.get("stt.language", "ko")),
        )
        transcriber.load()

        min_seconds = float(self.config.get("audio.min_seconds", 0.4))
        mode = str(self.config.get("hotkey.mode", "hold")).lower()
        combo = str(self.config.get("hotkey.combo", "<ctrl>+<space>"))
        quit_combo = str(self.config.get("hotkey.quit_combo", "<ctrl>+<alt>+q"))

        audio_queue: queue.Queue = queue.Queue()
        busy = threading.Event()

        def begin() -> None:
            if busy.is_set() or recorder.active:
                return
            recorder.start()
            print("\n🎤 듣는 중…", end="", flush=True)

        def finish() -> None:
            if not recorder.active:
                return
            audio = recorder.stop()
            seconds = len(audio) / 16_000
            print(f"\r🎤 {seconds:.1f}초 녹음 완료.        ")
            if seconds < min_seconds:
                log.info("너무 짧아서 무시했습니다.")
                return
            audio_queue.put(audio)

        def toggle() -> None:
            finish() if recorder.active else begin()

        hotkeys = HotkeyManager()
        if mode == "toggle":
            hotkeys.add(combo, on_press=toggle)
        else:
            hotkeys.add(combo, on_press=begin, on_release=finish)
        hotkeys.add(quit_combo, on_press=self.stop_event.set)
        hotkeys.start()

        name = self.config.get("assistant.name", "자비스")
        log.banner(f"{name} 준비 완료.")
        log.info(f"  말하기: {combo} ({'토글' if mode == 'toggle' else '누르고 있는 동안'})")
        log.info(f"  종료:   {quit_combo} 또는 Ctrl+C")
        log.info(f"  도구 {len(self.agent.tool_names())}개: {', '.join(self.agent.tool_names())}")
        self.speaker.speak(f"{name} 준비되었습니다.")

        try:
            while not self.stop_event.is_set():
                try:
                    audio = audio_queue.get(timeout=0.3)
                except queue.Empty:
                    continue
                busy.set()
                try:
                    text = transcriber.transcribe(audio)
                    if not text:
                        log.info("들리지 않았습니다.")
                        continue
                    if not self.handle(text):
                        break
                finally:
                    busy.clear()
        except KeyboardInterrupt:
            print()
        finally:
            hotkeys.stop()
            recorder.abort()
        log.banner("종료했습니다.")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="jarvis", description="개인 음성 비서")
    parser.add_argument("--config", type=Path, default=None, help="설정 파일 경로")
    parser.add_argument("--text", action="store_true", help="마이크 없이 텍스트로 대화")
    parser.add_argument("--say", type=str, default=None, help="한 마디만 처리하고 종료")
    args = parser.parse_args(argv)

    try:
        config = Config.load(args.config)
    except ConfigError as exc:
        log.error(str(exc))
        return 1

    if not config.api_key:
        log.error(
            "Anthropic API 키가 없습니다.\n"
            "  환경변수 ANTHROPIC_API_KEY 를 설정하거나 config.toml 의 [api] api_key 를 채우세요."
        )
        return 1

    jarvis = Jarvis(config)
    if args.say:
        jarvis.handle(args.say)
        return 0
    if args.text:
        jarvis.run_text()
        return 0
    try:
        jarvis.run_voice()
    except Exception as exc:  # noqa: BLE001
        log.error(f"음성 모드를 시작할 수 없습니다: {exc}")
        log.info("마이크 없이 쓰려면 --text 옵션을 붙여 실행하세요.")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
