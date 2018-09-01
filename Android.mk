LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := tinyserial.c
LOCAL_MODULE := tinyserial
include $(BUILD_EXECUTABLE)
