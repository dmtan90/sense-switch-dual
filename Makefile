#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := sense_switch_dual_0_0_1_4

APP ?= mongoose-os
APP_PLATFORM = esp32

COMPONENT_ADD_INCLUDEDIRS := components

include $(IDF_PATH)/make/project.mk

COMPONENT_LDFLAGS += -lstdc++ --verbose

CFLAGS += -DCS_PLATFORM=3 -DMG_DISABLE_DIRECTORY_LISTING=1 -DMG_DISABLE_DAV=1 -DMG_DISABLE_CGI=1 
MODULE_CFLAGS=-DMG_ENABLE_THREADS -DMG_ENABLE_HTTP_WEBSOCKET=0
# -DMG_DISABLE_PFS=1 -DMG_SSL_MBED_DUMMY_RANDOM=1 -DMG_ENABLE_DEBUG=1 -DMG_SSL_IF_MBEDTLS_MAX_FRAG_LEN=2048 -DMG_SSL_IF_MBEDTLS_FREE_CERTS=1 -DMG_ENABLE_SSL=1  -DMG_ENABLE_DEBUG=1 

#SSL_LIB ?= openssl

# Copy some defaults into the sdkconfig by default
# so BT stack is enabled
sdkconfig: sdkconfig.defaults
	$(Q) cp $< $@

menuconfig: sdkconfig
defconfig: sdkconfig

