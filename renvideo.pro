QT += core gui
QT += widgets

INCLUDEPATH +=$$PWD

TARGET = renvideo
TEMPLATE = app

SOURCES += main.cpp \
           remvideo.cpp

HEADERS += remvideo.h

FORMS += remvideo.ui

RESOURCES += \
    rec.qrc
