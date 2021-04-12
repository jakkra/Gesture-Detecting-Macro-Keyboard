# Gesture detection

## Training
For each gesture `collect_train_data.py` needs to run twice, first to collect train data and then to collect test data. For a limited amount of gestures about 100 train samples and about 20 test samples seems to be enough.

    python collect_train_data.py --port COMX --gesture_name v_shape
    python collect_train_data.py --port COMX --gesture_name v_shape --test_data

After each gesture a plot of the input data will be shown, if it looks bad because you made a misstake, then just press ENTER in the console and the last sample will be removed from the data.

When collection of sample data is finished run `python train_gestures.py` it will take about 30s depending on computer. This script will generate a .c and a .h file containing the TF Lite model that can be imported by embedded TF Lite. The .h file also contains the map between predictions (int) and their meaning.

When training is done you can verify the model by running `python test_model.py --port COMX` and watch the predictions.
