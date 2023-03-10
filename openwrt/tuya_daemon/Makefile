include $(TOPDIR)/rules.mk

PKG_NAME:=tuya_daemon
PKG_VERSION:=0.1
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/tuya_daemon
	DEPENDS:=+libubus +libubox +libblobmsg-json +libtuya_iot_core +libuci
	CATEGORY:=Network
	TITLE:=Daemon that sends device memory info to Tuya cloud
endef

define Package/tuya_daemon/install
	$(INSTALL_DIR) $(1)/usr/bin $(1)/etc/config $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tuya_daemon $(1)/usr/bin/tuya_daemon
	$(INSTALL_BIN) ./files/tuya.init $(1)/etc/init.d/tuya
	$(INSTALL_CONF) ./files/tuya.config $(1)/etc/config/tuya
endef

$(eval $(call BuildPackage,tuya_daemon))
