#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)
COMPONENT_NAME:= . ArduinoJson
COMPONENT_ADD_INCLUDEDIRS := include
CFLAGS += -Wno-error=implicit-function-declaration -Wno-error=format= -Uwrite
