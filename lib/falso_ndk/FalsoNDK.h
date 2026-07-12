/*
 * FalsoNDK
 * 
 * Android NDK compatibility layer for the PlayStation Vita.
 *
 * Copyright 2026      Ellie J Turner
 * Copyright 2023-2024 Volodymyr Atamanenko
 * Copyright 2010      The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "FalsoNDK_Utils.h"

#include "android/AAssetManager.h"
#include "android/AConfiguration.h"
#include "android/AInput.h"
#include "android/ALooper.h"
#include "android/ANativeActivity.h"
#include "android/ANativeWindow.h"
#include "android/ASensor.h"
#include "android/native_app_glue.h"

#include "linux/fndk_unistd.h"
#include "linux/fndk_pipe.h"

#include "shim/fndk_controls.h"
