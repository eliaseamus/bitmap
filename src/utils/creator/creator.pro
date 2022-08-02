TEMPLATE = app
TARGET = BitmapCreator
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

INCLUDEPATH += ../../libbitmap ../../../include

LIBS += -L$$PWD/../../../lib -lbitmap -lSQLiteCpp -lsqlite3

target.path = $$PWD/../../../bin
INSTALLS += target
