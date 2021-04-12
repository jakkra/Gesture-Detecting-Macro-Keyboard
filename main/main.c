#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "tf_gesture_predictor.h"


#include "horizontal.h"
#include "vertical.h"
#include "v.h"

void app_main(void) {
  gesture_prediction_t prediction;
  tf_gesture_predictor_init();
  while (true) {
    int64_t start = esp_timer_get_time();
    tf_gesture_predictor_run(vertical, sizeof(vertical), &prediction, true);
    printf("Invoke DONE took %d\n", (int)(esp_timer_get_time() - start) / 1000);
    tf_gesture_predictor_run(horizontal, sizeof(horizontal), &prediction, true);
    tf_gesture_predictor_run(v_shape, sizeof(v_shape), &prediction, true);
    vTaskDelay(10000 / portTICK_RATE_MS);
  }
}
