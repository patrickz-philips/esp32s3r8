/**
 * @file audio_player.h
 * @brief MP3 decode + codec playback module: decodes an MP3 from the SD card
 *        and streams the PCM to the ES8311 speaker codec. Thread management
 *        (the playback loop) lives in the app layer (app_salary_cat.c).
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* POSIX path of the MP3 asset on the SD card. */
#define AUDIO_PLAYER_MP3_PATH "/sdcard/music.mp3"

/**
 * @brief Initialize the speaker codec and the MP3 decoder.
 *
 * @return ESP_OK on success, or an error code if the codec or decoder could
 *         not be created.
 */
esp_err_t audio_player_init(void);

/**
 * @brief Open @p path, decode it to completion (streaming from the SD card),
 *        and close it. Blocking; intended to be driven from a dedicated task.
 *
 * @return ESP_OK once the file finished playing, or ESP_FAIL if it could not
 *         be opened.
 */
esp_err_t audio_player_play_file(const char *path);

/**
 * @brief Adjust the output volume by @p delta (clamped to 0..100). The change
 *        is applied by the audio task, so this is safe to call from another
 *        task (e.g. the UI thread).
 */
void audio_player_adjust_volume(int delta);

/**
 * @brief Get the current output volume (0..100).
 */
int audio_player_get_volume(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif
