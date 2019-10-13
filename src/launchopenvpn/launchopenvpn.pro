#-------------------------------------------------
#
# Project created by QtCreator 2015-06-21T18:49:37
#
#-------------------------------------------------

QT       += core
QT       -= gui

TARGET = launchopenvpn
#CONFIG   += console
CONFIG   -= app_bundle
CONFIG += static

TEMPLATE = app

QMAKE_INCDIR += ../monvpn/

SOURCES += main.cpp \
	../monvpn/pathhelper.cpp \
    common.cpp

HEADERS  += \
	../monvpn/pathhelper.h \
    common.h
