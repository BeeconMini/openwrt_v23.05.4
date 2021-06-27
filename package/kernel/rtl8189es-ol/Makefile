include $(TOPDIR)/rules.mk

PKG_NAME:=rtl8189es-ol
PKG_RELEASE=2

PKG_LICENSE:=GPLv2
PKG_LICENSE_FILES:=

PKG_SOURCE_URL:=https://github.com/openlumi/rtl8189es.git
PKG_MIRROR_HASH:=d6d26cfd93dcac3ed9f0e7915255f5337a296062041407034482c4618cd52db9
PKG_SOURCE_PROTO:=git
PKG_SOURCE_DATE:=2021-04-01
PKG_SOURCE_VERSION:=07df9757ffa854bf86b4dc8a99548bc1b5a21ee7

PKG_MAINTAINER:=Ivan Belokobylskiy <belokobylskij@gmail.com>
PKG_BUILD_PARALLEL:=1

STAMP_CONFIGURED_DEPENDS := $(STAGING_DIR)/usr/include/mac80211-backport/backport/autoconf.h

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

define KernelPackage/rtl8189es-ol
  SUBMENU:=Wireless Drivers
  TITLE:=Realtek RTL8189ES SDIO WiFi support (out-of-tree)
  DEPENDS:=+kmod-mac80211 +kmod-cfg80211 +@DRIVER_11N_SUPPORT
  FILES:=\
	$(PKG_BUILD_DIR)/rtl8189es.ko
  AUTOLOAD:=$(call AutoProbe,rtl8189es)
  PROVIDES:=kmod-rtl8189es
endef
define KernelPackage/rtl8189es-ol/config
  if PACKAGE_kmod-rtl8189es-ol

	config PACKAGE_RTL8189ES-OL_CONCURRENT
		bool "Enable concurrent mode"
		default y
  endif
endef

NOSTDINC_FLAGS := \
	$(KERNEL_NOSTDINC_FLAGS) \
	-I$(PKG_BUILD_DIR) \
	-I$(PKG_BUILD_DIR)/include \
	-I$(STAGING_DIR)/usr/include/mac80211-backport \
	-I$(STAGING_DIR)/usr/include/mac80211-backport/uapi \
	-I$(STAGING_DIR)/usr/include/mac80211 \
	-I$(STAGING_DIR)/usr/include/mac80211/uapi \
	-include backport/backport.h

NOSTDINC_FLAGS += \
		-DCONFIG_TRAFFIC_PROTECT \
		-DCONFIG_IOCTL_CFG80211 \
		-DRTW_USE_CFG80211_STA_EVENT \
		-DBUILD_OPENWRT \
  $(if $(CONFIG_PACKAGE_RTL8189ES-OL_CONCURRENT),-DCONFIG_CONCURRENT_MODE,-DRTW_P2P_GROUP_INTERFACE=1 -DRTW_DEDICATED_P2P_DEVICE=0)

ifeq ($(CONFIG_BIG_ENDIAN),y)
NOSTDINC_FLAGS+= -DCONFIG_BIG_ENDIAN
endif
ifneq ($(CONFIG_BIG_ENDIAN),y)
NOSTDINC_FLAGS+= -DCONFIG_LITTLE_ENDIAN
endif

define Build/Compile
	rm -f $(PKG_BUILD_DIR)/include/linux/wireless.h
	+$(MAKE) $(PKG_JOBS) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)" \
		NOSTDINC_FLAGS="$(NOSTDINC_FLAGS)" \
		CONFIG_RTL8189ES=m \
		USER_MODULE_NAME=rtl8189es \
		modules
endef

$(eval $(call KernelPackage,rtl8189es-ol))
