/**
 * @file audio_player.h
 * @brief MP3 decode and ES8311 playback owned by the salary-cat app.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t audio_player_init(void);
esp_err_t audio_player_deinit(void);
esp_err_t audio_player_play_file(const char *path);
void audio_player_adjust_volume(int delta);
int audio_player_get_volume(void);

#ifdef __cplusplus
}
#endif