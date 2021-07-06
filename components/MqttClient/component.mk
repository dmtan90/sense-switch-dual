#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_NAME:= . MqttClient
COMPONENT_ADD_INCLUDEDIRS := include
COMPONENT_EMBED_TXTFILES := keys/ec_private.pem keys/ec_public.pem
