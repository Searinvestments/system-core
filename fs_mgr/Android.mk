#
# Copyright (C) 2014 MediaTek Inc.
# Modification based on code covered by the mentioned copyright
# and/or permission notice(s).
#
# Copyright 2011 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)

common_static_libraries := \
    liblogwrap \
    libfec \
    libfec_rs \
    libbase \
    libmincrypt \
    libcrypto_static \
    libext4_utils_static \
    libsquashfs_utils

include $(CLEAR_VARS)
LOCAL_CLANG := true
LOCAL_SANITIZE := integer
LOCAL_SRC_FILES:= \
    fs_mgr.c \
    fs_mgr_format.c \
    fs_mgr_fstab.c \
    fs_mgr_slotselect.c \
    fs_mgr_verity.cpp
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    system/vold \
    system/extras/ext4_utils \
    external/openssl/include \
    bootable/recovery
LOCAL_MODULE:= libfs_mgr
LOCAL_STATIC_LIBRARIES := $(common_static_libraries)
LOCAL_STATIC_LIBRARIES += libselinux
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_CFLAGS := -Werror
ifneq (,$(filter userdebug,$(TARGET_BUILD_VARIANT)))
LOCAL_CFLAGS += -DALLOW_ADBD_DISABLE_VERITY=1
endif

# Lenovo wuzb1 2017-02-28 INDRIYAPRO-106 Moto Build C
ifeq ($(MOT_TARGET_BUILD_ADDITIONAL_CONFIG),bldccfg)
LOCAL_CFLAGS += -DALLOW_ADBD_DISABLE_VERITY=1
LOCAL_CFLAGS += -DMOTO_BLD_C=1
endif

# add mtk fstab flags support
LOCAL_CFLAGS += -DMTK_FSTAB_FLAGS
# end
ifeq ($(LENOVO_RADIO_SECURE), yes)
LOCAL_CFLAGS += -DLENOVO_RADIO_SECURE=1
endif

ifeq ($(strip $(MTK_NAND_UBIFS_SUPPORT)),yes)
LOCAL_CFLAGS += -DMTK_UBIFS_SUPPORT
LOCAL_CFLAGS += -DBOARD_UBIFS_CACHE_VOLUME_SIZE=$(BOARD_UBIFS_CACHE_VOLUME_SIZE)

#add for ipoh
ifeq ($(MTK_IPOH_SUPPORT),yes)
LOCAL_CFLAGS += -DBOARD_UBIFS_IPOH_VOLUME_SIZE=$(BOARD_UBIFS_IPOH_VOLUME_SIZE)
endif
endif

ifeq ($(strip $(MTK_SLC_BUFFER_SUPPORT)),yes)
LOCAL_CFLAGS += -DMTK_SLC_BUFFER_SUPPORT
endif
ifeq ($(strip $(MTK_NAND_MTK_FTL_SUPPORT)),yes)
LOCAL_CFLAGS += -DMTK_FTL_SUPPORT
LOCAL_CFLAGS += -DBOARD_UBIFS_CACHE_VOLUME_SIZE=$(BOARD_UBIFS_CACHE_VOLUME_SIZE)
endif


ifeq ($(strip $(MTK_UFS_BOOTING)),yes)
LOCAL_CFLAGS += -DMTK_UFS_BOOTING
endif


include $(BUILD_STATIC_LIBRARY)

# ========================================================
# Shared library
# ========================================================
include $(CLEAR_VARS)
LOCAL_CLANG := true
LOCAL_SANITIZE := integer
LOCAL_SRC_FILES:= \
    fs_mgr.c \
    fs_mgr_format.c \
    fs_mgr_fstab.c \
    fs_mgr_slotselect.c \
    fs_mgr_verity.cpp
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    system/vold \
    system/extras/ext4_utils \
    external/openssl/include \
    bootable/recovery
LOCAL_MODULE:= libfs_mgr
LOCAL_STATIC_LIBRARIES := $(common_static_libraries)
LOCAL_STATIC_LIBRARIES += libcutils liblog libsparse_static libz libselinux
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_CFLAGS := -Werror
ifneq (,$(filter userdebug,$(TARGET_BUILD_VARIANT)))
LOCAL_CFLAGS += -DALLOW_ADBD_DISABLE_VERITY=1
endif
# add mtk fstab flags support
LOCAL_CFLAGS += -DMTK_FSTAB_FLAGS
# end

ifeq ($(strip $(MTK_NAND_UBIFS_SUPPORT)),yes)
LOCAL_CFLAGS += -DMTK_UBIFS_SUPPORT
LOCAL_CFLAGS += -DBOARD_UBIFS_CACHE_VOLUME_SIZE=$(BOARD_UBIFS_CACHE_VOLUME_SIZE)

#add for ipoh
ifeq ($(MTK_IPOH_SUPPORT),yes)
LOCAL_CFLAGS += -DBOARD_UBIFS_IPOH_VOLUME_SIZE=$(BOARD_UBIFS_IPOH_VOLUME_SIZE)
endif
endif
ifeq ($(strip $(MTK_SLC_BUFFER_SUPPORT)),yes)
LOCAL_CFLAGS += -DMTK_SLC_BUFFER_SUPPORT
endif
ifeq ($(strip $(MTK_NAND_MTK_FTL_SUPPORT)),yes)
LOCAL_CFLAGS += -DMTK_FTL_SUPPORT
LOCAL_CFLAGS += -DBOARD_UBIFS_CACHE_VOLUME_SIZE=$(BOARD_UBIFS_CACHE_VOLUME_SIZE)
endif

# Lenovo wuzb1 2017-01-23 INDRIYAPRO-106 Moto Build C
ifeq ($(MOT_TARGET_BUILD_ADDITIONAL_CONFIG),bldccfg)
LOCAL_CFLAGS += -DALLOW_ADBD_DISABLE_VERITY=1
LOCAL_CFLAGS += -DMOTO_BLD_C=1
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_CLANG := true
LOCAL_SANITIZE := integer
LOCAL_SRC_FILES:= fs_mgr_main.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_MODULE:= fs_mgr
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)/sbin
LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_UNSTRIPPED)
LOCAL_STATIC_LIBRARIES := libfs_mgr \
    $(common_static_libraries) \
    libcutils \
    liblog \
    libc \
    libsparse_static \
    libz \
    libselinux
LOCAL_CXX_STL := libc++_static
LOCAL_CFLAGS := -Werror
include $(BUILD_EXECUTABLE)
