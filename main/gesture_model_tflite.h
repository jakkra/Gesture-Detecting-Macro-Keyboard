/*
This is a automatically generated TensorFlow Lite model by train_gestures.py, see README.md for more info.
It is converted into a C data array using xxd and is defined in gesture_model_tflite.cc
*/
#ifndef TENSORFLOW_LITE_GESTURE_MODEL_H_
#define TENSORFLOW_LITE_GESTURE_MODEL_H_
extern const unsigned char gesture_model_tflite_data[];
extern const unsigned int gesture_model_tflite_data_len;

typedef enum gesture_label_t 
{
	LABEL_LINE_DOWN_GESTURE = 0,
	LABEL_LINE_HORIZONTAL_GESTURE = 1,
	LABEL_O_GESTURE = 2,
	LABEL_V_GESTURE = 3,
	LABEL_END
} gesture_label_t;
#endif  // TENSORFLOW_LITE_GESTURE_MODEL_H_
