/*
 * log.h
 *
 *  Created on: 2019-1-7
 *      Author: lenovo
 */

#ifndef LOG_H_
#define LOG_H_

#define LOG_TAG "Test_Jni"
#include <android/log.h>
// for native window JNI
#define MAX_AUDIO_FRME_SIZE (48000 * 4)

//用于打印debug级别的log信息
//__VA_ARGS__ 可变参数
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

//用于打印info级别的log信息
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

//用于打印error级别的log信息
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


#endif /* LOG_H_ */
