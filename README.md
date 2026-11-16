# DigiThru

An [elekloader](https://github.com/elekloader/elekloader) mod for the **Digitakt mk1, OS 1.53**.

Play an external synth through a Digitakt MIDI track and keep your keyboard's
wheels: pitch bend, aftertouch and the mod wheel arriving on the **AUTO
channel** go out on the **active MIDI track's channel**, just like its notes.
The stock firmware forwards only notes.

| In (AUTO channel) | Out (active MIDI track's CHAN) |
|---|---|
| Pitch bend | Pitch bend, full 14-bit |
| Channel aftertouch | Channel aftertouch |
| Poly aftertouch | Poly aftertouch |
| Mod wheel (CC 1, LSB CC 33) | CC 1 / CC 33 |

When you switch to another track, the track you left gets pitch bend centre
and pressure 0, so no note stays bent.

## Install

1. Get `digithru-<version>.elemod` from [Releases](../../releases), or from
   the [Elekloader Shop](https://elekloader.github.io/shop/).
2. In elekloader (app or [web builder](https://elekloader.github.io/elekloader/)),
   pick your stock `Digitakt_OS1.53.syx`, add the `.elemod`, tick it with any
   other mods and build, together with the Digitakt core.
3. Send the built `.syx` with Elektron Transfer, as for any OS update.

## Use

- Set your keyboard to the Digitakt's AUTO channel (SETTINGS > MIDI CONFIG > CHANNELS).
- Select a MIDI track (9-16) whose CHAN is set to your synth's channel.
- MIDI CONFIG must receive notes (pitch bend, aftertouch) and CC (mod wheel), and PORT CONFIG's INPUT FROM / OUTPUT TO
  decide the ports, as for the track's notes.
- Audio tracks (1-8) and messages on other channels are left alone.

## How it works

The Digitakt's MIDI task hands each received message to a table of handlers,
one per status nibble (0x401b8a54 in OS 1.53). The entries for poly
aftertouch (0xA0), channel pressure (0xD0) and pitch bend (0xE0) point at an
empty `rts`, which is why the stock firmware drops them. DigiThru replaces
those three pointers (`ptr` sites) with its own handlers. The stock CC handler
keeps CC 1 and 33 for the track's internal mod wheel before any CC router (or
core's `ev_midi_cc`) sees them, so DigiThru also wraps the CC entry (0xB0): it
forwards the mod wheel, then calls the stock handler, which still gets every
CC as before. Messages are sent through the
sequencer's own MIDI output routine (0x400e076a), which follows OUTPUT TO and
copies each message into the DIN buffer with interrupts masked.

It changes no pattern, kit or project data.

## Status

0.2, tested on a Digitakt mk1 with OS 1.53: pitch bend, aftertouch and the
mod wheel reach the synth on the active MIDI track's channel. The forwarding logic has a host-side test
(`tools/run_host_test.sh`, 19 cases, ASan/UBSan).

## Build

```bash
python -m elekloader.sdk.build path/to/digithru --stock Digitakt_OS1.53.syx
```

## If a build won't start

Hold FUNC while powering on, press TRIG 4 (OS UPGRADE) and send the stock
OS file. The bootloader is never touched.

## Licence

GPL-2.0-or-later. Contains no Elektron firmware.
