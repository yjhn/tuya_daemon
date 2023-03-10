include $(TOPDIR)/rules.mk

PKG_NAME:=libtuya_iot_core
PKG_RELEASE:=1
PKG_VERSION:=0.1
PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/tuya/tuya-iot-core-sdk
PKG_SOURCE_VERSION:=0208badefb2003d30e70b5b875395e3187c850d9

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
INTERNAL_BUILD_DIR:=$(PKG_BUILD_DIR)/build
PROVIDED_HEADERS:=$(wildcard $(PKG_BUILD_DIR)/include/*.h) \
	$(wildcard $(PKG_BUILD_DIR)/interface/*.h) \
	$(wildcard $(PKG_BUILD_DIR)/utils/*.h) \
	$(PKG_BUILD_DIR)/examples/data_model_basic_demo/tuya_cacert.h
PROVIDED_LIBS:=$(INTERNAL_BUILD_DIR)/lib/liblink_core.so \
	$(INTERNAL_BUILD_DIR)/lib/libmiddleware_implementation.so \
	$(INTERNAL_BUILD_DIR)/lib/libplatform_port.so \
	$(INTERNAL_BUILD_DIR)/lib/libutils_modules.so

include $(INCLUDE_DIR)/package.mk

define Package/libtuya_iot_core
	CATEGORY:=Network
	TITLE:=Tuya IoT Core SDK
	URL:=$(PKG_SOURCE_URL)
endef

define Package/libtuya_iot_core/description
	SDK for connecting to Tuya IoT Cloud.
	Source code of the SDK can be found on GitHub:
	$(PKG_SOURCE_URL)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib $(1)/usr/include
	$(INSTALL_BIN) $(PROVIDED_LIBS) $(1)/usr/lib/
	$(INSTALL_DATA) $(PROVIDED_HEADERS) $(1)/usr/include/
endef

define Build/Compile
	CC="$(TARGET_CC)" CFLAGS="$(TARGET_CFLAGS)" LDFLAGS="$(TARGET_LDFLAGS)" \
		cmake -S $(PKG_BUILD_DIR) -B $(INTERNAL_BUILD_DIR)
	$(call Build/Compile/Default,-C $(INTERNAL_BUILD_DIR))
endef

define Package/libtuya_iot_core/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PROVIDED_LIBS) $(1)/usr/lib
endef

$(eval $(call BuildPackage,libtuya_iot_core))
