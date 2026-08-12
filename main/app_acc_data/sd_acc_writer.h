#ifndef SD_ACC_WRITER_H
#define SD_ACC_WRITER_H

#include <stdbool.h>

#include "esp_err.h"
#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SD_ACC_SAMPLE_COUNT 3000U

esp_err_t sd_acc_writer_init(void);
bool sd_acc_writer_request_start(void);
bool sd_acc_writer_request_stop(void);
bool sd_acc_writer_post_sample(const acc_data_sample_t * sample);

#ifdef __cplusplus
}
#endif

#endif /* SD_ACC_WRITER_H */