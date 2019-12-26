LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= EncoderTest.c \

MY_TOP=$(LOCAL_PATH)/..
cedx=$(LOCAL_PATH)/../frameworks/av/media/libcedarx

$(info $(LOCAL_PATH))
$(info $(TOP))

LOCAL_C_INCLUDES := \
	$(cedx)\
	$(cedx)/ \
	$(cedx)/libcore/base/include/      \
	$(cedx)/CODEC/VIDEO/ENCODER/include/ \
	$(cedx)/libcore/playback/include  \
	$(cedx)/external/include/adecoder \
	$(MY_TOP)/frameworks/av/media/libcedarc/include \


LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libVE \
	libMemAdapter \
	libvencoder \
	libcdx_base

LOCAL_MODULE:= awenc

#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_EXECUTABLE)
