#-------------------------------------------------
#
# Project created by QtCreator 2012-08-31T08:32:07
#
#-------------------------------------------------

QT       += core gui svg printsupport
# QTPLUGIN += qico

# QT       += core gui svg

win32{
#   QTPLUGIN += windowsprintersupport
} else:mac{
   QTPLUGIN += cocoaprintersupport
}

TARGET = PlotXY
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
    dialogs/CFourOutputInfo.cpp \
    dialogs/CFourOptions.cpp \
    dialogs/CProgOptions.cpp \
    dialogs/CPlotOptions.cpp \
    dialogs/CScaleDlg.cpp \
    dialogs/CPrintWOptions.cpp \
    dialogs/CHelpDlg.cpp \
    dialogs/CFunStrInput.cpp \
    dialogs/CUnitsDlg.cpp \
    suppFunctions.cpp

HEADERS  += CDataSelWin.h\
    CFourWin.h \
    CLineChart.h \
    CPlotWin.h \
    CSimOut.h\
    CValuesWin.h \
    CVarmenu.h \
    Globals.h \
    matrix.h \
    CVarTableComp.h \
    CLineCalc.h \
    dialogs/CFourOutputInfo.h \
    dialogs/CFourOptions.h \
    dialogs/CProgOptions.h \
    dialogs/CPlotOptions.h \
    dialogs/CScaleDlg.h \
    dialogs/CPrintWOptions.h \
    dialogs/CHelpDlg.h \
    dialogs/CFunStrInput.h \
    dialogs/CUnitsDlg.h

win32:DEFINES+=_CRT_SECURE_NO_WARNINGS \

FORMS    += CDataSelWin.ui \
    CFourWin.ui \
    CPlotWin.ui \
    CValuesWin.ui \
    dialogs/CFourOutputInfo.ui \
    dialogs/CFourOptions.ui \
    dialogs/CProgOptions.ui \
    dialogs/CPlotOptions.ui \
    dialogs/CScaleDlg.ui \
    dialogs/CPrintWOptions.ui \
    dialogs/CHelpDlg.ui \
    dialogs/CFunStrInput.ui \
    dialogs/CUnitsDlg.ui

RESOURCES =  Images.qrc
ICON = xyNew.icns
RC_ICONS = xyNew.ico

