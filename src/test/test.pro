TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
TARGET = Test

INCLUDEPATH += ../libbitmap ../../include

SOURCES += main.cpp
OTHER_FILES += $$files(*.sql)

LIBS += -L$$PWD/../../lib -lbitmap -lSQLiteCpp -lsqlite3

target.path = $$PWD/../../bin
INSTALLS += target
