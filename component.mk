# Edit following two lines to set component requirements (see docs)
SRCDIRS := freertos-addons-master/c/Source 
SRCDIRS += freertos-addons-master/c++/Source 

INCLDIRS := freertos-addons-master/c/Source/include 
INCLDIRS += freertos-addons-master/c++/Source/include
INCLDIRS += freertos-addons-master

COMPONENT_ADD_INCLUDEDIRS := $(INCLDIRS)

COMPONENT_SRCDIRS := $(SRCDIRS)
