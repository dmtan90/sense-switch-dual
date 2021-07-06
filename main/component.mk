#
# Main Makefile. This is basically the same as a component makefile.
#
# This Makefile should, at the very least, just include $(SDK_PATH)/make/component_common.mk. By default,
# this will take the sources in the src/ directory, compile them and link them into
# lib(subdirectory_name).a in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#

COMPONENT_ADD_INCLUDEDIRS := . BLE WIFI Mongoose OTA StorageUtility MqttClient

COMPONENT_SRCDIRS := $(COMPONENT_ADD_INCLUDEDIRS)

SSL_LIB ?= openssl

CFLAGS += -DCS_PLATFORM=3 -DMG_DISABLE_DIRECTORY_LISTING=1 -DMG_DISABLE_DAV=1 -DMG_DISABLE_CGI=1 -DMG_ENABLE_SSL=1



#main.o: prepare

#.PHONY: prepare
#prepare:
	#tail -n +2 $(SSL_CERT_PEM).pem | \
		head -n -1 | base64 -d - \
			> $(COMPONENT_PATH)/../romfs-files/cert-key/ssl-cert.der
	#tail -n +2 $(SSL_KEY_PEM).pem | \
		head -n -1 | base64 -d - \
			> $(COMPONENT_PATH)/../romfs-files/cert-key/ssl-key.der
	#genromfs -f romfs.img -d $(COMPONENT_PATH)/../romfs-files
