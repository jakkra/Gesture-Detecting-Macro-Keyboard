#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "gesture_model_tflite.h"
#include "esp_err.h"

typedef struct gesture_prediction_t {
    gesture_label_t label;
    float           probability;
} gesture_prediction_t;

esp_err_t tf_gesture_predictor_init(void);
esp_err_t tf_gesture_predictor_run(float* input_data, int data_length, gesture_prediction_t *p_result, bool print_input);
const char* tf_gesture_predictor_get_name(gesture_label_t label);

#ifdef __cplusplus
}
#endif
