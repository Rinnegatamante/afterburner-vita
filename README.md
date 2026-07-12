# After Burner Climax Vita

<p align="center"><img src="./extras/screenshots/game1.png"></p>

This is a wrapper/port of <b>After Burner Climax</b> for the *PS Vita*.

The port works by loading the official Android ARMv7 executables in memory, resolving its imports with native functions and patching it in order to properly run.
By doing so, it's basically as if we emulate a minimalist Android environment in which we run natively the executable as is.

## Changelog

### v.1.0

- Initial release.
- Menus are interactable only with touch whilst gameplay is fully physically buttons mapped.

## Notes and Tips

- The port has been tested with v.0.1.7 of the game.

## Setup Instructions (For End Users)

In order to properly install the game, you'll have to follow these steps precisely:

- Install [kubridge](https://github.com/TheOfficialFloW/kubridge/releases/) and [FdFix](https://github.com/TheOfficialFloW/FdFix/releases/) by copying `kubridge.skprx` and `fd_fix.skprx` to your taiHEN plugins folder (usually `ux0:tai`) and adding two entries to your `config.txt` under `*KERNEL`:
  
```
  *KERNEL
  ux0:tai/kubridge.skprx
  ux0:tai/fd_fix.skprx
```

**Note** Don't install fd_fix.skprx if you're using rePatch plugin

- **Optional**: Install [PSVshell](https://github.com/Electry/PSVshell/releases) to overclock your device to 500Mhz.
- Install `libshacccg.suprx`, if you don't have it already, by following [this guide](https://samilops2.gitbook.io/vita-troubleshooting-guide/shader-compiler/extract-libshacccg.suprx).
- Obtain your copy of *After Burner Climax* legally for Android in form of an `.apk` file and an obb. [You can get all the required files directly from your phone](https://stackoverflow.com/questions/11012976/how-do-i-get-the-apk-of-an-installed-app-without-root-access) or by using an apk extractor you can find in the play store.
- Open the apk with your zip explorer and extract the file `libacb.so` from the `lib/armeabi-v7a` folder to `ux0:data/afterburner`.
- *(Optional)* In order to have the intro logo to play, extract the file `sega_720.mp` from the `res/raw` folder to `ux0:data/afterburner`.
- Put the obb file named as `main.obb` in `ux0:data/afterburner`.
- Extract `libfmodstudio.suprx` from PCSE01188 or PCSE01426 following [this guide](https://gist.github.com/hatoving/99253e1b3efdefeaf0ca66e0c5dc7089).
- Extract the zip file in the Release tab in `ux0:data`.
- Install the vpk from the Release tab.

## Build Instructions (For Developers)

In order to build the loader, you'll need a [vitasdk](https://github.com/vitasdk) build fully compiled with softfp usage.  
You can find a precompiled version here: https://github.com/vitasdk/buildscripts/actions/runs/1102643776.  
Additionally, you'll need these libraries to be compiled as well with `-mfloat-abi=softfp` added to their CFLAGS:

- [libmathneon](https://github.com/Rinnegatamante/math-neon)

  - ```bash
    make install
    ```

- [vitaShaRK](https://github.com/Rinnegatamante/vitaShaRK)

  - ```bash
    make install
    ```

- [kubridge](https://github.com/TheOfficialFloW/kubridge)

  - ```bash
    mkdir build && cd build
    cmake .. && make install
    ```

- [vitaGL](https://github.com/Rinnegatamante/vitaGL)

  - ````bash
    make NO_DEBUG=1 SOFTFP_ABI=1 DEPTH_STENCIL_HACK=1 CIRCULAR_POOL_SPEEDHACK=1 HAVE_SHADER_CACHE=1 INDICES_SPEEDHACK=1 HAVE_CUSTOM_HEAP=1 SINGLE_THREADED_GC=1 ENABLE_LEGACY_PIPELINE=1 install
    ````

After all these requirements are met, you can compile the loader with the following commands:

```bash
mkdir build && cd build
cmake .. && make
```

## Credits

- TheFloW for the original .so loader.
- gl33ntwine for the awesome Android subsystem reimplementation [FalsoNDK](https://github.com/v-atamanenko/FalsoNDK) and [FalsoJNI](https://github.com/v-atamanenko/FalsoJNI).
- elliencode for giving an hand figuring out some qwirks with FalsoNDK and FalsoJNI and for the optimized FalsoNDK fork.
- BetterWinds for the Livearea assets.
- withLogic for the screenshots for the release.
