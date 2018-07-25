TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp \
    CameraManager.cpp

HEADERS += \
    CameraManager.h

LIBS += -lpthread
