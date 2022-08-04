QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = BitmapViewer

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += $$files(*.cpp)

HEADERS += $$files(*.h)

INCLUDEPATH += ../../libbitmap ../../../include

LIBS += -L$$PWD/../../../lib -lbitmap -lSQLiteCpp -lsqlite3

target.path = $$PWD/../../../bin
INSTALLS += target
