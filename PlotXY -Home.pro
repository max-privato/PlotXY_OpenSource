#-------------------------------------------------
#
# Project created by QtCreator 2012-08-31T08:32:07
#
#-------------------------------------------------

QT       += core gui svg printsupport

QTPLUGIN += qico
# QT       += core gui svg
# DEFINES= += QT_DISABLE_DEPRECATED_BEFORE=0x050f00

# --- Solo per il kit statico MSYS2 (Windows/MinGW con Qt statica) ---
win32-g++:CONFIG(static, static|shared) {
    QMAKE_LFLAGS += -static
    # dipendenze delle librerie esterne, messe in coda al link nell'ordine giusto
    QMAKE_LIBS_PRIVATE += -lharfbuzz -lfreetype -lharfbuzz -lgraphite2 -lpng16 \
                          -lbz2 -lbrotlidec -lbrotlicommon -lz \
                          -lglib-2.0 -lintl -liconv -lpcre2-8 \
                          -lrpcrt4 -lusp10 -lgdi32 -lws2_32 -lole32 -lwinmm -lshlwapi -luuid
}
win32{
#   QTPLUGIN += windowsprintersupport
} else:mac{
   QTPLUGIN += cocoaprintersupport
}

# QMAKE_CXXFLAGS += -Wfloat-conversion -Wconversion
QMAKE_CXXFLAGS+=-Wdeprecated-declarations
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

DEFINES+=_CRT_SECURE_NO_WARNINGS \

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
