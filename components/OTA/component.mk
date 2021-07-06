COMPONENT_NAME := . OTA
COMPONENT_ADD_INCLUDEDIRS += include ../WIFI/include ../Config/include ../ArduinoJson/include ../Mongoose/include ../Mongoose
CFLAGS += -DLWIP_COMPAT_SOCKETS
