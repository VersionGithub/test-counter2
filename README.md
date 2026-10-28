# FrameWindow

Geode mod project based on the official [geode-sdk/example-mod](https://github.com/geode-sdk/example-mod) layout.

## Current scaffold
- Official-template-style CMake project and `mod.json`
- Requested timing buckets: `1`, `2`, `3`, `4`, `5 - 6`, `7 - 8`, `9 - 12`, `13 - 16`, `17 - 19`, `20+`
- Placeholder circle markers and frame labels
- Pause menu `Showcase` button
- Explicit TODO hooks for `.gdr2` parsing, game-accurate physics simulation, macro frame tracking, and sound playback
- GitHub Actions workflow placeholder for Geode CI

## Build locally
Install Geode SDK / CLI for your Geometry Dash installation, then run:

```sh
geode build
```

## Important status
This is a scaffold, **not a finished analyzer or verified `.geode` binary**. The `.gdr2` reader and exact physics-window search are not implemented. A correct implementation must use the actual replay format and actual per-frame Geometry Dash simulation; generic trajectory math is not accurate enough.

The workflow file is a starter and may need adjustment to the current Geode CI action/version.
