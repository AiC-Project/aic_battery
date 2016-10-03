LOCAL_PATH := $(call my-dir)

# C++ battery protobuf parsing
include $(CLEAR_VARS)
IGNORED_WARNINGS := -Wno-sign-compare -Wno-unused-parameter -Wno-sign-promo -Wno-error=return-type
LOCAL_CFLAGS     := -DLOG_TAG=\"protobuf_battery\" -O2 -DGOOGLE_PROTOBUF_NO_RTTI
LOCAL_STATIC_LIBRARIES := liblog libcutils libstlport_static libprotobuf-cpp-2.3.0-full libcppsensors_packet_static
LOCAL_C_INCLUDES := \
					bionic \
					external/stlport/stlport \
					external/protobuf/src \
					external/aic/libaicd

LOCAL_SRC_FILES := read_battery_proto.cpp
LOCAL_MODULE := libbattery_proto
LOCAL_MODULE_TAGS := optional
include $(BUILD_STATIC_LIBRARY)

# C battery stubs
include $(CLEAR_VARS)
LOCAL_CFLAGS           := -DLOG_TAG=\"battery_sensor\"
LOCAL_C_INCLUDES       := external/aic/libaicd
LOCAL_STATIC_LIBRARIES := liblog libcutils libbattery_proto
LOCAL_SRC_FILES        := battery_sensor.c
LOCAL_MODULE           := libaic_battery
LOCAL_MODULE_TAGS      := optional
include $(BUILD_STATIC_LIBRARY)

