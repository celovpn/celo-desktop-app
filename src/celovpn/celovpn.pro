#-------------------------------------------------
#
# Project created by QtCreator 2015-02-25T18:16:55
#
#-------------------------------------------------

QT	   += network xml core gui widgets

TARGET = "Celo VPN"
TEMPLATE = app
INCLUDEPATH += ../common

macx: {
    QMAKE_INFO_PLIST = ./Info.plist
    QMAKE_LFLAGS += -F /System/Library/Frameworks/
    QMAKE_RPATHDIR += @executable_path/../Frameworks
    LIBS += -framework Security -framework ServiceManagement -framework Foundation
    target.path = /Applications
    resources.path = "/Applications/Celo VPN.app/Contents/Resources"
    resources.files = ./resources/*
    INSTALLS = target resources
    ICON = celo.icns

    include (../common/certificate.pri)

    INFO_PLIST_PATH = $$shell_quote($$(PWD)/Celo VPN.app/Contents/Info.plist)

    APP_IDENTIFIER = net.celo.CeloVPN
    HELPER_IDENTIFIER = net.celo.CeloVPNHelper

    plist.commands += $(COPY) $$PWD/Info.plist $${INFO_PLIST_PATH};
    # plist.commands += /usr/libexec/PlistBuddy -c \"Set :CFBundleIdentifier $${APP_IDENTIFIER}\" $${INFO_PLIST_PATH};
    # plist.commands += /usr/libexec/PlistBuddy -c \'Set :SMPrivilegedExecutables:$${HELPER_IDENTIFIER} 'anchor apple generic and identifier \\\"$${HELPER_IDENTIFIER}\\\" and (certificate leaf[field.1.2.840.113635.100.6.1.9] /* exists */ or certificate 1[field.1.2.840.113635.100.6.2.6] /* exists */ and certificate leaf[field.1.2.840.113635.100.6.1.13] /* exists */ and certificate leaf[subject.OU] = $${CERT_OU})'\' $${INFO_PLIST_PATH};
    first.depends = $(first) plist
    export(first.depends)
    export(plist.commands)
    QMAKE_EXTRA_TARGETS += first plist

    SOURCES += smjobbless.mm
    HEADERS += smjobbless.h
}

win32: {
    TARGET=celovpn
    WINSDK_DIR = C:/Program Files/Microsoft SDKs/Windows/v7.1
    WIN_PWD = $$replace(PWD, /, \\)
    OUT_PWD_WIN = $$replace(OUT_PWD, /, \\)
    QMAKE_POST_LINK = "$$quote($$OUT_PWD_WIN\\..\\fixbinary.bat) \"$$OUT_PWD_WIN\\..\\$$TARGET\".exe $$quote($$WIN_PWD\\runasadmin.manifest)"
    RC_FILE = celo.rc
    LIBS += -lws2_32 -lIphlpapi
    DESTDIR = ../../buildwindows/
    MOC_DIR = ../.obj/
    HEADERS += \
        qtlocalpeer.h \
        qtlockedfile.h \
        qtsingleapplication.h \

    SOURCES += \
        qtlocalpeer.cpp \
        qtlockedfile.cpp \
        qtsingleapplication.cpp \
}

linux: {
    TARGET=celovpn
    HEADERS += \
        qtlocalpeer.h \
        qtlockedfile.h \
        qtsingleapplication.h \

    SOURCES += \
        qtlocalpeer.cpp \
        qtlockedfile.cpp \
        qtsingleapplication.cpp \

    CONFIG += static
}

SOURCES += \
    authmanager.cpp \
    ../common/common.cpp \
    confirmationdialog.cpp \
    connectiondialog.cpp \
    dlg_newnode.cpp \
    encryptiondelegate.cpp \
    errordialog.cpp \
    flag.cpp \
    fonthelper.cpp \
    locationdelegate.cpp \
    ../common/log.cpp \
    loginwindow.cpp \
    main.cpp \
    mapscreen.cpp \
    ministun.c \
    ../common/osspecific.cpp \
    ../common/pathhelper.cpp \
    pingwaiter.cpp \
    portforwarder.cpp \
    protocoldelegate.cpp \
    scr_logs.cpp \
    setting.cpp \
    stun.cpp \
    trayiconmanager.cpp \
    vpnservicemanager.cpp \
    wndmanager.cpp \
    settingsscreen.cpp

HEADERS += \
    application.h \
    authmanager.h \
    ../common/common.h \
    confirmationdialog.h \
    connectiondialog.h \
    dlg_newnode.h \
    encryptiondelegate.h \
    errordialog.h \
    flag.h \
    fonthelper.h \
    locationdelegate.h \
    ../common/log.h \
    loginwindow.h \
    mapscreen.h \
    ministun.h \
    ../common/osspecific.h \
    ../common/pathhelper.h \
    pingwaiter.h \
    portforwarder.h \
    protocoldelegate.h \
    scr_logs.h \
    setting.h \
    stun.h \
    trayiconmanager.h \
    update.h \
    version.h \
    vpnservicemanager.h \
    wndmanager.h \
    settingsscreen.h

FORMS += \
    confirmationdialog.ui \
    connectiondialog.ui \
    dlg_newnode.ui \
    errordialog.ui \
    loginwindow.ui \
    mapscreen.ui \
    scr_logs.ui \
    settingsscreen.ui

RESOURCES += \
    imgs.qrc

