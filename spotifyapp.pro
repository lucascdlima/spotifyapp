QT       += core gui network networkauth uitools

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
CONFIG += qt debug
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    interface/mainwindow.cpp\
    api/spotifyapi.cpp \
    models/treeitem.cpp \
    models/treemodel.cpp

HEADERS += \
    interface/mainwindow.h \
    models/musicutils.h \
    api/spotifyapi.h \
    models/spotifyutils.h \
    models/treeitem.h \
    models/treemodel.h

FORMS += \
    interface/mainwindow.ui

DESTDIR = /home/lucaslima/DesafioStone/spotifyapp/bin

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
