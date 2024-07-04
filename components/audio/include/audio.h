// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "led_strip.h"
#include "driver/rmt.h"

typedef enum {
    NONE,
    PREVIOUSaudio,
    PLAY_PAUSEaudio,
    NEXTaudio,
    STOPaudio,
    VOL_UPaudio,
    VOL_DOWNaudio
} audio_command_t;

/**
 * @brief Initialize the audio and create task to play and control music.
 *        Note: You need to initialize touch before you can initialize audio
 *

 */
int audio_init();

/**
 * @brief Guarda comando en la cola
 */
BaseType_t send_command(audio_command_t command);

#ifdef __cplusplus
}
#endif
