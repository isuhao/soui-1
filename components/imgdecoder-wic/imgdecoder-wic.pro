######################################################################
# Automatically generated by qmake (2.01a) ?? ?? 23 19:27:34 2014
######################################################################

TEMPLATE = lib
TARGET = imgdecoder-wic
DEPENDPATH += .
INCLUDEPATH += . \
			   ../../utilities/include \
			   ../../soui/include \

dir = ../..
include($$dir/common.pri)

CONFIG(debug,debug|release){
	DESTDIR = $$dir/bin/debug
}
else{
	DESTDIR = $$dir/bin/release
}

PRECOMPILED_HEADER = stdafx.h

# Input
HEADERS += imgdecoder-wic.h targetver.h
SOURCES += dllmain.cpp imgdecoder-wic.cpp
