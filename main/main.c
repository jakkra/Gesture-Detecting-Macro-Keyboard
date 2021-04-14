#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "tf_gesture_predictor.h"
#include "touchpad_sensor.h"

#include "horizontal.h"
#include "vertical.h"
#include "v.h"

//#define TRAINING

static void runPrintTrainData(void);
static char* getNameOfPrediction(gesture_label_t prediction);

void app_main(void) {
  gesture_prediction_t prediction;
  float* in_matrix;

  ESP_ERROR_CHECK(touchpad_sensor_init());

#ifdef TRAINING
  runPrintTrainData();
  // Never returns
  assert(false);
#endif
  ESP_ERROR_CHECK(tf_gesture_predictor_init());
  
  int64_t start = esp_timer_get_time();
  tf_gesture_predictor_run(vertical, sizeof(vertical), &prediction, true);
  printf("Prediction took %d\n", (int)(esp_timer_get_time() - start) / 1000);

  while (true) {
    in_matrix = touchpad_sensor_fetch();
    if (in_matrix) {
      tf_gesture_predictor_run(in_matrix, 28 * 28 * sizeof(float), &prediction, true);
      printf("Prediction: %s, prob: %f\n", getNameOfPrediction(prediction.label),  prediction.probability);
    } else {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

static void runPrintTrainData(void) {
  while (true) {
    if (!touchpad_sensor_print_raw()) {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

static char* getNameOfPrediction(gesture_label_t prediction)
{
  switch(prediction) {
    case LABEL_LINE_DOWN_GESTURE: return "LABEL_LINE_DOWN_GESTURE";
    case LABEL_LINE_HORIZONTAL_GESTURE: return "LABEL_LINE_HORIZONTAL_GESTURE";
    case LABEL_O_GESTURE: return "LABEL_O_GESTURE";
    case LABEL_V_GESTURE: return "LABEL_V_GESTURE";
    default: return "UNKNOWN_PREDICTION";
  }
}