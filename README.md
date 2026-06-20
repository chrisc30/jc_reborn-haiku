# Johnny Reborn — Haiku Screensaver Port

A port of [Johnny Reborn](https://github.com/jno6809/jc_reborn) to [Haiku OS](https://www.haiku-os.org/) as a native screensaver.

Johnny Reborn is an open-source engine for the classic **Johnny Castaway** screensaver by Sierra On-Line (1992).

![Preview](preview.jpg)

---

## Requirements

- Haiku OS (x86_64)
- Original Johnny Castaway resource files (`RESOURCE.MAP` and `RESOURCE.001`)
  - These are not included and must be obtained separately from the original Sierra game

---

## Building

```bash
make -f Makefile.haiku
make -f Makefile.haiku install
```

The `install` target copies the screensaver to:
```
~/config/non-packaged/add-ons/Screen Savers/JohnnyCastaway
```

---

## Setup

1. Build and install as above
2. Place `RESOURCE.MAP` and `RESOURCE.001` in `/boot/home/JohnnyCastaway/`
3. Open **Screen Saver** preferences (Deskbar → Preferences → Screen Saver)
4. Select **JohnnyCastaway** from the list
5. The resource path can be changed in the settings panel if needed

---

## What changed from the Linux version

| Area | Change |
|---|---|
| Graphics | SDL2 removed; replaced with `GrSurface` pixel buffer and Haiku `BBitmap`/`BView` |
| Events | SDL event loop replaced with Haiku `snooze()` timing |
| Sound | Stubbed out (SDL_mixer removed; Haiku Media Kit port is a future TODO) |
| Entry point | New `jc_saver.cpp` — `BScreenSaver` subclass running game in a `BThread` |
| Structs | New `ttm_types.h` defining `TTtmSlot`, `TTtmThread`, `TAdsScene` (were undefined in original) |
| Bug fixes | `grCopyZoneToBg` bounds checking, `LOAD_PALETTE` TTM opcode, island layer crash |

---

## Known issues

- No sound (stubbed)
- The settings panel Apply button does not yet persist the resource path between sessions

---

## Credits

- Original Johnny Castaway screensaver © Sierra On-Line
- Johnny Reborn engine by Jeremie GUILLAUME — https://github.com/jno6809/jc_reborn
- Haiku port by chrisc30
