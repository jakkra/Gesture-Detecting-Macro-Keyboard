from keras.preprocessing.image import load_img
from keras.preprocessing.image import img_to_array
from keras.models import load_model
from matplotlib import pyplot as plt
from keras.utils import plot_model

import tensorflow as tf
import numpy as np
import tflite_runtime.interpreter as tflite
import serial
import argparse
import os
import training_utils


output_dir = os.path.dirname(os.path.realpath(__file__)) + "/output/"

def load_labels(filename):
  with open(filename, 'r') as f:
    return [line.strip() for line in f.readlines()]

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Create and store training data')
    parser.add_argument('--port', type=str, required=True, help='COM port')
    parser.add_argument('--center_gesture', type=bool, required=True, help='Move the drawn gesture to the center of the matrix. Need to match if model was trained with centered data or not.')

    args = parser.parse_args()
    ser = serial.Serial(args.port, 115200, timeout=1)
    
    print('Loading TensorFlow model')
    model = load_model(output_dir + 'tf_gesture_model.h5')
    print(model.summary())

    print('Loading TensorFlow Lite model')
    interpreter = tflite.Interpreter(model_path=output_dir + 'gesture_model.tflite')
    interpreter.allocate_tensors()

    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    height = input_details[0]['shape'][1]
    width = input_details[0]['shape'][2]
    print("Input size:", height, width)
    print(input_details)

    while True:
        data = ser.readline()
        if len(data) > 0:
            data = data.decode("utf-8")
            data = data.split('START:')
            if len(data) > 1:
                data = data[1][:-3].split(',')
                img = np.zeros((28, 28))
                for i in range(0, len(data), 2):
                    img[int(data[i + 1])//64 - 1, int(data[i])//64 - 1] = 1
                if args.center_gesture:
                    img = training_utils.moveToCenter(img)
                plt.imshow(img, interpolation='nearest')
                plt.show(block=False)
                plt.pause(1)
                plt.close("all")
                img = img.reshape(1, 28, 28, 1)
                img = img.astype('float32')

                # Load the labels so we can display more friendly result
                labels = load_labels(output_dir + 'labels.txt')

                # Run both TF and RF Lite so we can see that convertion was OK
                # Show result from TensorFlow model
                print("TensorFlow Model:")

                results = model.predict(img)[0]
                top_k = results.argsort()[-5:][::-1]
                for i in top_k:
                    print('{:08.6f}: {}'.format(float(results[i]), labels[i]))

                # Show result from TensorFlow Lite model
                print("TensorFlow Model Lite:")

                interpreter.set_tensor(input_details[0]['index'], img)
                
                interpreter.invoke()

                output_data = interpreter.get_tensor(output_details[0]['index'])
                results = np.squeeze(output_data)

                top_k = results.argsort()[-5:][::-1]
                for i in top_k:
                    print('{:08.6f}: {}'.format(float(results[i]), labels[i]))

