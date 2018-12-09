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
#include <QSettings>
#include "CDataSelWin.h"
#include "Globals.h"


/*La seguente struttura, in PlotXY di BCB era pensata per trasferire agevolmente poche variabili globali fra i vari moduli.
Nella traduzione Qt è risultato possibile fare un migliore incapsulamento delle variabili, e pertanto questa struttura è condivisa al momento (18/1/2014)solo fra main e CDataSelWin!!
*/
SGlobalVars GV;

int main(int argc, char *argv[])
{
    int i;
    QApplication a(argc, argv);
    //Le informazioni delle seguenti due righe sono utilizzate poi per la scrittura sul registry dei dati registrati mediante l'oggetto "QSettings".
    //Il nome dell'organizzazione è diverso da quello del vecchio PlotXY, in modo da mantenere l'indipendenza sul registro. La cancellazione delle chiavi sotto "University of Pisa" non cancellerà le chiavi del sotto "University of Pisa - MC's" e viceversa.
    QCoreApplication::setOrganizationName("University of Pisa - MC's");
    QCoreApplication::setApplicationName("MC's PlotXY");
    //Ora se l'utente ha generato dei settings in precedenti istanze (in windows voci del registro di sistema) li carico.
    // Successivamente tali valori potranno essere modificati sulla base di parametri passati
    // Le seguenti variabili tutte maiuscole sono definite in appositi defines in Globals.h
    QSettings settings;
//    struct SOptions* PO=&GV.PO;
    settings.beginGroup("globalOptions");
    GV.PO.autoLabelXY=settings.value("autoLabelXY",AUTOLABELXY).toBool();
    GV.PO.barChartForFS=settings.value("barChartForFS",BARCHARTFORHFS).toBool();
    GV.PO.largerFonts=settings.value("largerFonts",LARGERFONTS).toBool();
    GV.PO.useBrackets=settings.value("useBrackets",USEBRACKETS).toBool();
    GV.PO.useGrids=settings.value("useGrids",USEGRIDS).toBool();
    GV.PO.useOldColors=settings.value("useOldColors",USEOLDCOLORS).toBool();
    GV.PO.plotPenWidth=settings.value("plotPenWidth",PLOTPENWIDTH).toInt();
    GV.PO.onlyPoints=settings.value("onlyPoints",ONLYPOINTSINPLOTS).toBool();
    GV.PO.commasAreSeparators=settings.value("commasAreSeparators",COMMASARESEPARATORS).toBool();
    GV.PO.rememberWinPosSize=settings.value("rememberWinPosSize",REMEMBERWINPOSANDSIZE).toBool();
    GV.PO.trimQuotes=settings.value("trimQuotes",TRIMQUOTES).toBool();
    GV.PO.defaultFreq=settings.value("defaultFreq",DEFAULTFREQ).toDouble();
    settings.endGroup();
    GV.multiFileMode =settings.value("multifileMode",true).toBool();
    for(int i=0;i<MAXFILES; i++)GV.varNumsLst.append(0);

    QString opt;
    for(i=1; i<QCoreApplication::arguments().count(); i++){
        opt=QCoreApplication::arguments().at(i).toLower();
        if(opt[0]!='/')break;
        if(opt=="/sff")
            GV.PO.showFullFilelist=true;
        if(opt=="/uml")
            GV.PO.useMatLib=true;
        else
            GV.PO.useMatLib=false;
        if(opt=="/set")
            GV.PO.showElapsTime=true;
        else
            GV.PO.showElapsTime=false;
    }
    // I nomi dei files vengono dopo le opzioni, quindi devo memorizzare l'ultima opzione trovata:
    GV.PO.firstFileIndex=i;

    // Al momento (7/9/2014) le GV.PO sono accedute direttamente da DataSelWin, e passate con il meccanismo signal/slot a CPlotWin e CFourWin. Gli slot di queste due classi distribuiscono poi le opzioni alle classi sottostanti che ne necessitano (senza ulteriori signal/slot).
    //La prima esecuzione del signal è comandata in fondo a CDataSelWin::CdataSeWin, dopo che sono stati fatti i connect relativi.

    qApp->setStyleSheet("QTabWidget { font-size: 12px }");
    qApp->setStyleSheet("QTableWidget { font-size: 12px }");
    qApp->setOverrideCursor(QCursor(Qt::WaitCursor));

    CDataSelWin w;
    w.show();

    return a.exec();
}
