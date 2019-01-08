LOCAL_PATH := $(call my-dir)

#ffmpeg lib
include $(CLEAR_VARS)
LOCAL_MODULE := avcodec
LOCAL_SRC_FILES := ffmpeg/libavcodec-56.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avdevice
LOCAL_SRC_FILES := ffmpeg/libavdevice-56.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avfilter
LOCAL_SRC_FILES := ffmpeg/libavfilter-5.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avformat
LOCAL_SRC_FILES := ffmpeg/libavformat-56.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avutil
LOCAL_SRC_FILES := ffmpeg/libavutil-54.so
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := postproc
LOCAL_SRC_FILES := ffmpeg/libpostproc-53.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := swresample
LOCAL_SRC_FILES := ffmpeg/libswresample-1.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := swscale
LOCAL_SRC_FILES := ffmpeg/libswscale-3.so
include $(PREBUILT_SHARED_LIBRARY)

#libyuv

include $(CLEAR_VARS)
LOCAL_MODULE := yuv
LOCAL_SRC_FILES := libyuv/libyuv.so
include $(PREBUILT_SHARED_LIBRARY)

#myapp
include $(CLEAR_VARS)
LOCAL_MODULE := myffmpeg
LOCAL_SRC_FILES := ffmpeg.cpp syn_audio_video.cpp Queue.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/ffmpeg/include 
LOCAL_C_INCLUDES += $(LOCAL_PATH)/libyuv/include 
LOCAL_LDLIBS := -llog -landroid
LOCAL_SHARED_LIBRARIES := avcodec avdevice avfilter avformat avutil postproc swresample swscale yuv
LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS
include $(BUILD_SHARED_LIBRARY)

