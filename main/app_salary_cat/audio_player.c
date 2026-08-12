/**
 * @file audio_player.c
 * @brief Decode an MP3 stream with Helix and play mono PCM through the BSP codec.
 */

#include "audio_player.h"

#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#include "bsp/esp-bsp.h"
#include "esp_codec_dev.h"
#include "esp_log.h"
#include "mp3dec.h"

#define AUDIO_READ_BUF_SIZE 4096U
#define AUDIO_PCM_MAX_SAMPLES (1152 * 2)
#define AUDIO_DEFAULT_VOLUME 70

static const char *TAG = "audio_player";

static esp_codec_dev_handle_t s_codec;
static HMP3Decoder s_decoder;
static uint8_t s_read_buf[AUDIO_READ_BUF_SIZE];
static int16_t s_pcm_buf[AUDIO_PCM_MAX_SAMPLES];
static int16_t s_mono_buf[AUDIO_PCM_MAX_SAMPLES];
static bool s_codec_open;
static int s_codec_rate;
static atomic_int s_volume = AUDIO_DEFAULT_VOLUME;
static atomic_bool s_volume_dirty;

static esp_err_t codec_configure(int sample_rate)
{
    if (s_codec_open && s_codec_rate == sample_rate) {
        return ESP_OK;
    }

    if (s_codec_open) {
        int close_ret = esp_codec_dev_close(s_codec);
        if (close_ret != 0) {
            ESP_LOGE(TAG, "Failed to close codec: %d", close_ret);
            return ESP_FAIL;
        }
        s_codec_open = false;
    }

    esp_codec_dev_sample_info_t sample_info = {
        .bits_per_sample = 16,
        .channel = 1,
        .channel_mask = 0,
        .sample_rate = (uint32_t)sample_rate,
        .mclk_multiple = 0,
    };

    int ret = esp_codec_dev_open(s_codec, &sample_info);
    if (ret != 0) {
        ESP_LOGE(TAG, "esp_codec_dev_open failed: %d", ret);
        return ESP_FAIL;
    }

    ret = esp_codec_dev_set_out_vol(s_codec, atomic_load(&s_volume));
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to set initial volume: %d", ret);
        esp_codec_dev_close(s_codec);
        return ESP_FAIL;
    }

    atomic_store(&s_volume_dirty, false);
    s_codec_open = true;
    s_codec_rate = sample_rate;
    ESP_LOGI(TAG, "Codec opened at %d Hz, mono", sample_rate);
    return ESP_OK;
}

void audio_player_adjust_volume(int delta)
{
    int volume = atomic_load(&s_volume) + delta;
    if (volume < 0) {
        volume = 0;
    } else if (volume > 100) {
        volume = 100;
    }
    atomic_store(&s_volume, volume);
    atomic_store(&s_volume_dirty, true);
}

int audio_player_get_volume(void)
{
    return atomic_load(&s_volume);
}

static void apply_pending_volume(void)
{
    if (s_codec != NULL && s_codec_open && atomic_exchange(&s_volume_dirty, false)) {
        int volume = atomic_load(&s_volume);
        int ret = esp_codec_dev_set_out_vol(s_codec, volume);
        if (ret != 0) {
            ESP_LOGE(TAG, "Failed to set volume: %d", ret);
            atomic_store(&s_volume_dirty, true);
        }
    }
}

static esp_err_t write_frame(const MP3FrameInfo *info)
{
    if (info->outputSamps <= 0 || info->nChans <= 0) {
        return ESP_OK;
    }

    esp_err_t ret = codec_configure(info->samprate);
    if (ret != ESP_OK) {
        return ret;
    }

    apply_pending_volume();

    int write_ret;
    if (info->nChans == 2) {
        int frames = info->outputSamps / 2;
        for (int index = 0; index < frames; index++) {
            int32_t left = s_pcm_buf[2 * index];
            int32_t right = s_pcm_buf[2 * index + 1];
            s_mono_buf[index] = (int16_t)((left + right) / 2);
        }
        write_ret = esp_codec_dev_write(s_codec, s_mono_buf, frames * (int)sizeof(int16_t));
    } else {
        write_ret = esp_codec_dev_write(s_codec, s_pcm_buf,
                                        info->outputSamps * (int)sizeof(int16_t));
    }

    if (write_ret != 0) {
        ESP_LOGE(TAG, "Codec write failed: %d", write_ret);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t decode_file(FILE *file)
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

            size_t wanted = AUDIO_READ_BUF_SIZE - (size_t)bytes_left;
            size_t received = fread(s_read_buf + bytes_left, 1U, wanted, file);
            bytes_left += (int)received;
            if (received < wanted) {
                if (ferror(file)) {
                    ESP_LOGE(TAG, "Failed to read MP3 stream");
                    return ESP_FAIL;
                }
                eof = true;
            }
        }

        if (bytes_left <= 0) {
            break;
        }

        int offset = MP3FindSyncWord(read_ptr, bytes_left);
        if (offset < 0) {
            bytes_left = 0;
            if (eof) {
                break;
            }
            continue;
        }
        read_ptr += offset;
        bytes_left -= offset;

        int decode_ret = MP3Decode(s_decoder, &read_ptr, &bytes_left, s_pcm_buf, 0);
        if (decode_ret == 0) {
            MP3FrameInfo frame_info;
            MP3GetLastFrameInfo(s_decoder, &frame_info);
            esp_err_t ret = write_frame(&frame_info);
            if (ret != ESP_OK) {
                return ret;
            }
        } else {
            if (bytes_left > 0) {
                read_ptr++;
                bytes_left--;
            }
            if (bytes_left <= 0 && eof) {
                break;
            }
        }
    }

    return ESP_OK;
}

esp_err_t audio_player_play_file(const char *path)
{
    if (path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open %s", path);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Playing %s", path);
    esp_err_t ret = decode_file(file);
    fclose(file);
    return ret;
}

esp_err_t audio_player_init(void)
{
    if (s_codec != NULL && s_decoder != NULL) {
        return ESP_OK;
    }

#if defined(SALARY_CAT_BOARD_AMOLED_175)
    if (bsp_io_expander_init() == NULL) {
        ESP_LOGE(TAG, "Failed to initialize TCA9554 for the 1.75 audio path");
        return ESP_FAIL;
    }
#endif

    s_codec = bsp_audio_codec_speaker_init();
    if (s_codec == NULL) {
        ESP_LOGE(TAG, "Failed to initialize speaker codec");
        return ESP_FAIL;
    }

    s_decoder = MP3InitDecoder();
    if (s_decoder == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MP3 decoder");
        esp_codec_dev_delete(s_codec);
        s_codec = NULL;
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t audio_player_deinit(void)
{
    esp_err_t ret = ESP_OK;
    if (s_codec_open && esp_codec_dev_close(s_codec) != 0) {
        ESP_LOGE(TAG, "Failed to close codec");
        ret = ESP_FAIL;
    } else {
        s_codec_open = false;
    }

    if (s_decoder != NULL) {
        MP3FreeDecoder(s_decoder);
        s_decoder = NULL;
    }
    if (s_codec != NULL) {
        esp_codec_dev_delete(s_codec);
        s_codec = NULL;
    }
    s_codec_rate = 0;
    return ret;
}