import signal
import sys
import numpy as np
import serial
from matplotlib import pyplot as plt
import argparse
import threading
from pathlib import Path
import os

training_data = []
cwd = os.path.dirname(os.path.realpath(__file__)) + "/train_data/"

def signal_handler(sig, frame):
    print('Storing dataset...')
    for sample in training_data:
        f.write('{0}\n'.format(raw_data))
    f.close()
    print('Stored, exiting.')
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)



def console_input_thread(name):
    while(True):
        try:
            user_input = input()
            print('Deleted last sample')
            if (len(training_data) > 0):
                training_data.pop()
        except KeyboardInterrupt:
            sys.exit(0)
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Create and store training data')
    parser.add_argument('--port', type=str, required=True, help='COM port')
    parser.add_argument('--gesture_name', type=str, required=True, help='Name of gesture must be unique')
    parser.add_argument('--test_data', default=False, action='store_true', help='If data shall be used as test and not train data, only changes output file name.')

    print('Press CTRL+C when you are tired of collecting training data and the data will be stored.')
    print('Press ENTER in the console to remove the last sample in case you mess up.')

    args = parser.parse_args()
    Path(cwd).mkdir(parents=True, exist_ok=True)

    file_extention = '.train'
    if (args.test_data):
        file_extention = '.test'
    f = open('{0}{1}'.format(cwd + args.gesture_name, file_extention),"w")

    ser = serial.Serial(args.port, 115200, timeout=1)

    console_thread = threading.Thread(target=console_input_thread, args=(1,))
    console_thread.daemon = True
    console_thread.start()
    sameple_number = 0

    while True:
        try:
            data = ser.readline()
            if len(data) > 0:
                data = data.decode("utf-8")
                data = data.split('START:')
                if len(data) > 1:
                    data = data[1][:-3].split(',')
                    raw_data = data
                    if len(data) > 20:
                        img = np.zeros((28, 28))
                        for i in range(0, len(data), 2):
                            img[int(data[i + 1])//64 - 1, int(data[i])//64 - 1] = 1
                        plt.imshow(img, interpolation='nearest')
                        plt.show(block=False)
                        plt.pause(0.1)
                        plt.close("all")
                        img = img.reshape(1, 28, 28, 1)
                        img = img.astype('float32')
                        training_data.append(raw_data)
                        sameple_number = sameple_number + 1
                        print('Sample {0} added'.format(sameple_number))
        except KeyboardInterrupt:
            print('EXIT')
            sys.exit()