#include "android/ANativeActivity.h"

#include <cstdlib>
#include <cerrno>

void ANativeActivity_setWindowFlags(ANativeActivity* activity, uint32_t addFlags, uint32_t removeFlags) {
    // see Android's window.h for flags reference.
    // they are pretty much useless for us because we are always fullscreen, focusable, etc.
}

void ANativeActivity_finish(ANativeActivity* activity) {
    free(activity);
}
