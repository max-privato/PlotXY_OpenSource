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

#ifndef CDATASELWIN_H
#define CDATASELWIN_H
#define FILETABLECOLS 7
#define FILENUMCOL 1
#define FILENAMECOL 2
#define TSHIFTCOL 6
#define FRACTIONOOFSCREEN 0.95


#include <qcolor.h>
#include <QDataStream>
#include <QDropEvent>
#include <QMainWindow>
#include <QString>
#include <QTableWidget>
#include "Globals.h"
#include "suppFunctions.h"
#include "CFourWin.h"
#include "dialogs/CUnitsDlg.h"
#include "dialogs/CHelpDlg.h"
#include "dialogs/CProgOptions.h"
#include "CSimOut.h"
#include "CVarTableComp.h"
#include "CPlotWin.h"

namespace Ui {
class CDataSelWin;
}

class CDataSelWin : public QMainWindow {
    Q_OBJECT

public:
    explicit CDataSelWin(QWidget *parent = 0);
    void closeEvent(QCloseEvent *) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void enterEvent(QEvent *);
    QDataStream giveState();
    void resizeEvent(QResizeEvent *) override;
    void showEvent(QShowEvent *) override;
    ~CDataSelWin();
    void checkCalcData(SXYNameData calcData, QString & ret);  //funzione attualmente non usata!
    void updateSheet(int);
public slots: //Deve rimanere slot anche in Qt5 perché usato in QTimer
    void resetStateBtns(void);
    void giveFileInfo(QList <int> &fileNums, QList<QString> &fileNames, QList <int> &varNums);

private:
    void adaptToDPI(int currentDPI_, int maxHeight_);
    void fillVarMenuTable(int fileIndex);
    void groupSelected(int beginRow, int endRow);
    void varMenuTable_cellClicked(int row, int column, bool rightBtn);
    void mousePressEvent(QMouseEvent *) override;
    void moveEvent(QMoveEvent *);
    void varTableChanged ();

private slots:  //devono rimanere slot in quanto connessi sulla base del nome e non attraverso connect specifici
    void on_aboutBtn_clicked();
    void on_fileTable_clicked(const QModelIndex &index);
    void on_fileTable_doubleClicked(const QModelIndex &index);
    void on_fourTBtn_clicked();
    void on_loadStateTBtn_clicked();
    void on_loadTBtn_clicked();
    void on_multifTBtn_clicked(bool checked);
    void on_optBtn_clicked();
    void on_plotTBtn_clicked();
    void on_refrTBtn_clicked();
    void on_refrUpdTBtn_clicked(bool checked);
    void on_resetTBtn_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_updateTBtn_clicked(bool checked);
    void on_saveVarsBtn_clicked();
    void on_eqTBtn_clicked();
    void on_arrTBtn_clicked();
    void on_saveStateTBtn_clicked();
    void on_sortTBtn_clicked();

private:
  bool fileLoaded, doubleClicking, updatingPlot, refreshUpdate,
       goneToSingleFile;  //Se almeno una volta sono andato in singleFile nella presente sessione è true;
  enum ESortType{noSort, descending, ascending} sortType;
  int currentDPI; //current value of DPI: can change in a single run in case of multiple screens
  int fourTableIndex; //Contiene l'indice di varTable quando si  fatto fourier. va salvato nei settings al salvataggio stato per rifare al recupero stato l'analisi di fourier sulla tabella plot corretta.
  int numOfLoadedFiles; // numero di files presenti nella fileTable;
  int originalMaxWidth;
  int originalMinWidth;
  int originalWidth;
  int originalHeight;
  int selectedFileIdx; //indice del file corrente (da 0 a 7)
  int selectedFileRow; //indice di riga del file selezionato
  int visibleFileRows; //numero di righe di file visibili (si parte da 3)
  int topIndex[MAXFILES]; //contiene l'indice della prima variabile visualizzata del file finora visualizzato

  float * integrate(float*x, float *y, int nPoints); //calcola l'integrale di un array
  QString integrateUnits(QString unitS_);
  float * funXVar; //contiene i valori della variabie x quando è una funzione di variabile (cfr. developer.odt)
  QFont myFont;
  QString * saveStrings;  //Stringhe per il salvataggio valori della rica di indice 1 di fileTable quando si passa da multifile a singlefile
  QColor headerGray, neCellBkColor;
  QLabel *saveStateLbl;
  QSet <int> freeFileIndex; //contiene gli indici dei files NON caricati (liberi).

  /*  Le seguenti tre liste vengono in realtà gestite più come vettori che come liste: hanno sempre MAXFILES elementi, e condentono i dati dei file con num da 1 a 8 (come memorizzato in file NumLst.
   * In passato le gestivo come liste inserendo e toglilendo files NMaMa questo non funziona bene in quanto PlotXY è organizzato in modo che può esistere ad es. un unico file di Num pari a 2 (idx pari a 1) e in quel caso le liste avevano quell'ultnico filem, accedibvbile con indice 0 invece di 1. Allora occorrerebbe dtenere sempre traccia di come gli indici di files sono mappati nella lista il che è complicato. Invece se ho un elemento della lista per ogni potenziale file caricato, il problema si risolve. Dove non c'è un file il corrispondente Num è posto pari a 0.
*/
  QList <QString> fileNamesLst; //List dei nomi di files correntemente visualizzati nella fileTable
  QList <int> fileNumsLst;  //List dei numeri di files correntemente visualizzati nella fileTable
     //Questa lista però è utilizzata in più come array che come lista: i suoi elementi sono in numero sempre pari a MAXFILES; se il num è 0 il file in realtà non è caricato in memoria: i num infatti partono da 1. In futuro andrebbe convertito in una vera lista

  QList <int> varMaxNumsLst; //Lista dei numeri di variabili dei files correntemente visualizzati nella fileTable


//  EPlotType plotType;


  CSimOut* mySO[MAXFILES];
  CPlotWin *plotWin1, *plotWin2, *plotWin3, *plotWin4, *myPlotWin;
  CFourWin *myFourWin;
  CProgOptions *myProgOptions;
  CVarTableComp *myVarTable;

  QString computeCommonX(void);
  QString loadFile(int fileIndex, QString fileName, bool refresh=false, bool refreshUpdate=false, QString tShift="0");
  QString loadFileList(QStringList fileNameList, QString tShift="0");
  QString loadFileListLS(QStringList fileNamesList, QList <int>fileNumList, QStringList tShiftLst);
  void removeFile(int row_);
  void selectFile(int row);


  QPoint toInPrimaryScreen(QPoint inPoint, int pixelMargin=100);
  Ui::CDataSelWin *ui;

};

#endif // CDATASELWIN_H
