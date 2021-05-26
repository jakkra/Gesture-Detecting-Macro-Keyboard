# Gesture detection

## Training
For each gesture `collect_train_data.py` needs to run twice, first to collect train data and then to collect test data. For a limited amount of gestures about 100 train samples and about 20 test samples seems to be enough.

    python collect_train_data.py --port COMX --gesture_name v_shape
    python collect_train_data.py --port COMX --gesture_name v_shape --test_data

After each gesture a plot of the input data will be shown, if it looks bad because you made a misstake, then just press ENTER in the console and the last sample will be removed from the data.

When collection of sample data is finished run `python train_gestures.py --center_gesture true/false` it will take about 30s depending on computer. This script will generate a .c and a .h file containing the TF Lite model that can be imported by embedded TF Lite. The .h file also contains the map between predictions (int) and their meaning.

When training is done you can verify the model by running `python test_model.py --port COMX --center_gesture true/false` and watch the predictions.

### `--center_gesture`
Option to center the gesture in the input matrix, this will help when the training data is drawn at a specific location on the trackpad. However this will remove functionality to have for example two line down gestures, one at the right side and one at the left side of the trackpad as this function will move it to the center. For this to work the same change is needed on target before inputing the drawn gesture to the embedded model.

Depending if this is enabled or not, the `#define CENTER_GESTURE 1` in main.c needs to match. TODO: append this to `gesture_model_tflite.h` and this manual step will not be needed anymore.

