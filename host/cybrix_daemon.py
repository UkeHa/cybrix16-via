#!/usr/bin/env python3
"""
Cybrix 16 Macroboard Host Daemon (Audio-Switcher & PipeWire Soundboard)
-----------------------------------------------------------------------
Features:
- Direkte USB-HID-Erkennung des Kiiboom Cybrix 16 (VID 0x5343, PID 0x0080)
  (funktioniert auch in Vollbild-Spielen unter Wayland/Proton ohne Root)
- Ebene 0:
  - [3,0] Unten links (F13):        Tonquelle -> Lautsprecher
  - [3,1] Zweite unten links (F14): Tonquelle -> Headset
- Ebene 1 (Soundboard):
  - [0,1]..[3,2] (Slots 1..14):     Spielt Sound-Datei in TeamSpeak/Discord/Mumble ab
  - [3,3] Unten rechts (Ctrl+F19):  Stoppt aktuelle Soundboard-Wiedergabe sofort
  - Drehrad e1 (Ctrl+F20 / F21):    Soundboard-Lautstärke leiser / lauter
"""

import argparse
import glob
import json
import os
import select
import signal
import subprocess
import sys
import time
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent.parent
SOUNDS_DIR = BASE_DIR / "sounds"
CONFIG_PATH = SOUNDS_DIR / "soundboard.json"
SOUNDBOARD_SINK = "cybrix_soundboard"
CYBRIX_HID_ID = "0003:00005343:00000080"

# HID Usage Codes für F13 (0x68) bis F24 (0x73)
USAGE_F13 = 0x68
USAGE_F14 = 0x69
USAGE_F15 = 0x6A
USAGE_F16 = 0x6B
USAGE_F17 = 0x6C
USAGE_F18 = 0x6D
USAGE_F19 = 0x6E
USAGE_F20 = 0x6F
USAGE_F21 = 0x70
USAGE_F22 = 0x71
USAGE_F23 = 0x72
USAGE_F24 = 0x73

MOD_LCTRL = 0x01
MOD_RCTRL = 0x10


