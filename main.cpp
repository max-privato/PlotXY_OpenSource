/*
 * This file is part of MC's PlotXY.
 *
 * PlotXY was created during 1998, continuously maintained and upgraded up to current year
 * by Massimo Ceraolo from the University of Pisa.
 *
 * The Linux distribution has been built using Ceraolo's source code in 2018 by Perry
 * Clements from Canada.
 *
 * This program is free software: you can redistribute it under the terms of GNU Public
 * License version 3 as published by the Free Software Foundation.
 *
 * PLOTXY AND ALL THE RELATED MATERIAL INCLUDED IN THE DISTRIBUTION PLOTXY.ZIP FILE OR
 * AVAILABLE FROM GITHUB IS SUPPLIED "AS-IS" THE AUTHOR OFFERS NO WARRANTY OF ITS FITNESS
 * FOR ANY PURPOSE WHATSOEVER, AND ACCEPTS NO LIABILITY WHATSOEVER FOR ANY LOSS OR
 * DAMAGE INCURRED BY ITS USE.
 *
 */

#include <QApplication>  //#include <QtGui/QApplication>
#include <QMessageBox>
#include <QSettings>
#include "CDataSelWin.h"
#include "Globals.h"


/* The following structure, in BCB's PlotXY, was designed to easily transfer a
 * few global variables between the various modules. In the Qt translation it
 * was possible to make a better encapsulation of the variables, and therefore
 * this structure is shared at the moment (18/1/2014) only between main and CDataSelWin !!
*/
SGlobalVars GV;
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    }
}

int main(int argc, char *argv[])
{
    int i;
    //La seguente riga consente di sostituire errori runTtime con chiamata alla funzione quisopra, che puòpoi essere debuggata.
    //Va abilitata quando ci sono errori runtime, e poi disabilitata quando sono stati risolti.

    qInstallMessageHandler(myMessageOutput);
    QApplication a(argc, argv);

    // The information in the following two lines is used later to write on the registry the data recorded using the "QSettings" object.
    // The name of the organization is different from that of the old PlotXY, in order to maintain independence of the two on the register. The deletion of the keys under "University of Pisa" will not delete the keys  under "University of Pisa - MC's" and vice versa.
    // In Linux, the data is stored in $HOME/.config/University of Pisa - MC's/MC's PlotXY.conf
    QCoreApplication::setOrganizationName("University of Pisa - MC's");
    QCoreApplication::setApplicationName("MC's PlotXY");

    // Now if the user has generated settings in previous instances (in windows registry entries) I load them.
    // Later these values ​​can be modified on the basis of passed parameters
    // The following all uppercase variables are defined in special terms in Globals.h
    QSettings settings;
//    struct SOptions* PO=&GV.PO;
    settings.beginGroup("globalOptions");
    GV.PO.autoLabelXY=settings.value("autoLabelXY",AUTOLABELXY).toBool();
    GV.PO.barChartForFS=settings.value("barChartForFS",BARCHARTFORHFS).toBool();
    GV.PO.commasAreSeparators=settings.value("commasAreSeparators",COMMASARESEPARATORS).toBool();
    GV.PO.compactMMvarMenu=settings.value("compactMMvarMenu",COMPACTMMVARMENU).toBool();
    GV.PO.largerFonts=settings.value("largerFonts",LARGERFONTS).toBool();
    GV.PO.useBrackets=settings.value("useBrackets",USEBRACKETS).toBool();
    GV.PO.useGrids=settings.value("useGrids",USEGRIDS).toBool();
    GV.PO.useOldColors=settings.value("useOldColors",USEOLDCOLORS).toBool();
    GV.PO.plotPenWidth=settings.value("plotPenWidth",PLOTPENWIDTH).toInt();
    GV.PO.onlyPoints=settings.value("onlyPoints",ONLYPOINTSINPLOTS).toBool();
    GV.PO.rememberWinPosSize=settings.value("rememberWinPosSize",REMEMBERWINPOSANDSIZE).toBool();
    GV.PO.trimQuotes=settings.value("trimQuotes",TRIMQUOTES).toBool();
    GV.PO.useCopiedDialog=settings.value("useCopiedDialog",USECOPIEDDIALOG).toBool();
    GV.PO.defaultFreq=settings.value("defaultFreq",DEFAULTFREQ).toDouble();
    settings.endGroup();
    GV.multiFileMode =settings.value("multifileMode",true).toBool();
    for(int i=0;i<MAXFILES; i++)
      GV.varNumsLst.append(0);

    QStringList optLst={"/dtXY","/dtQtF","/dtQtI","/dtQtP","/sff","/uml","/set"};
    GV.PO.drawType=0;  // filtraggio grafici XY
    QString opt;
    for(i=1; i<QCoreApplication::arguments().count(); i++){
      opt=QCoreApplication::arguments().at(i);
      if(opt[0]!='/')break;
      if(optLst.indexOf(opt)<0){
        QMessageBox msgBox;
        msgBox.setText("the following command-line option is unrecognised: \n"+opt);
        msgBox.exec();
     }
     if(opt=="/dtQtF")   //filtraggio Qt con precisione float
       GV.PO.drawType=1;
     if(opt=="/dtQtI") //filtraggio Qt con precisione integer
       GV.PO.drawType=2;
     if(opt=="/dtQtP") //filtraggio Qt con polinomio
       GV.PO.drawType=3;
     if(opt=="/sff") //Show Full File List
       GV.PO.showFullFilelist=true;
     if(opt=="/uml"){
       GV.PO.useMatLib=true;
      }else{
        GV.PO.useMatLib=false;
      }
      if(opt=="/set")  //ShowElapsedTime
        GV.PO.showElapsTime=true;
      else
        GV.PO.showElapsTime=false;
    }
    // The file names come after the options, so I have to memorize the last option found:
    GV.PO.firstFileIndex=i;

    // At the moment (7/9/2014) the GV.PO are accessed directly by DataSelWin, and passed with the signal/slot mechanism to CPlotWin and CFourWin.
    // The slots of these two classes then distribute the options to the underlying classes that need them (without additional signal / slot).
    // The first execution of the signal is commanded at the bottom of CDataSelWin :: CdataSeWin, after the relative connect has been made.

    qApp->setStyleSheet("QTabWidget { font-size: 12px }");
    qApp->setStyleSheet("QTableWidget { font-size: 12px }");
    qApp->setOverrideCursor(QCursor(Qt::WaitCursor));

    // The following line is essential for compiling under Ubuntu. Otherwise
    // the numbers are coded using the comma as decimal separator, which would cause big troubles to all the I/O software from text files .
    // Basically "The std :: scanf family of functions are locale aware". Evidently the default locale in Ubuntu is not the standard C.
     setlocale(LC_NUMERIC,"C");

    CDataSelWin w;
    w.show();

    return a.exec();
}
