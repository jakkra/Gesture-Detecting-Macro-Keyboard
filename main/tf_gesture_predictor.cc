#include "tf_gesture_predictor.h"

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "gesture_model_tflite.h"
#include "Trill.h"

#include "esp_err.h"
#include "esp_log.h"

static const char* TAG = "TF_GESTURE_PREDICTOR";

namespace {
  tflite::ErrorReporter* error_reporter = nullptr;
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* model_input = nullptr;
  TfLiteTensor* output = nullptr;
  float * input = nullptr;
  constexpr int kTensorArenaSize = 35 * 1024;
  uint8_t tensor_arena[kTensorArenaSize];
  int input_length_bytes;
}  // namespace

esp_err_t tf_gesture_predictor_init(void) {
  tflite::InitializeTarget();

  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  model = tflite::GetModel(gesture_model_tflite_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return ESP_FAIL;
  }

  static tflite::AllOpsResolver resolver;

  // Build an interpreter to run the model with.
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return ESP_FAIL;
  }

  // Obtain pointers to the model's input and output tensors.
  model_input = interpreter->input(0);
  input = interpreter->input(0)->data.f;
  output = interpreter->output(0);

  int input_length = model_input->bytes / sizeof(float);

  ESP_LOGI(TAG, "Input matrix size: %dx%dx%d", model_input->dims->data[0], model_input->dims->data[1], model_input->dims->data[2]);
  
  ESP_LOGI(TAG, "Input_length: %d", input_length);
  ESP_LOGI(TAG, "Input element size: %d", model_input->dims->size);
  ESP_LOGI(TAG, "Input size in bytes: %d", model_input->bytes);
  ESP_LOGI(TAG, "Output size: %d", output->dims->size);
  ESP_LOGI(TAG, "Num gestures: %d",output->dims->data[1]);

  input_length_bytes = model_input->bytes;

  return ESP_OK;
}

esp_err_t tf_gesture_predictor_run(float* input_data, int data_length, gesture_prediction_t *p_result, bool print_input) {
  TfLiteStatus invoke_status;
  float max = 0;
  gesture_label_t label;

  assert(data_length == input_length_bytes);
  
  memcpy(input, input_data, data_length);
  if (print_input) {
    for (int row = 0; row < 28 * 28; row++) {
      if (row % 28 == 0) {
        printf("\n");
      }
      printf("%d ", (int)input[row]);
    }
    printf("\n");
  }
  invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    ESP_LOGE(TAG, "Invoke ERROR %d\n", invoke_status);
    return ESP_FAIL;
  }

  float * predictions = interpreter->output(0)->data.f;
  for (int gesture = 0; gesture < LABEL_END; gesture++) {
    if (predictions[gesture] > max) {
      label = (gesture_label_t)gesture;
      max = predictions[gesture];
    }
  }
  
  ESP_LOGI(TAG, "Gesture: %d, probability: %f", (int)label, max);
  ESP_LOGI(TAG, "Predicted: %f, %f, %f, %f", predictions[0], predictions[1], predictions[2], predictions[3]);

  p_result->label = label;
  p_result->probability = max;

  return ESP_OK;
}

const char* tf_gesture_predictor_get_name(gesture_label_t label) {
  return getNameOfPrediction(label);
}
