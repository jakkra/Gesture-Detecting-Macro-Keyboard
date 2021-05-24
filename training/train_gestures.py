import tensorflow as tf
import numpy as np
from keras.datasets import mnist
from keras.utils import to_categorical
from keras.models import Sequential
from keras.layers import Input
from keras.layers import Conv2D
from keras.layers import MaxPooling2D
from keras.layers import Dense
from keras.layers import Flatten
from keras.optimizers import SGD

from matplotlib import pyplot as plt
import os
from pathlib import Path

train_dir = os.path.dirname(os.path.realpath(__file__)) + "/train_data/"
output_dir = os.path.dirname(os.path.realpath(__file__)) + "/output/"

def create_img_from_line(line):
	line = line.replace(' ', '')
	line = line.replace('\'', '')
	line = line.replace('[', '')
	line = line.replace(']', '')
	line = line.replace('\n', '')
	line = line.split(',')
	img = np.zeros((28, 28))
	for i in range(0, len(line), 2):
		img[int(line[i + 1])//64 - 1, int(line[i])//64 - 1] = 1
	img = img.reshape(1, 28, 28, 1)
	img = img.astype('float32')
	return img

def load_manual_dataset():
	files = os.listdir(train_dir)
	num_gestures = len(files) / 2
	print("Found files {0} => {1} gestures".format(files, num_gestures))
	trainX = []
	testX = []
	trainY = []
	testY = []
	file_index = 0
	gestures = {}

	# Assumes the train/test data folder is not modified by user.
	# It should be generated using collect_train_data.py
	for file_name in files:
		gesture_name = file_name.split('.')[0]
		if (gesture_name not in gestures):
			gestures[gesture_name] = len(gestures)

		gesture_id = gestures[gesture_name]

		if (file_name.endswith('.train')):
			train_file = open(train_dir + file_name, "r")
			train_lines = train_file.readlines()

			for line in train_lines:
				trainX.append(create_img_from_line(line))
				trainY.append(gesture_id)

		elif file_name.endswith('.test'):
			test_file = open(train_dir + file_name, "r")
			test_lines = test_file.readlines()

			for line in test_lines:
				testX.append(create_img_from_line(line))
				testY.append(gesture_id)
		
	trainX = np.array(trainX)
	trainY = np.array(trainY)
	testX = np.array(testX)
	testY = np.array(testY)

	trainX = trainX.reshape((trainX.shape[0], 28, 28, 1))
	testX = testX.reshape((testX.shape[0], 28, 28, 1))
	
	print('Done creating test and train matrices')
	print(len(testX), len(testY))
	print(len(trainX), len(trainY))
	print(gestures)

	trainY = to_categorical(trainY)
	testY = to_categorical(testY)
	return trainX, trainY, testX, testY, gestures

# define CNN model
def define_model(num_gestures):
	model = Sequential()
	model.add(Input(shape=(28, 28, 1)))
	model.add(Conv2D(8, (3, 3), activation='relu', kernel_initializer='he_uniform'))
	model.add(MaxPooling2D((2, 2)))
	model.add(Conv2D(16, (3, 3), activation='relu', kernel_initializer='he_uniform'))
	model.add(Conv2D(16, (3, 3), activation='relu', kernel_initializer='he_uniform'))
	model.add(MaxPooling2D((2, 2)))
	#tf.keras.layers.Dropout(0.1) # Dropout may help with performance, but seems ok without so skip for now.
	model.add(Flatten())
	model.add(Dense(16, activation='relu', kernel_initializer='he_uniform'))
	#tf.keras.layers.Dropout(0.1)
	model.add(Dense(num_gestures, activation='softmax'))
	# compile model
	opt = SGD(lr=0.001, momentum=0.9) # TODO Experiment with lr if needed, does not look like it right now
	model.compile(optimizer=opt, loss='categorical_crossentropy', metrics=['accuracy'])
	print("hello")
	return model
 
if __name__ == "__main__":
	# Create folder for the generated files
	Path(output_dir).mkdir(parents=True, exist_ok=True)
	print('create', output_dir)
	trainX, trainY, testX, testY, gesture_map = load_manual_dataset()
	print('Loaded train data shape is {0} and test data shape is {1}'.format(trainX.shape, testX.shape))

	# define model
	model = define_model(len(gesture_map))
	#model.load_weights('good_model_weights.h5')
	# fit model
	history = model.fit(trainX, trainY, epochs=100, batch_size=10, verbose=1, validation_data=(testX, testY))
	model.save_weights('good_model_weights.h5')

	plt.plot(history.history['accuracy'])
	plt.plot(history.history['val_accuracy'])
	plt.title('model accuracy')
	plt.ylabel('accuracy')
	plt.xlabel('epoch')
	plt.legend(['train', 'val'], loc='upper left')

	plt.figure()
	plt.plot(history.history['loss'])
	plt.plot(history.history['val_loss'])
	plt.title('model loss')
	plt.ylabel('loss')
	plt.xlabel('epoch')
	plt.legend(['train', 'val'], loc='upper left')
	plt.show()
	# save model
	model.save(output_dir + 'tf_gesture_model.h5')

	# Convert the model to Tensorflow Lite format.
	converter = tf.lite.TFLiteConverter.from_keras_model(model)
	tflite_model = converter.convert()

	# Save the model.
	with open(output_dir + 'gesture_model.tflite', 'wb') as f:
		f.write(tflite_model)
	
	# Convert to C++ file
	res = os.system("xxd -i {0}/gesture_model.tflite > {0}/gesture_model_tflite.cc".format(output_dir))

	# Rename the model, xxd gives it a name that contains the directory path also make the model const
	cppModelFileContentLines = []
	with open(output_dir + 'gesture_model_tflite.cc', 'r') as f:
		cppModelFileContentLines = f.readlines()
	os.remove(output_dir + 'gesture_model_tflite.cc')
	cppModelFileContentLines[0] = 'const unsigned char gesture_model_tflite_data[] = {\n'
	cppModelFileContentLines.insert(0, '#include "gesture_model_tflite.h"\n')
	with open(output_dir + 'gesture_model_tflite.cc', 'w') as f:
		f.writelines(cppModelFileContentLines)
	
	# Create header file to access generated model
	# Append to the header file the map between TF result and the actual gesture
	with open(output_dir + 'gesture_model_tflite.h', 'w') as f:
		f.write('/*\nThis is a automatically generated TensorFlow Lite model by train_gestures.py, see README.md for more info.\nIt is converted into a C data array using xxd and is defined in gesture_model_tflite.cc\n*/\n')
		f.write('#ifndef TENSORFLOW_LITE_GESTURE_MODEL_H_\n')
		f.write('#define TENSORFLOW_LITE_GESTURE_MODEL_H_\n')
		f.write('extern const unsigned char gesture_model_tflite_data[];\n')

		f.write('\ntypedef enum gesture_label_t \n{\n')
		for idx, key in enumerate(gesture_map.keys()):
			f.write('\tLABEL_{0} = {1},\n'.format(key.upper(), idx))
		f.write('\tLABEL_END\n')
		f.write('} gesture_label_t;\n')

		f.write('\n')

		# Generate prediction to string
		f.write('static inline const char* getNameOfPrediction(gesture_label_t prediction)\n')
		f.write('{\n\tswitch (prediction) {\n')
		for idx, key in enumerate(gesture_map.keys()):
			f.write('\t\tcase LABEL_{0}: return \"LABEL_{1}\";\n'.format(key.upper(), key.upper()))
		f.write('\t\tdefault: return "UNKNOWN_PREDICTION";\n')
		f.write('\t}\n')
		f.write('}\n\n')

		f.write('#endif  // TENSORFLOW_LITE_GESTURE_MODEL_H_\n')
	
	# Generate labels.txt to be able to display more friendly output when testing the model on the computer.
	with open(output_dir + 'labels.txt', 'w') as f:
		for idx, key in enumerate(gesture_map.keys()):
			f.write('{0}:{1}\n'.format(idx, key.upper()))

	print('Finished, c++ model generated in gesture_model_tflite.cc')


