COMPONENT_NAME += . WIFI
COMPONENT_ADD_INCLUDEDIRS += include ../MQTT/include ../Mongoose/include  ../Config/include ../OTA/include
#CFLAGS += -Wno-error=implicit-function-declaration -Wno-error=format= -DHAVE_CONFIG_H -lstdc++ -Ulist_node
COMPONENT_EMBED_FILES += www/index.html 
COMPONENT_EMBED_FILES += www/fonts/material-icons.svg 

COMPONENT_EMBED_FILES += www/img/favicon.ico
COMPONENT_EMBED_FILES += www/img/logo.png
COMPONENT_EMBED_FILES += www/fonts/material-icons.woff2
COMPONENT_EMBED_FILES += www/css/style.min.css
COMPONENT_EMBED_FILES += www/js/materialize.1.min.js
COMPONENT_EMBED_FILES += www/js/materialize.2.min.js
COMPONENT_EMBED_FILES += www/js/jquery.min.js
COMPONENT_EMBED_FILES += www/js/init.min.js
COMPONENT_EMBED_FILES += www/js/core.js
COMPONENT_EMBED_FILES += www/js/lang.min.js

