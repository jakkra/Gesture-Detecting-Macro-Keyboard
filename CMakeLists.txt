cmake_minimum_required(VERSION 3.5)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(gesture_detector)

# add_custom_command to automatically copy the latest model from output folder before every build
add_custom_command(
    TARGET app PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/training/output/gesture_model_tflite.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/main/gesture_model_tflite.cc)

add_custom_command(
    TARGET app PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/training/output/gesture_model_tflite.h
            ${CMAKE_CURRENT_SOURCE_DIR}/main/gesture_model_tflite.h)

# configure_file also needed for the initial copy otherwise CMAKE complains.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/training/output/gesture_model_tflite.cc ${CMAKE_CURRENT_SOURCE_DIR}/main/gesture_model_tflite.cc COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/training/output/gesture_model_tflite.h ${CMAKE_CURRENT_SOURCE_DIR}/main/gesture_model_tflite.h COPYONLY)