def notify(title: str, body: str = "", icon: str = "audio-card", timeout_ms: int = 2000) -> None:
    try:
        subprocess.Popen(
            [
                "notify-send",
                "-a", "Cybrix 16",
                "-i", icon,
                "-t", str(timeout_ms),
                "-h", "string:x-canonical-private-synchronous:cybrix16",
                title,
                body,
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
    except Exception:
        pass


def load_config() -> dict:
    try:
        with open(CONFIG_PATH, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception as e:
        print(f"[Cybrix] Fehler beim Laden von {CONFIG_PATH}: {e}", file=sys.stderr)
        return {}


def save_config(cfg: dict) -> None:
    try:
        with open(CONFIG_PATH, "w", encoding="utf-8") as f:
            json.dump(cfg, f, indent=2, ensure_ascii=False)
            f.write("\n")
    except Exception as e:
        print(f"[Cybrix] Fehler beim Speichern von {CONFIG_PATH}: {e}", file=sys.stderr)


def run_cmd(cmd: list[str]) -> str:
    try:
        res = subprocess.run(cmd, capture_output=True, text=True, timeout=4)
        return res.stdout
    except Exception:
        return ""


def find_sink_by_pattern(pattern: str) -> str | None:
    out = run_cmd(["pactl", "list", "short", "sinks"])
    for line in out.splitlines():
        parts = line.split("\t")
        if len(parts) >= 2 and pattern.lower() in parts[1].lower():
            return parts[1]
    return None


def get_default_sink() -> str:
    out = run_cmd(["pactl", "get-default-sink"]).strip()
    return out


def switch_audio_output(target_key: str) -> None:
    cfg = load_config()
    outputs = cfg.get("audio_outputs", {})
    target_cfg = outputs.get(target_key, {})
    pattern = target_cfg.get("match", target_key)
    label = target_cfg.get("label", target_key)

    sink_name = find_sink_by_pattern(pattern)
    if not sink_name:
        notify("Audio-Umschaltung fehlgeschlagen", f"Gerät '{label}' ({pattern}) nicht gefunden.", "dialog-error")
        print(f"[Cybrix] Sink für Muster '{pattern}' nicht gefunden.", file=sys.stderr)
        return

    run_cmd(["pactl", "set-default-sink", sink_name])

    # Alle aktiven Wiedergabe-Streams (außer dem internen Soundboard-Loopback) verschieben
    inputs_out = run_cmd(["pactl", "list", "short", "sink-inputs"])
    for line in inputs_out.splitlines():
        parts = line.split("\t")
        if len(parts) >= 2:
            stream_id = parts[0]
            run_cmd(["pactl", "move-sink-input", stream_id, sink_name])

    # Falls das Soundboard lokal mitgehört wird, Link auf das neue Ausgabegerät aktualisieren
    sync_soundboard_links(cfg, active_sink_override=sink_name)

    icon = "audio-headphones" if target_key == "headset" else "audio-speakers"
    notify(f"Tonquelle: {label}", f"Ausgabe umgeschaltet auf {label}", icon)
    print(f"[Cybrix] Tonquelle umgeschaltet auf: {sink_name} ({label})")


def ensure_soundboard_sink() -> None:
    sinks_out = run_cmd(["pactl", "list", "short", "sinks"])
    for line in sinks_out.splitlines():
        if SOUNDBOARD_SINK in line:
            return

    prev_default = get_default_sink()
    run_cmd([
        "pactl", "load-module", "module-null-sink",
        f"sink_name={SOUNDBOARD_SINK}",
        "sink_properties='device.description=Cybrix_Soundboard priority.driver=1 priority.session=1'",
    ])
    time.sleep(0.15)
    if prev_default and prev_default != SOUNDBOARD_SINK:
        run_cmd(["pactl", "set-default-sink", prev_default])



def sync_soundboard_links(cfg: dict | None = None, active_sink_override: str | None = None) -> None:
    if cfg is None:
        cfg = load_config()

    ensure_soundboard_sink()

    mon_fl = f"{SOUNDBOARD_SINK}:monitor_FL"
    mon_fr = f"{SOUNDBOARD_SINK}:monitor_FR"

    links_out = run_cmd(["pw-link", "-l"])
    inputs_out = run_cmd(["pw-link", "-i"])
    all_inputs = [line.strip() for line in inputs_out.splitlines() if line.strip() and not line.startswith(" ")]

    # 1. Finde alle Voice-Chat-Eingänge (TeamSpeak, Discord, Mumble, WebRTC sowie alle Ports,
    #    die aktuell am Standard-Mikrofon hängen)
    voice_prefixes = cfg.get("voice_apps", ["TeamSpeak", "Discord", "WEBRTC VoiceEngine", "Mumble", "mumble", "vesktop"])
    target_voice_ports = set()

    for port in all_inputs:
        for prefix in voice_prefixes:
            if port.lower().startswith(prefix.lower() + ":"):
                target_voice_ports.add(port)

    # Zusätzlich alle Capture-Clients erkennen, die mit einem echten Mikrofon (alsa_input.*) verbunden sind
    current_port = None
    for line in links_out.splitlines():
        if not line.startswith(" "):
            current_port = line.strip()
        else:
            if current_port and ("|<-" in line) and ("alsa_input." in line):
                if not current_port.startswith(SOUNDBOARD_SINK):
                    target_voice_ports.add(current_port)

    for vport in target_voice_ports:
        if vport.endswith("_FL"):
            run_cmd(["pw-link", mon_fl, vport])
        elif vport.endswith("_FR"):
            run_cmd(["pw-link", mon_fr, vport])
        else:
            # Mono-Eingang (z.B. TeamSpeak:input_MONO) bekommt beide Kanäle gemischt
            run_cmd(["pw-link", mon_fl, vport])
            run_cmd(["pw-link", mon_fr, vport])

    # 2. Lokales Mithören auf dem eigenen Standard-Ausgabegerät (Headset / Lautsprecher)
    if cfg.get("monitor_locally", True):
        default_sink = active_sink_override or get_default_sink()
        if default_sink and default_sink != SOUNDBOARD_SINK:
            # Alte Links zu anderen alsa_output Sinks trennen, damit es nur auf dem aktiven Gerät spielt
            for port in all_inputs:
                if port.startswith("alsa_output.") and not port.startswith(default_sink + ":"):
                    run_cmd(["pw-link", "-d", mon_fl, port])
                    run_cmd(["pw-link", "-d", mon_fr, port])

            run_cmd(["pw-link", mon_fl, f"{default_sink}:playback_FL"])
            run_cmd(["pw-link", mon_fr, f"{default_sink}:playback_FR"])


class SoundboardPlayer:
    def __init__(self) -> None:
        self.proc: subprocess.Popen | None = None
        self.current_slot: str | None = None

    def is_playing(self) -> bool:
        return self.proc is not None and self.proc.poll() is None

    def stop(self, show_notification: bool = True) -> None:
        if self.is_playing() and self.proc is not None:
            try:
                self.proc.terminate()
                self.proc.wait(timeout=0.5)
            except Exception:
                try:
                    self.proc.kill()
                except Exception:
                    pass
        self.proc = None
        self.current_slot = None
        if show_notification:
            notify("Soundboard gestoppt", "Wiedergabe beendet", "media-playback-stop", 1200)
            print("[Cybrix] Soundboard-Wiedergabe gestoppt.")

    def play_slot(self, slot_id: int) -> None:
        slot_key = str(slot_id)
        # Wenn derselbe Slot gerade läuft -> Toggle (Stopp)
        if self.is_playing() and self.current_slot == slot_key:
            self.stop(show_notification=True)
            return

        self.stop(show_notification=False)

        cfg = load_config()
        slots = cfg.get("slots", {})
        slot_info = slots.get(slot_key)
        if not slot_info:
            notify(f"Soundboard Slot {slot_id}", "Nicht in soundboard.json konfiguriert", "dialog-warning")
            return

        rel_or_abs = slot_info.get("file", "")
        label = slot_info.get("name", f"Slot {slot_id}")
        sound_path = Path(rel_or_abs)
        if not sound_path.is_absolute():
            sound_path = SOUNDS_DIR / sound_path

        if not sound_path.is_file():
            notify(
                f"Soundboard: {label}",
                f"Datei fehlt: {sound_path.name}\n(Lege sie in {SOUNDS_DIR} ab)",
                "dialog-warning",
                3000,
            )
            print(f"[Cybrix] Sound-Datei nicht gefunden: {sound_path}", file=sys.stderr)
            return

        vol = int(cfg.get("volume", 80))
        sync_soundboard_links(cfg)

        self.proc = subprocess.Popen(
            [
                "mpv",
                "--no-video",
                "--no-terminal",
                f"--audio-device=pulse/{SOUNDBOARD_SINK}",
                f"--volume={vol}",
                str(sound_path),
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        self.current_slot = slot_key
        notify(f"▶ Soundboard: {label}", f"{sound_path.name} (Lautstärke: {vol}%)", "media-playback-start", 1800)
        print(f"[Cybrix] Spiele Slot {slot_id} ({label}): {sound_path}")

    def change_volume(self, delta: int) -> None:
        cfg = load_config()
        vol = int(cfg.get("volume", 80))
        vol = max(5, min(150, vol + delta))
        cfg["volume"] = vol
        save_config(cfg)
        notify("Soundboard-Lautstärke", f"{vol}%", "audio-volume-high", 1200)
        print(f"[Cybrix] Soundboard-Lautstärke: {vol}%")


def find_cybrix_hidraw_devices() -> list[str]:
    devices = []
    for sys_path in sorted(glob.glob("/sys/class/hidraw/hidraw*")):
        uevent_path = os.path.join(sys_path, "device", "uevent")
        try:
            with open(uevent_path, "r", encoding="utf-8") as f:
                content = f.read()
            if f"HID_ID={CYBRIX_HID_ID}" in content:
                # input0 (6KRO), input1 (Raw HID für direkte Aktionen ohne OS-Tastendruck), input2 (NKRO)
                if "/input0" in content or "/input1" in content or "/input2" in content:
                    dev_name = os.path.basename(sys_path)
                    devices.append(f"/dev/{dev_name}")
        except Exception:
            continue
    return devices


def handle_raw_action(action_id: int, player: SoundboardPlayer) -> None:
    if action_id == 1:
        switch_audio_output("speakers")
    elif action_id == 2:
        switch_audio_output("headset")
    elif 11 <= action_id <= 24:
        player.play_slot(action_id - 10)
    elif action_id == 30:
        player.stop(show_notification=True)
    elif action_id == 31:
        player.change_volume(-5)
    elif action_id == 32:
        player.change_volume(+5)


def parse_hid_report(data: bytes) -> tuple[bool, set[int]]:
    """
    Gibt (ctrl_pressed, set_of_active_usage_ids) für einen HID-Report zurück.
    Unterstützt sowohl Standard-8-Byte-6KRO-Reports (input0) als auch
    QMK-Shared-Endpoint-NKRO-Reports (Report-ID 0x06 auf input2).
    """
    usages: set[int] = set()
    if not data:
        return False, usages

    # Fall 1: QMK Shared Endpoint NKRO Report (Report ID 0x06, Länge >= 20 Bytes)
    if data[0] == 0x06 and len(data) >= 20:
        mods = data[1]
        ctrl = bool(mods & (MOD_LCTRL | MOD_RCTRL))
        bitmap = data[2:]
        # Prüfe gezielt den Bereich 0x68 (F13) bis 0x73 (F24) -> Byte-Index 13 (0x68 // 8 = 13) und 14
        for usage in range(USAGE_F13, USAGE_F24 + 1):
            byte_idx = usage // 8
            bit_idx = usage % 8
            if byte_idx < len(bitmap) and (bitmap[byte_idx] & (1 << bit_idx)):
                usages.add(usage)
        return ctrl, usages

    # Fall 2: Standard 8-Byte 6KRO Keyboard Report (input0)
    if len(data) == 8 and data[0] in (0x00, MOD_LCTRL, MOD_RCTRL, MOD_LCTRL | MOD_RCTRL):
        mods = data[0]
        ctrl = bool(mods & (MOD_LCTRL | MOD_RCTRL))
        for b in data[2:8]:
            if USAGE_F13 <= b <= USAGE_F24:
                usages.add(b)
        return ctrl, usages

    return False, usages


def handle_action(ctrl: bool, usage: int, player: SoundboardPlayer) -> None:
    if not ctrl:
        if usage == USAGE_F13:
            switch_audio_output("speakers")
        elif usage == USAGE_F14:
            switch_audio_output("headset")
        elif USAGE_F15 <= usage <= USAGE_F24:
            slot_id = (usage - USAGE_F15) + 1  # F15 -> Slot 1 .. F24 -> Slot 10
            player.play_slot(slot_id)
    else:
        if USAGE_F15 <= usage <= USAGE_F18:
            slot_id = (usage - USAGE_F15) + 11  # Ctrl+F15 -> Slot 11 .. Ctrl+F18 -> Slot 14
            player.play_slot(slot_id)
        elif usage == USAGE_F19:
            player.stop(show_notification=True)
        elif usage == USAGE_F20:
            player.change_volume(-5)
        elif usage == USAGE_F21:
            player.change_volume(+5)


def run_daemon() -> None:
    player = SoundboardPlayer()
    ensure_soundboard_sink()
    sync_soundboard_links()

    running = True

    def sig_handler(_signum, _frame):
        nonlocal running
        running = False

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    print("[Cybrix] Starte Cybrix 16 Macroboard-Daemon...")

    while running:
        dev_paths = find_cybrix_hidraw_devices()
        if not dev_paths:
            time.sleep(1.5)
            continue

        fds: list[int] = []
        fd_to_path: dict[int, str] = {}
        active_keys_per_fd: dict[int, set[int]] = {}

        try:
            for p in dev_paths:
                fd = os.open(p, os.O_RDONLY | os.O_NONBLOCK)
                fds.append(fd)
                fd_to_path[fd] = p
                active_keys_per_fd[fd] = set()

            print(f"[Cybrix] Verbunden mit Cybrix 16 HID-Interfaces: {', '.join(dev_paths)}")

            last_link_check = time.monotonic()
            while running:
                rlist, _, _ = select.select(fds, [], [], 2.0)
                now = time.monotonic()
                if now - last_link_check >= 5.0:
                    # Alle 5s prüfen, ob TeamSpeak/Discord/Mumble neu gestartet wurde
                    if player.is_playing():
                        sync_soundboard_links()
                    last_link_check = now

                for fd in rlist:
                    try:
                        data = os.read(fd, 64)
                    except BlockingIOError:
                        continue
                    except OSError:
                        # Gerät wurde getrennt (z.B. Replug / Bootloader)
                        raise RuntimeError("HID device disconnected")

                    if not data:
                        raise RuntimeError("HID device EOF")

                    # Direkte Raw-HID-Aktion vom Cybrix 16 (sendet keinen Tastendruck an das OS!)
                    if len(data) >= 3 and data[0] == 0xFE and data[1] == 0xC1:
                        handle_raw_action(data[2], player)
                        continue

                    ctrl, current_usages = parse_hid_report(data)
                    prev_usages = active_keys_per_fd.get(fd, set())
                    newly_pressed = current_usages - prev_usages
                    active_keys_per_fd[fd] = current_usages

                    for usage in sorted(newly_pressed):
                        handle_action(ctrl, usage, player)

        except Exception as e:
            print(f"[Cybrix] Verbindung getrennt ({e}), warte auf Reconnect...", file=sys.stderr)
        finally:
            for fd in fds:
                try:
                    os.close(fd)
                except Exception:
                    pass
            time.sleep(1.0)

    player.stop(show_notification=False)


def main() -> None:
    parser = argparse.ArgumentParser(description="Kiiboom Cybrix 16 Audio-Switcher & Soundboard")
    parser.add_argument("--daemon", action="store_true", help="Starte Hintergrund-Listener für das Cybrix 16")
    parser.add_argument("--speakers", action="store_true", help="Tonquelle auf Lautsprecher umschalten")
    parser.add_argument("--headset", action="store_true", help="Tonquelle auf Headset umschalten")
    parser.add_argument("--play", type=int, metavar="SLOT", help="Soundboard-Slot (1..14) abspielen/stoppen")
    parser.add_argument("--stop", action="store_true", help="Soundboard-Wiedergabe sofort stoppen")
    parser.add_argument("--vol", type=int, metavar="DELTA", help="Soundboard-Lautstärke ändern (z.B. +5 oder -5)")

    args = parser.parse_args()
    player = SoundboardPlayer()

    if args.speakers:
        switch_audio_output("speakers")
    elif args.headset:
        switch_audio_output("headset")
    elif args.play is not None:
        player.play_slot(args.play)
    elif args.stop:
        run_cmd(["pkill", "-f", f"audio-device=pulse/{SOUNDBOARD_SINK}"])
        notify("Soundboard gestoppt", "Wiedergabe beendet", "media-playback-stop", 1200)
    elif args.vol is not None:
        player.change_volume(args.vol)
    else:
        run_daemon()


if __name__ == "__main__":
    main()
