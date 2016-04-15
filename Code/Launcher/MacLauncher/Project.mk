#############################################################################
## Crytek Source File
## Copyright (C), Crytek, 1999-2012.
##
## Creator: Valerio Guagliumi
## Date: Feb 01, 2013
## Description: GNU-make based build system
#############################################################################

PROJECT_TYPE := program

PROJECT_CPPFLAGS_COMMON := \
	-I$(CODE_ROOT)/CryEngine/CryCommon \
	-I$(CODE_ROOT)/CryEngine/CrySystem \
	-I$(CODE_ROOT)/CryEngine/CryAction \
	-I$(CODE_ROOT)/SDKs/boost

PROJECT_SOURCES_CPP := \
	StdAfx.cpp \
	Main.cpp \

PROJECT_SOURCES_H := StdAfx.h 
PROJECT_SOURCE_PCH := StdAfx.h

# Macports library directory
PROJECT_LDFLAGS := -L/opt/local/lib

# SDK library
PROJECT_LDLIBS := -lSDL

# Frameworks required by freetype for MAC
PROJECT_LDFLAGS += -framework CoreServices
PROJECT_LDFLAGS += -framework ApplicationServices

# Frameworks required by FMOD for MAC
PROJECT_LDFLAGS += -framework AudioUnit
PROJECT_LDFLAGS += -framework CoreAudio
PROJECT_LDFLAGS += -framework Carbon

# FMOD libraries
PROJECT_LDLIBS += $(CODE_ROOT)/SDKs/Audio/FmodEx/lib/Mac/libfmodex.a
PROJECT_LDLIBS += $(CODE_ROOT)/SDKs/Audio/FmodEx/lib/Mac/libfmodevent.a
## No need to include FMOD event net system binaries for Release builds!
ifneq ($(OPTION_BUILD_MODE),Release)
    LDLIBS += $(CODE_ROOT)/SDKs/Audio/FmodEx/lib/Mac/libfmodeventnet.a
endif	

# vim:ts=8:sw=8

