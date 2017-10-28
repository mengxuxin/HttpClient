QT += core network
QT -= gui

CONFIG += c++11

TARGET = HttpClient
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    httpclient.cpp

HEADERS += \
    httpclient.h
