# Pebble Watchface with Animated Sine Wave

A Pebble watchface that displays the current time with an animated sine wave background.

## Project Structure

```
.
├── src/
│   └── dummy.c         # Main watchface implementation
├── package.json        # Project metadata and Pebble configuration
└── wscript            # Build system configuration
```

## Features

- Digital time display that updates every minute
- Animated sine wave background that continuously moves
- Black background with white text and graphics
- Support for both 12h and 24h time formats
- Memory efficient implementation
  - Basalt: Uses 2671 bytes RAM (62865 bytes free)
  - Aplite: Uses 2671 bytes RAM (21905 bytes free)

## Build Instructions

The project uses the Rebble tool for building and running. To build the project:

```bash
rebble build
```

This will generate a `build/dummy_project.pbw` file.

## Running in Emulator

To run the watchface in the emulator:

```bash
rebble install --emulator basalt build/dummy_project.pbw
```

To interact with the emulator:

```bash
rebble emu-control
```

This will provide a web interface accessible at a local URL (displayed in the terminal) where you can:
- View the watchface
- Test different time formats (12h/24h)
- Simulate different times of day
- Test other watch features

## Implementation Details

### Main Components

1. **Time Display**
   - Uses Pebble's time service to update every minute
   - Formats time according to user's 12h/24h preference
   - Centered text using Bitham 42 Bold font

2. **Sine Wave Animation**
   - Smooth animation using app timer
   - 100ms update interval
   - Wave calculated using Pebble's sine lookup table
   - Wave amplitude: 1/4 of screen height
   - Wave moves continuously across the screen

3. **Memory Management**
   - Efficient use of Pebble's resources
   - Clean cleanup in window unload
   - Proper timer management

## Configuration Files

### package.json
```json
{
  "name": "dummy-watchface",
  "author": "Rebble",
  "version": "1.0.0",
  "keywords": ["pebble-app"],
  "private": true,
  "dependencies": {},
  "pebble": {
    "displayName": "Dummy Watchface",
    "uuid": "1234abcd-1234-1234-1234-123456789abc",
    "sdkVersion": "3",
    "enableMultiJS": false,
    "targetPlatforms": [
      "aplite",
      "basalt"
    ],
    "watchapp": {
      "watchface": true
    },
    "resources": {
      "media": []
    }
  }
}
```

### wscript
```python
import os

def options(ctx):
    ctx.load('pebble_sdk')

def configure(ctx):
    ctx.load('pebble_sdk')

def build(ctx):
    ctx.load('pebble_sdk')
    build_worker = os.path.exists('worker_src')
    binaries = []

    for p in ctx.env.TARGET_PLATFORMS:
        ctx.set_env(ctx.all_envs[p])
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf = '{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_program(target=app_elf, source=ctx.path.ant_glob('src/**/*.c'))

        if build_worker:
            worker_elf = '{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': p, 'app_elf': app_elf, 'worker_elf': worker_elf})
            ctx.pbl_worker(target=worker_elf, source=ctx.path.ant_glob('worker_src/**/*.c'))
        else:
            binaries.append({'platform': p, 'app_elf': app_elf})

    ctx.set_group('bundle')
    ctx.pbl_bundle(binaries=binaries, js=ctx.path.ant_glob('src/js/**/*.js'))
```

## Development Notes

- The project was created using the Rebble tool, which is the modern replacement for the original Pebble SDK tools
- The watchface supports both Aplite (original Pebble) and Basalt (Pebble Time) platforms
- No JavaScript components are included, but the build system is configured to support them if needed
- The animation system uses Pebble's efficient drawing APIs and timer services
