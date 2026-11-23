# I-0010 active-display preflight

Observed at `2026-09-20T13:19:41Z`, before I-0010 implementation and without
launching or injecting Dark Souls III.

## Method

Windows `EnumDisplayDevicesW` identified attached desktop displays and
`EnumDisplaySettingsW` read the current mode plus enumerated modes at each
display's current resolution. `Win32_VideoController` independently reported
the active adapter path as 2560x1440 at 279 Hz; the native enumeration below is
used as the more precise mode record.

## Observation

```text
DEVICE=\\.\DISPLAY1 FLAGS=5 NAME=NVIDIA GeForce RTX 4070 Ti SUPER
CURRENT=2560x1440@280 MAX_SAME_RES=280 HAS_240=True PRIMARY=True
DEVICE=\\.\DISPLAY2 FLAGS=1 NAME=NVIDIA GeForce RTX 4070 Ti SUPER
CURRENT=1920x1080@60 MAX_SAME_RES=75 HAS_240=False PRIMARY=False
DEVICE=\\.\DISPLAY3 FLAGS=0 NAME=NVIDIA GeForce RTX 4070 Ti SUPER
DEVICE=\\.\DISPLAY4 FLAGS=0 NAME=NVIDIA GeForce RTX 4070 Ti SUPER
```

## Interpretation and limit

The active primary display is currently 2560x1440 at 280 Hz and exposes at
least one mode at or above 240 Hz at that resolution. This clears the known
display-mode precondition that previously capped a target-165 run near 120 FPS.
It does not prove that DS3, the compositor or the driver will sustain 240
updates/presents per second; that requires the separately announced live
cadence gate.
