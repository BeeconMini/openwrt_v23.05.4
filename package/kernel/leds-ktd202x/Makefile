include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=leds-ktd202x
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0-or-later

include $(INCLUDE_DIR)/package.mk

define KernelPackage/leds-ktd202x
  SUBMENU:=LED modules
  TITLE:=KDT-2026/7 LED controller support
  FILES:= $(PKG_BUILD_DIR)/leds-ktd202x.ko
  AUTOLOAD:=$(call AutoProbe,leds-ktd202x,1)
  DEPENDS:=+kmod-i2c-core +kmod-regmap-i2c
endef

define KernelPackage/leds-ktd202x/description
  support for Kinetic KTD2026 (RGB) and KTD2027 (RGBW)
  single multi-LED controller with I2C interface
endef

define Build/Compile
	$(KERNEL_MAKE) M="$(PKG_BUILD_DIR)" modules
endef

$(eval $(call KernelPackage,leds-ktd202x))
