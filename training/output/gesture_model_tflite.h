/*
This is a automatically generated TensorFlow Lite model by train_gestures.py, see README.md for more info.
It is converted into a C data array using xxd and is defined in gesture_model_tflite.cc
*/
#ifndef TENSORFLOW_LITE_GESTURE_MODEL_H_
#define TENSORFLOW_LITE_GESTURE_MODEL_H_
extern const unsigned char gesture_model_tflite_data[];

typedef enum gesture_label_t 
{
	LABEL_C_GESTURE = 0,
	LABEL_ARROW_RIGHT = 1,
	LABEL_ARROW_UP = 2,
	LABEL_LINE_DOWN_GESTURE = 3,
	LABEL_LINE_HORIZONTAL_GESTURE = 4,
	LABEL_S_GESTURE = 5,
	LABEL_V_GESTURE = 6,
	LABEL_END
} gesture_label_t;

static inline const char* getNameOfPrediction(gesture_label_t prediction)
{
	switch (prediction) {
		case LABEL_C_GESTURE: return "LABEL_C_GESTURE";
		case LABEL_ARROW_RIGHT: return "LABEL_ARROW_RIGHT";
		case LABEL_ARROW_UP: return "LABEL_ARROW_UP";
		case LABEL_LINE_DOWN_GESTURE: return "LABEL_LINE_DOWN_GESTURE";
		case LABEL_LINE_HORIZONTAL_GESTURE: return "LABEL_LINE_HORIZONTAL_GESTURE";
		case LABEL_S_GESTURE: return "LABEL_S_GESTURE";
		case LABEL_V_GESTURE: return "LABEL_V_GESTURE";
		default: return "UNKNOWN_PREDICTION";
	}
}

#endif  // TENSORFLOW_LITE_GESTURE_MODEL_H_
