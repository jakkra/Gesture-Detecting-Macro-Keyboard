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

maxCoord = 1792

def showImg(data, block=False):
    img = np.zeros((28, 28))
    for i in range(0, len(data), 2):
        img[max(0, (int(data[i + 1]) - 1)//64), max(0, (int(data[i])//64) - 1)] = 1
    plt.imshow(img, interpolation='nearest')
    #img = img.reshape(1, 28, 28, 1)
    #img = img.astype('float32')
    return img

def dataStringToXandY(gestureStr):
    gestureStr = gestureStr.replace('[', '')
    gestureStr = gestureStr.replace('\'', '')
    gestureStr = gestureStr.replace(']', '')
    gestureStr = gestureStr.replace(' ', '')
    gestureStr = gestureStr.split(',')
    return [int(item) for item in gestureStr]

def moveToCenter(img):
    minRow = 100
    maxRow = -1
    minCol = 100
    maxCol = -1
    for rowcol, val in np.ndenumerate(img):
        col = rowcol[0]
        row = rowcol[1]
        if (val > 0):
            if (row < minRow):
                minRow = row
            if (row > maxRow):
                maxRow = row
            
            if (col < minCol):
                minCol = col
            if (col > maxCol):
                maxCol = col
    print("minRow: {}, minCol: {}".format(minRow, minCol))
    print("maxRow: {}, maxCol: {}".format(maxRow, maxCol))

    diffRow = (maxRow - minRow) // 2
    diffCol = (maxCol - minCol) // 2
    print("diffRow: {}, diffCol: {}".format(diffRow, diffCol))
    centeredImg = np.zeros((28, 28))
    centeredImg[minCol, minRow] = 1
    centeredImg[maxCol, maxRow] = 1
    centeredImg[diffCol + minCol, diffRow + minRow] = 1

    rowOffset = 14 - (diffRow + minRow)
    colOffset = 14 - (diffCol + minCol)
    print(rowOffset, colOffset)
    plt.subplot(1, 3, 2)
    plt.imshow(centeredImg, interpolation='nearest')
    plt.show(block=True)
    plt.close("all")
    #plt.subplot(1, 3, 3)
    movedImg = np.zeros((28, 28))

    for rowcol, val in np.ndenumerate(img):
        y = rowcol[0]
        x = rowcol[1]
        if (img[y, x] > 0):
            movedImg[min(max(y + colOffset, 0), 27), min(max(x + rowOffset, 0), 27)] = 1
    
    return movedImg

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Create and store training data')
    parser.add_argument('--output_file ', type=str, required=True, help='Name of gesture must be unique')
    parser.add_argument('--input_data', type=str, required=True, help='Name of the data input file')
    parser.add_argument('--test_data', default=False, action='store_true', help='If data shall be used as test and not train data, only changes output file name.')


    args = parser.parse_args()
    new_gestures = []
    with open(args.input_data) as fp:
        lines = fp.readlines()
        lines = lines[:6]
        for gesture in lines:
            print(gesture)
            gesture = dataStringToXandY(gesture)
            plt.figure(figsize=(16, 8), dpi=80)
            plt.subplot(1, 3, 1)
            img = showImg(gesture)
            plt.imshow(img, interpolation='nearest')
            newImg = moveToCenter(img)
            plt.subplot(1, 2, 2)
            plt.imshow(newImg, interpolation='nearest')
            plt.show(block=True)
            plt.close("all")

            #img = np.zeros((1793, 1793))
            #for i in range(0, len(gesture), 2):
            #    img[gesture[i], gesture[i + 1]] = 1
            ## TODO use numpy functions as roll etc
            ##...
#
            ## Then convert back
            #updatedGesture = '['
            #lens = 0
            #for x in range(0, 1792):
            #    for y in range(0, 1792):
            #        if (img[x, y] == 1):
            #            lens = lens + 1
            #            updatedGesture += "\'{}\', \'{}', ".format(x, y)
            #print('LEN', lens)
            #updatedGesture = updatedGesture[:-2] + ']'
            #print(updatedGesture)
            #newGesture = dataStringToXandY(gesture)
#
            #plt.subplot(1, 2, 2)
            #showImg(newGesture, True)
            #plt.show(block=False)
            #plt.pause(1)
            #plt.close("all")
            #new_gestures.append(updatedGesture)
        
        print(len(new_gestures))


    #while True:
    #    try:
    #        data = ser.readline()
    #        if len(data) > 0:
    #            data = data.decode("utf-8")
    #            data = data.split('START:')
    #            if len(data) > 1:
    #                data = data[1][:-3].split(',')
    #                raw_data = data
    #                if len(data) > 20:
    #                    img = np.zeros((28, 28))
    #                    for i in range(0, len(data), 2):
    #                        img[max(0, (int(data[i + 1]) - 1)//64), max(0, (int(data[i])//64) - 1)] = 1
    #                    plt.imshow(img, interpolation='nearest')
    #                    plt.show(block=False)
    #                    plt.pause(0.1)
    #                    plt.close("all")
    #                    img = img.reshape(1, 28, 28, 1)
    #                    img = img.astype('float32')
    #                    training_data.append(raw_data)
    #                    sameple_number = sameple_number + 1
    #                    print('Sample {0} added'.format(sameple_number))
    #    except KeyboardInterrupt:
    #        print('EXIT')
    #        sys.exit()