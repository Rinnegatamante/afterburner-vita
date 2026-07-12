<h1 align="center"><img alt="FalsoNDK" src="https://raw.githubusercontent.com/gist/elliencode/f75d532a99f852c9235f7ed586e14f2d/raw/fb5b8e1f55db03c4cda1aa65086901579ad3de8a/FalsoNDK.svg"></h1>

<p align="center">
  <a href="#how-it-works">How it works</a> •
  <a href="#setup">Setup</a> •
  <a href="#usage">Usage</a> •
  <a href="#flags">Flags</a> •
  <a href="#credits">Credits</a> •
  <a href="#license">License</a>
</p>

FalsoNDK (*falso* as in *fake* from Italian) is a library that implements a
subset of the Android NDK sufficient to run `ANativeActivity`-based Android
games on the PSVita via [SoLoBoP](https://github.com/v-atamanenko/soloader-boilerplate).

It is a sister project to [FalsoJNI](https://github.com/v-atamanenko/FalsoJNI),
which handles the Java/JNI side of things. FalsoNDK handles the native side:
the activity lifecycle, input, sensors, event loop, and the Linux FD
primitives that VitaSDK does not provide.

## How it works

Android's NDK exposes two layers that a native game typically depends on:

- **Linux FD primitives** — `eventfd`, `epoll`, `pipe`, and the standard
  `read`/`write`/`close` calls that dispatch across them. VitaSDK provides
  none of these. FalsoNDK reimplements them on top of SCE kernel primitives
  (`LwMutex` and `MsgPipe`), with enough accuracy for the game
  to use them for its internal threading and event signaling.

- **Android NDK APIs** — `ANativeActivity`, `ALooper`, `AInputQueue`,
  `ANativeWindow`, `ASensor`, `AAssetManager`, etc. FalsoNDK provides
  stubs or functional implementations of each. The `shim/` layer bridges PSVita
  controls and motion sensors into the `AInputEvent` / `ASensorEvent` formats the game
  expects.

### Ports using FalsoNDK

- [SoulCalibur by Rinnegatamante](https://github.com/Rinnegatamante/soulcalibur_vita)
- [Baldur's Gate: Dark Alliance by Nevak](https://github.com/Nevak/bgda-vita)
- [Galaxy on Fire 2 by gl33ntwine](https://github.com/v-atamanenko/gof2-vita)
- [Action Squad by elliencode](https://github.com/elliencode/actionsquad-psv-internal)
- And more.

## Setup

FalsoNDK is designed to be used as a submodule inside a
soloader-boilerplate-based project:

```sh
git submodule add https://github.com/elliencode/FalsoNDK lib/falso_ndk
```

Then in your `CMakeLists.txt`, add the subdirectory and link against it:

```cmake
add_subdirectory(lib/falso_ndk)
add_dependencies(${CMAKE_PROJECT_NAME} FalsoNDK)

target_link_libraries(${CMAKE_PROJECT_NAME}
                      FalsoNDK
                      <...>
                      )
```

And adjust your dynlib to include FalsoNDK's implementations:

```c
#include <falso_ndk/FalsoNDK.h>

so_default_dynlib default_dynlib[] = {
  { "AConfiguration_delete", (uintptr_t)&AConfiguration_delete },
  { "AConfiguration_fromAssetManager", (uintptr_t)&AConfiguration_fromAssetManager },
  { "AConfiguration_getCountry", (uintptr_t)&AConfiguration_getCountry },
  { "AConfiguration_getLanguage", (uintptr_t)&AConfiguration_getLanguage },
  { "AConfiguration_new", (uintptr_t)&AConfiguration_new },
  { "AInputEvent_getDeviceId", (uintptr_t)&AInputEvent_getDeviceId },
  { "AInputEvent_getSource", (uintptr_t)&AInputEvent_getSource },
  { "AInputEvent_getType", (uintptr_t)&AInputEvent_getType },
  { "AInputQueue_attachLooper", (uintptr_t)&AInputQueue_attachLooper },
  { "AInputQueue_detachLooper", (uintptr_t)&AInputQueue_detachLooper },
  { "AInputQueue_finishEvent", (uintptr_t)&AInputQueue_finishEvent },
  { "AInputQueue_getEvent", (uintptr_t)&AInputQueue_getEvent },
  { "AInputQueue_hasEvents", (uintptr_t)&AInputQueue_hasEvents },
  { "AInputQueue_preDispatchEvent", (uintptr_t)&AInputQueue_preDispatchEvent },
  { "AKeyEvent_getAction", (uintptr_t)&AKeyEvent_getAction },
  { "AKeyEvent_getKeyCode", (uintptr_t)&AKeyEvent_getKeyCode },
  { "ALooper_addFd", (uintptr_t)&ALooper_addFd },
  { "ALooper_pollAll", (uintptr_t)&ALooper_pollAll },
  { "ALooper_prepare", (uintptr_t)&ALooper_prepare },
  { "AMotionEvent_getAction", (uintptr_t)&AMotionEvent_getAction },
  { "AMotionEvent_getAxisValue", (uintptr_t)&AMotionEvent_getAxisValue },
  { "AMotionEvent_getPointerCount", (uintptr_t)&AMotionEvent_getPointerCount },
  { "AMotionEvent_getX", (uintptr_t)&AMotionEvent_getX },
  { "AMotionEvent_getY", (uintptr_t)&AMotionEvent_getY },
  { "ANativeWindow_getHeight", (uintptr_t)&ANativeWindow_getHeight },
  { "ANativeWindow_getWidth", (uintptr_t)&ANativeWindow_getWidth },
  { "ANativeWindow_setBuffersGeometry", (uintptr_t)&ANativeWindow_setBuffersGeometry },
  
  { "read", (uintptr_t)&fndk_read },
  { "write", (uintptr_t)&fndk_write },
  { "pipe", (uintptr_t)&fndk_pipe },
  { "close", (uintptr_t)&fndk_close }, // Optional
};
```

Pay attention to the last four lines! `read`, `write`, and `pipe` linked
to FalsoNDK's implementation are necessary for the library to work. `close` is optional.
Omitting it will cause pipe and eventfd objects to leak, but empirically native games only ever create one of each.

The library also defines several weak symbols that let you customize behavior.
First of all, you can change the controls mapping and analog stick deadzones, like so:

```c
#include <falso_ndk/FalsoNDK.h>

ButtonMapping fndk_button_mapping[] = {
    { SCE_CTRL_CROSS,    AKEYCODE_BUTTON_B },   // swap A/B for this game
    { SCE_CTRL_CIRCLE,   AKEYCODE_BUTTON_A },
    { SCE_CTRL_START,    AKEYCODE_BUTTON_START },
    { SCE_CTRL_SELECT,   AKEYCODE_BUTTON_SELECT },
    // Only 4 buttons used.
};
int fndk_button_mapping_count = 4;

float L_INNER_DEADZONE = 0.15f;
float R_INNER_DEADZONE = 0.15f;
```

You can also override the default logging function:

```c
#include <falso_ndk/FalsoNDK.h>

void fndk_log(int severity, const char * message) {
    if (severity >= FALSONDK_LOG_WARN) {
        my_log_write("[FalsoNDK] %s\n", message);
    }
}
```

## Usage

Below is a typical `main()` for an `ANativeActivity`-based game port. It
follows the same pattern as the boilerplate's `main.c`:

```c
#include <falso_jni/FalsoJNI.h>
#include <falso_ndk/FalsoNDK.h>

void main() {
    soloader_init_all();

    // Load and call JNI_OnLoad so the game initialises its Java-side state
    int (*JNI_OnLoad)(void *jvm) = (void *)so_symbol(&so_mod, "JNI_OnLoad");
    JNI_OnLoad(&jvm);

    gl_init();

    // Build a fake ANativeActivity that the game's onCreate will receive
    ANativeActivity *activity = malloc(sizeof(ANativeActivity));
    activity->callbacks       = malloc(sizeof(ANativeActivityCallbacks));
    activity->env             = &jni;                    // from FalsoJNI
    activity->vm              = &jvm;                    // from FalsoJNI
    activity->clazz           = (jclass)0x42424242;
    activity->internalDataPath = DATA_PATH "assets/";
    activity->externalDataPath = DATA_PATH "assets/";
    activity->sdkVersion      = 14;
    activity->instance        = NULL;

    // Drive the activity lifecycle
    int (*ANativeActivity_onCreate)(ANativeActivity *, void *, size_t) =
        (void *)so_symbol(&so_mod, "ANativeActivity_onCreate");
    ANativeActivity_onCreate(activity, NULL, 0);

    activity->callbacks->onStart(activity);
    activity->callbacks->onResume(activity);

    // Wire up input and the native window
    AInputQueue *aInputQueue = AInputQueue_create();
    activity->callbacks->onInputQueueCreated(activity, aInputQueue);

    ANativeWindow *aNativeWindow = ANativeWindow_create();
    activity->callbacks->onNativeWindowCreated(activity, aNativeWindow);

    activity->callbacks->onWindowFocusChanged(activity, 1);

    sceKernelExitDeleteThread(0);
}
```

FalsoNDK automatically starts the controls and sensors polling threads when
`AInputQueue_create()` and the sensor queue are initialized; you don't need
to manage those manually.

## Flags

Define these in your `CMakeLists.txt` as needed:

| Flag | Effect                                                                                                                                            |
|---|---------------------------------------------------------------------------------------------------------------------------------------------------|
| `FNDK_SAFER_SLOWER` | Validates pipe and eventfd fds by scanning the pool rather than using the fast arithmetic check. Useful if you're seeing unexpected EBADF errors. |
| `DEBUG_EPOLL` | Verbose logging for all epoll operations                                                                                                          |
| `DEBUG_EVENTFD` | Verbose logging for all eventfd operations                                                                                                        |
| `DEBUG_PIPEFD` | Verbose logging for all pipe operations                                                                                                           |

## Credits

- Volodymyr Atamanenko ([gl33ntwine](https://github.com/v-atamanenko))
- Ellie J. Turner ([elliencode](https://github.com/elliencode))

## License

This software may be modified and distributed under the terms of the Apache 2.0
license. See the [LICENSE](LICENSE) file for details.
