/**
 * @file audio_player.c
 * @brief MP3 decode + codec playback. Decodes an MP3 file from the SD card with
 *        the Helix decoder and streams mono PCM to the ES8311 speaker codec.
 *        The playback thread lives in the app layer (app_salary_cat.c); this
 *        module only exposes init, a per-file decode call, and volume control.
 */

#include "audio_player.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "bsp/esp-bsp.h"
#include "esp_codec_dev.h"
#include "mp3dec.h"

#define AUDIO_READ_BUF_SIZE     4096U
/* Helix upper bound: MAX_NGRAN(2) * MAX_NSAMP(576) * MAX_NCHAN(2) shorts. */
#define AUDIO_PCM_MAX_SAMPLES   (1152 * 2)
#define AUDIO_DEFAULT_VOLUME    70

static const char *TAG = "audio_player";

static esp_codec_dev_handle_t s_codec;
static HMP3Decoder s_decoder;

/* Large scratch buffers kept in BSS to avoid deep task-stack usage. */
static uint8_t s_read_buf[AUDIO_READ_BUF_SIZE];
static int16_t s_pcm_buf[AUDIO_PCM_MAX_SAMPLES];
static int16_t s_mono_buf[AUDIO_PCM_MAX_SAMPLES];

static bool s_codec_open;
static int s_codec_rate;
/* Desired volume (0-100). Written from the UI thread, but only ever applied to
 * the codec by the audio task so all codec/I2C access stays on one thread. */
static volatile int s_volume = AUDIO_DEFAULT_VOLUME;
static volatile bool s_volume_dirty;

static esp_err_t codec_configure(int sample_rate)
{
    if (s_codec_open && s_codec_rate == sample_rate) {
        return ESP_OK;
    }

    if (s_codec_open) {
        esp_codec_dev_close(s_codec);
        s_codec_open = false;
    }

    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
        .channel = 1,
        .channel_mask = 0,
        .sample_rate = (uint32_t)sample_rate,
        .mclk_multiple = 0,
    };

    int ret = esp_codec_dev_open(s_codec, &fs);
    if (ret != 0) {
        ESP_LOGE(TAG, "esp_codec_dev_open failed: %d", ret);
        return ESP_FAIL;
    }

    esp_codec_dev_set_out_vol(s_codec, s_volume);
    s_volume_dirty = false;
    s_codec_open = true;
    s_codec_rate = sample_rate;
    ESP_LOGI(TAG, "Codec opened at %d Hz, mono", sample_rate);
    return ESP_OK;
}

void audio_player_adjust_volume(int delta)
{
    /* Runs in the UI thread: only record the target, never touch the codec
     * here (I2C access from two tasks stalls the LVGL/touch thread). */
    int vol = s_volume + delta;
    if (vol < 0) {
        vol = 0;
    } else if (vol > 100) {
        vol = 100;
    }
    s_volume = vol;
    s_volume_dirty = true;
}

int audio_player_get_volume(void)
{
    return s_volume;
}

/* Apply a pending volume change from the audio task only. */
static void apply_pending_volume(void)
{
    if (s_volume_dirty && s_codec != NULL && s_codec_open) {
        int vol = s_volume;
        s_volume_dirty = false;
        esp_codec_dev_set_out_vol(s_codec, vol);
        ESP_LOGI(TAG, "Volume applied: %d", vol);
    }
}

static void write_frame(const MP3FrameInfo *info)
{
    if (info->outputSamps <= 0 || info->nChans <= 0) {
        return;
    }

    if (codec_configure(info->samprate) != ESP_OK) {
        return;
    }

    apply_pending_volume();

    if (info->nChans == 2) {
        int frames = info->outputSamps / 2;
        for (int i = 0; i < frames; i++) {
            int32_t l = s_pcm_buf[2 * i];
            int32_t r = s_pcm_buf[2 * i + 1];
            s_mono_buf[i] = (int16_t)((l + r) / 2);
        }
        esp_codec_dev_write(s_codec, s_mono_buf, frames * (int)sizeof(int16_t));
    } else {
        esp_codec_dev_write(s_codec, s_pcm_buf, info->outputSamps * (int)sizeof(int16_t));
    }
}

/* Decode a single MP3 file to completion (streaming from SD). */
static void decode_file(FILE *fp)
{
    unsigned char *read_ptr = s_read_buf;
    int bytes_left = 0;
    bool eof = false;

    while (true) {
        if (!eof) {
            if (bytes_left > 0 && read_ptr != s_read_buf) {
                memmove(s_read_buf, read_ptr, (size_t)bytes_left);
            }
            read_ptr = s_read_buf;

            size_t want = AUDIO_READ_BUF_SIZE - (size_t)bytes_left;
            size_t got = fread(s_read_buf + bytes_left, 1U, want, fp);
            bytes_left += (int)got;
            if (got < want) {
                eof = true;
            }
        }

        if (bytes_left <= 0) {
            break;
        }

        int offset = MP3FindSyncWord(read_ptr, bytes_left);
        if (offset < 0) {
            /* No frame sync in the buffered data - discard it. */
            bytes_left = 0;
            if (eof) {
                break;
            }
            continue;
        }
        read_ptr += offset;
        bytes_left -= offset;

        int err = MP3Decode(s_decoder, &read_ptr, &bytes_left, s_pcm_buf, 0);
        if (err == 0) {
            MP3FrameInfo info;
            MP3GetLastFrameInfo(s_decoder, &info);
            write_frame(&info);
        } else {
            /* Bad frame or a false sync inside e.g. an ID3 tag: skip one byte
             * and resync. Near EOF with too little data left, we are done. */
            if (bytes_left > 0) {
                read_ptr += 1;
                bytes_left -= 1;
            }
            if (bytes_left <= 0 && eof) {
                break;
            }
        }
    }
}

esp_err_t audio_player_play_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        ESP_LOGE(TAG, "Failed to open %s", path);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Playing %s", path);
    decode_file(fp);
    fclose(fp);
    return ESP_OK;
}

esp_err_t audio_player_init(void)
{
    if (s_codec != NULL) {
        return ESP_OK;
    }

    s_codec = bsp_audio_codec_speaker_init();
    if (s_codec == NULL) {
        ESP_LOGE(TAG, "Failed to initialize speaker codec");
        return ESP_FAIL;
    }

    s_decoder = MP3InitDecoder();
    if (s_decoder == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MP3 decoder");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
