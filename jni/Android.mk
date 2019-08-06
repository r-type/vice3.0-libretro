LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..

COMMONFLAGS :=
INCFLAGS    :=

EMUTYPE     ?= x64

include $(CORE_DIR)/Makefile.common

COREFLAGS := -DCORE_NAME=\"$(EMUTYPE)\" \
  -D__LIBRETRO__ \
  -DFRONTEND_SUPPORTS_RGB565 \
  $(INCFLAGS) $(COMMONFLAGS) \
  -DHAVE_INET_ATON \
  -DWANT_ZLIB

SOURCES_C += $(LIBRETRO_COMM_DIR)/vfs/vfs_implementation.c $(LIBRETRO_COMM_DIR)/file/file_path.c $(LIBRETRO_COMM_DIR)/compat/compat_strcasestr.c

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

ifeq ($(TARGET_ARCH),arm)
  COREFLAGS += -DARM_ARCH
endif

include $(CLEAR_VARS)
LOCAL_MODULE       := retro
LOCAL_SRC_FILES    := $(SOURCES_C) $(SOURCES_CC)
LOCAL_CPPFLAGS     := $(COREFLAGS)
LOCAL_CFLAGS       := $(COREFLAGS)
LOCAL_LDFLAGS      := -Wl,-version-script=$(CORE_DIR)/libretro/link.T -ldl
LOCAL_LDLIBS       := -llog
LOCAL_CPP_FEATURES := exceptions
include $(BUILD_SHARED_LIBRARY)
