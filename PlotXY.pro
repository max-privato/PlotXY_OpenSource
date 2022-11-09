#-------------------------------------------------
#
# Project created by QtCreator 2012-08-31T08:32:07
#
#-------------------------------------------------

QT       += core gui svg printsupport

greaterThan(QT_MAJOR_VERSION,5) {
  QT += core5compat
}

# QTPLUGIN += qico

# QT       += core gui svg
# DEFINES= += QT_DISABLE_DEPRECATED_BEFORE=0x050f00

win32{
#   QTPLUGIN += windowsprintersupport
} else:mac{
   QTPLUGIN += cocoaprintersupport
}

# QMAKE__CXXFLAGS += -Wfloat-conversion -Wconversion
QMAKE_CXXFLAGS+=-pg
QMAKE_LFLAGS+=-pg
TEMPLATE = app
CONFIG += c++11

SOURCES += main.cpp\
    CDataSelWin.cpp \
    CFourWin.cpp \
    CLineChart.cpp \
    CPlotWin.cpp\
    CSimOut.cpp  \
    CValuesWin.cpp \
    CVarMenu.cpp \
    CVarTableComp.cpp \
    CLineCalc.cpp \
    Dialogs/CFourOutputInfo.cpp \
    Dialogs/CFourOptions.cpp \
    Dialogs/CProgOptions.cpp \
    Dialogs/CPlotOptions.cpp \
    Dialogs/CScaleDlg.cpp \
    Dialogs/CPrintWOptions.cpp \
    Dialogs/CHelpDlg.cpp \
    Dialogs/CFunStrInput.cpp \
    Dialogs/CUnitsDlg.cpp \
    SuppFunctions.cpp \
    CParamView.cpp \
    Dialogs/CCustomiseCol.cpp

HEADERS  += CDataSelWin.h\
    CFourWin.h \
    CLineChart.h \
    CPlotWin.h \
    CSimOut.h\
    CValuesWin.h \
    CVarmenu.h \
    Globals.h \
    CVarTableComp.h \
    CLineCalc.h \
    Dialogs/CFourOutputInfo.h \
    Dialogs/CFourOptions.h \
    Dialogs/CProgOptions.h \
    Dialogs/CPlotOptions.h \
    Dialogs/CScaleDlg.h \
    Dialogs/CPrintWOptions.h \
    Dialogs/CHelpDlg.h \
    Dialogs/CFunStrInput.h \
    Dialogs/CUnitsDlg.h \
    CParamView.h \
    SuppFunctions.h \
    ExcludeATPCode.h \
    Dialogs/CCustomiseCol.h

win32:DEFINES+=_CRT_SECURE_NO_WARNINGS \

FORMS    += CDataSelWin.ui \
    CFourWin.ui \
    CPlotWin.ui \
    CValuesWin.ui \
    Dialogs/CFourOutputInfo.ui \
    Dialogs/CFourOptions.ui \
    Dialogs/CProgOptions.ui \
    Dialogs/CPlotOptions.ui \
    Dialogs/CScaleDlg.ui \
    Dialogs/CPrintWOptions.ui \
    Dialogs/CHelpDlg.ui \
    Dialogs/CFunStrInput.ui \
    Dialogs/CUnitsDlg.ui \
    CParamView.ui \
    Dialogs/CCustomiseCol.ui

RESOURCES =  Images.qrc
ICON = xyNew.icns
RC_ICONS = xyNew.ico
