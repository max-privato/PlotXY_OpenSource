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

#include "CDataSelWin.h"
#include "CLineCalc.h"
#include "ExcludeATPCode.h"
#include <QApplication>
#include <QDebug>
#include <QFileDialog>
#include <QMimeData>
#include <QMessageBox>
#include <QScreen>
#include <QScrollBar>
#include <QSettings>
#include <QTimer>
#include "ui_CDataSelWin.h"
#include <QResizeEvent>
#define PIXELREDUCTION 0 //number of pixels by which the numbers on varMenu are reduced in comparison to variable names
#if defined(Q_OS_MAC)
   #define FACTORROWS 1.3
#else
   #define FACTORROWS 1.2
#endif
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#define min(a, b)  (((a) < (b)) ? (a) : (b))
int myTestVariable;
extern SGlobalVars GV; //definite in main(); struttura in "Globals.h"


void CDataSelWin::adaptToDPI(int currentDPI_, int maxHeight_){
    /*  Funzione per adattare il comportamento del programma ai DPI.
     * Nella prima implementazione era stato messo nel costruttore di DatasSelWin.
     * Ora è isolato in quanto il programma è diventato anche multiple-screeen-aware.
     * Può benissimo capitare che il secondo schermo abbiaia una risoluzione differente,
     * ad esempio minore di qualella del primo. Se la differenza è consistente è probabile
     * che abbia un diverso valore di DPI.
     * Ed esempio lo schermo prirncipale può avere 144 DPI e il secondario 96.
     * In questo caso devo riconoscere quando mi sto spostando a dei DPI differenti
     * e adattare la visualizzazione.
     *
     * Notare che il sistema cambia automaticamente le dimensioni dei pixel specificati in
     * punqi quando la mezzeria della finestra attraversa lo schermo. Quello sarebbe il
     * momento ideale per cambiare anche i settaggi personalizzati del programma, ma
     * purtroppo non risulta emesso un evento in corrispondenza di quel cambiamento,
     * Con la presente funzione quindi i font vengono cambiati dal sistema quando la
     * mezzeria della finestra attraversa il bordo dello schermo, e successivamente
     * le mie personalizzazioni quando il punto (0,0) lo attraversa. Notare che questo punto
     * non è l'angolo superiore sinistro della finestra visibile, ma un po' più in alto e a
     * sinistra, in quanto va incluso quello che nelle versioni di Windows antecedenti
     * alla 10 era il bordo e ora è traspartente ma fa parte delle finestre e serve
     * per risimensionarle.
     *
     *
*/
    float factorW;
    float factorH;

    //Se sono su schermi 4k aumento la dimensione della finestra rispetto a quanto progettato in Designer, ma non la raddoppio; In futuro metterò un'opzione che consentirà all'utente di scegliere fra finestre medie, piccole o grandi. Quella che faccio qui è piccola
    if(currentDPI_>100){
      factorW=qMin(1.7,0.9*currentDPI_/96.0);
      factorH=qMin(1.5,0.9*currentDPI_/96.0);
    }else {
      factorW=1.0;
      factorH=1.0;
    }
    resize(factorW*originalWidth,factorH*originalHeight);
    setMaximumWidth(factorW*originalMaxWidth);
    setMinimumWidth(factorW*originalMinWidth);

    // ALla fine la massima altezza è bene che sia pari al massimo spazio disponibile in verticale. QUesta logica è ad esempio quella di Qt Creator. Se infatti una finestra alta al massimo viene spostata verso i lbasso non si può più espandere agendo sul bordo superiore, proprio perché appare opportuno non superare l'altezza massima disponibile.
    setMaximumHeight(maxHeight_);

    saveStateLbl->resize(1.5*ui->loadTBtn->width(), ui->loadTBtn->height());

    /* Qui adatto le altezze delle tabelle file e varTable. Non lascio che sia fatto
     * in maniera automatica perché Qt lascia troppi spazi sopra e sotto il testo.
     * La cosa migliore è determinare l'altezza delle righe in funzione dell'altezza
     * del font contenuto. In tal modo ho la libertà di definire (in punti) in maniera
     * articolata il font delle celle delle tabelle, il quale è myFont, definito nel costruttore
     * sia in funzione dei tipi dello schermo che delle opzioni di programma che, infine,
     * del sistema operativo. myFont è poi passato passato sopra alle varTable e più sotto
     * alla fileTable.
    */
    int fontPxHeight=currentDPI_/96.0*myFont.pointSize()*16.0/12.0+0.5;
    int cellRowHeight=1.4*fontPxHeight;

    for(int i=0;i<=MAXFILES;i++)
      ui->fileTable->setRowHeight(i,cellRowHeight);
    for (int i=0; i<TOTROWS; i++){
        ui->varTable1->setRowHeight(i,cellRowHeight);
        ui->varTable2->setRowHeight(i,cellRowHeight);
        ui->varTable3->setRowHeight(i,cellRowHeight);
        ui->varTable4->setRowHeight(i,cellRowHeight);
    }
    //Sperimentale leggo e riscrivo gli item nella prima riga della tabella file per fare aggiornare le dimensioni in pixel del contenuto
//      ui->fileTable->setFont(myFont);
}



CDataSelWin::CDataSelWin(QWidget *parent): QMainWindow(parent), ui(new Ui::CDataSelWin){
  /* Questa funzione oltre a fare le inizializzazioni relative alla finestra CDatSelWin, fa molte inizializzazioni generali dell'intero programma, essendo questa finestra la finestra base del programma.
  Passi di inizializzazione:
  A) Setup generali
  B) eventuale caricamento files per parametri passati
  C) tutti i connect del programma eccetto quelli che hanno come origine e destinazione
     un medesimo widget diverso da CDataSelWin. Questi ultimi connect distribuiti sono:
     - connect del timer in CVarMenu
     - connect del timer in CLineChart
    E' effettuato anche un altro connect in quanto in CDataSelWin non vi è la
    visibilità delle istanze sia di signal che di slot:
    - connect di giveUnits fra CLineCalc e CVarTableComp
  D) passaggio delle opzioni di programma a plotWin e fourWin (che non sono più
     lette per accesso diretto a GV)

  Dettagli di A:
  A1: setupUI e personalizzazione DPI-aware di font dei widget e dimensioni di CDataSelWin
  A2: inizializzazione variabili semplici locali (bool, int, altro,in ord. alfabetico)
  A3: creazione di finestre  (plotWin, FourWin, progOptions)
      ** Decidere una regola uniforme di creazione delle finestre qui: solo le modali?
      ** le modali e le dialog? **
  A4: lettura settings ed eventuale personalizzazione dimensione e posizione finestre
  A5: Inizializzazioni relative alla FileTable
  A6: Inizializzazioni relative alla VarMenuTable
  A7: Inizializzazioni relative alla SelVarTable
*/
  int i;
  QString ret;

  // Fase A1: setupUI e personalizzazione DPI-aware dei font dei widget e dimensioni di CDataSelWin
  ui->setupUi(this);
  move(toInPrimaryScreen(QPoint(0,0)));
  ui->varTable1->setNameNumber(1);
  ui->varTable2->setNameNumber(2);
  ui->varTable3->setNameNumber(3);
  ui->varTable4->setNameNumber(4);

  ui->varTable1->getColorScheme(GV.PO.useOldColors);
  ui->varTable2->getColorScheme(GV.PO.useOldColors);
  ui->varTable3->getColorScheme(GV.PO.useOldColors);
  ui->varTable4->getColorScheme(GV.PO.useOldColors);

  //La ui contiene anche la savestateLbl:
  saveStateLbl= new QLabel(this);
  saveStateLbl->setFrameStyle(QFrame::Panel | QFrame::Sunken);
 //    saveStateLbl->setText("State Saved");
  QFont f=saveStateLbl->font();
  f.setBold(true);
  saveStateLbl->setFont(f);
  saveStateLbl->setAlignment(Qt::AlignCenter);
  saveStateLbl->setVisible(false);

  originalMaxWidth=maximumWidth();
  originalMinWidth=minimumWidth();
  originalWidth=geometry().width();
  originalHeight=geometry().height();

  QScreen *screen=QGuiApplication::primaryScreen();
  currentDPI=screen->logicalDotsPerInch();

  int maxHeight=FRACTIONOOFSCREEN*screen->availableGeometry().height();
    /* Il secondo parametro che passo nella seguente chiamata a funzione è la massima altezza che posso dare alla finestra. Vorrei che la finestra restasse sempre nello spazio disponibile, inclusa la propria intestazione. Purtroppo questo mi risulta al momento impossibile in quanto i due metodi della presente finestra geometry() e frameGeometry(), che dovrebbero dare valori differenti, danno gli stessi rettangoli e coincidono con la zona utile, al netto di frame e zona del titolo, cioè con quello che secondo la documentazione dovrebbe essre geometry(), ma non frameGeometry().
    * Quindi mi accontento di passare il 95% della altezza utile sul desktop. Il risultato sarà perfetto solo se l'altezza utile di CDataSelWIn è il 95% dell'altezza totale, cioè se le parti accessorie occupano il 5% del totale
*/
    adaptToDPI(currentDPI, maxHeight);

  QFont font8Pt=QFont("arial",8);
  QFont font9Pt=QFont("arial",9);
  QFont font10Pt=QFont("arial",10);
  QFont font11Pt=QFont("arial",11);
  QFont font12Pt=QFont("arial",12);
 if (GV.PO.largerFonts)
   myFont=font9Pt;
 else
   myFont=font8Pt;

#if defined(Q_OS_MAC)
 if (GV.PO.largerFonts)
   myFont=font12Pt;
 else
   myFont=font11Pt;
 ui->tabWidget->setMinimumWidth(230);
#endif
  ui->loadTBtn->setFont(myFont);
  ui->refrTBtn->setFont(myFont);
  ui->fourTBtn->setFont(myFont);
  ui->plotTBtn->setFont(myFont);
  ui->resetTBtn->setFont(myFont);
  ui->saveVarsBtn->setFont(myFont);
  ui->eqTBtn->setFont(myFont);
  //Aumento i punti delle label testuali
  ui->EqualiseBox->setFont(myFont);
  ui->arrTBtn->setFont(myFont);
  ui->allToBtn->setFont(myFont);
  ui->fileTable->setFont(myFont);
  ui->toWin1Btn->setFont(myFont);
  ui->tabWidget->setFont(myFont);
  ui->varTable1->getFont(myFont);
  ui->varTable2->getFont(myFont);
  ui->varTable3->getFont(myFont);
  ui->varTable4->getFont(myFont);


  // Fase A2: inizializzazione variabili semplici locali (bool, int, altro,in ord. alfabetico)
  doubleClicking=false;
  fileLoaded=false;
  goneToSingleFile=false;
  refreshUpdate=ui->refrTBtn->isChecked();
  updatingPlot=false;

  funXVar=NULL;
  fourTableIndex=-1;
  numOfLoadedFiles=0;
  selectedFileIdx=-1;
  selectedFileRow=-1; //Valore inaccettabile nel normale uso; viene usato per indicare
                         //che ancora non è stato selezionato alcun file

  headerGray.setRgb(210,210,210);
  neCellBkColor.setRgb(240,240,240);

  for(i=0; i<MAXFILES; i++){
    fileNamesLst.append(" ");
    fileNumsLst.append(0);
    varMaxNumsLst.append(0);
    topIndex[i]=0;
    freeFileIndex<<i;
  }
  myVarTable=ui->varTable1;
  //In alcuni PC la seguente riga crea un segment fault. Probabilmente perché a questo punto la tabWidget non è stata in realtà ancora creata
  //ui->tabWidget->setCurrentIndex(0);

  //generazione e inizializzazione delle saveStrings:
  saveStrings=new QString[ui->fileTable->columnCount()];
  for (int i=0; i<ui->fileTable->columnCount(); i++)
      saveStrings[i]="";

  // Creazione e inizializzazione degli oggetti mySO:
  for(i=0;i<MAXFILES; i++) {
    mySO[i]=new CSimOut(this);
    mySO[i]->useMatLib=GV.PO.useMatLib;
  }


  //Fase A3: creazione di finestre  (plotwWin, FourWin, progOptions)  ** Decidere una regola uniforme di creazione delle finestre qui: solo le modali? le modali e le dialog? **

  plotWin1 = new CPlotWin(); plotWin1->setWindowTitle("Plot 1");
  plotWin2 = new CPlotWin(); plotWin2->setWindowTitle("Plot 2");
  plotWin3 = new CPlotWin(); plotWin3->setWindowTitle("Plot 3");
  plotWin4 = new CPlotWin(); plotWin4->setWindowTitle("Plot 4");
  myPlotWin=plotWin1;

  myFourWin= new CFourWin();
  myFourWin->setWindowTitle("MC's PlotXY - Fourier chart");

  myProgOptions= new CProgOptions(this);
  myProgOptions->setWindowTitle("MC's PlotXY - Program options");


  // ***
  //Fase A4: lettura settings ed eventuale personalizzazione dimensione e posizione finestre
  // ***
  QSettings settings;
  if(GV.PO.rememberWinPosSize){
    GV.multiFileMode=settings.value("multifileMode").toBool();
    //La seguente riga non può essere usata perché in questo momento non sono stati ancora allocati gli items per la varTable, e quindi non si possono fare su di essa le trasformazioni richieste (altrimenti: SEGMENT FAULT!) Pertanto essa viene spostata in showEvent().
    //if(!multiFile) on_multifTBtn_clicked(true);

    resize(settings.value("dataSelWin/size", size()).toSize());

    plotWin1->resize(settings.value("plotWin1/size").toSize());
    plotWin2->resize(settings.value("plotWin2/size").toSize());
    plotWin3->resize(settings.value("plotWin3/size").toSize());
    plotWin4->resize(settings.value("plotWin4/size").toSize());
    myFourWin->resize(settings.value("fourWin/size", myFourWin->size()).toSize());


  /* Gestione smart degli schermi.
   * 1) mi devo assicurare che le finestre plot siano all'interno dello spazio
   *    di visualizzazione disponibile. Questo non è sempre vero ad esempio
   *    se sono state salvate quando ero con risoluzione più elevata oppure
   *    con schermo secondario attivo e ora la risoluzione o il numero di schermi
   *    è minore
   * 2) mi devo assicurare che la posizione delle finestre non cora le barre
   *   di sistema. Questo accadrebbe ad es. sul mac nella posizione default
   *   delle finestre, cioè (0,0)
  */

    bool someWinDisplaced=false;

    QPoint posPoint;
    int screenCount=QGuiApplication::screens().count();
    QScreen *firstScreen=QGuiApplication::primaryScreen();
    QScreen *lastScreen=QGuiApplication::screens()[screenCount-1];
    QRect firstScrAvGeometry=firstScreen->availableGeometry();
    QRect lastScrAvGeometry=lastScreen->availableGeometry();
    int firstScrAvRight=firstScrAvGeometry.right();
    int lastScrAvRight=lastScrAvGeometry.right();

    posPoint=settings.value("dataSelWin/pos").toPoint();
    //Primo passaggio: riporto dentro se lo spazio orizzontale disponibile
    // si è ridotto ad esempio perché non ho più lo schermo secondario:
    if(posPoint.x()+0.5*this->width()>lastScrAvRight){
       someWinDisplaced=true;
       posPoint.setX(lastScrAvRight-this->width()-5);
    }
    this->move(posPoint);
    //Secondo passaggio: se sono nello schermo primario devo evitare di andare
    // a finire nelle zone riservate dalle barre di sistema
    if (posPoint.x()+this->width()<firstScrAvRight)
        this->move(toInPrimaryScreen(posPoint));

    posPoint=settings.value("plotWin1/pos").toPoint();
    if(posPoint.x()+0.5*plotWin1->width()>lastScrAvRight){
       someWinDisplaced=true;
       posPoint.setX(lastScrAvRight-plotWin1->width()-5);
    }
    plotWin1->move(posPoint);
    if (posPoint.x()+plotWin1->width()<firstScrAvRight)
        plotWin1->move(toInPrimaryScreen(posPoint));

    posPoint=settings.value("plotWin2/pos").toPoint();
    if(posPoint.x()+0.5*plotWin2->width()>lastScrAvRight){
       someWinDisplaced=true;
       posPoint.setX(lastScrAvRight-plotWin2->width()-5);
    }
    plotWin2->move(posPoint);
    if(posPoint.x()+plotWin2->width()<firstScrAvRight)
       plotWin2->move(toInPrimaryScreen(posPoint));

    posPoint=settings.value("plotWin3/pos").toPoint();
    if(posPoint.x()+0.5*plotWin3->width()>lastScrAvRight){
      someWinDisplaced=true;
       posPoint.setX(lastScrAvRight-plotWin3->width()-5);
    }
    plotWin3->move(posPoint);
    if (posPoint.x()+plotWin3->width()<firstScrAvRight)
       plotWin3->move(toInPrimaryScreen(posPoint));

    posPoint=settings.value("plotWin4/pos").toPoint();
    if(posPoint.x()+0.5*plotWin4->width()>lastScrAvRight){
       someWinDisplaced=true;
       posPoint.setX(lastScrAvRight-plotWin4->width()-5);
    }
    plotWin4->move(posPoint);
    if (posPoint.x()+plotWin4->width()<firstScrAvRight)
       plotWin4->move(toInPrimaryScreen(posPoint));

    posPoint=settings.value("fourWin/pos").toPoint();
    if(posPoint.x()+0.5*myFourWin->width()>lastScrAvRight){
       someWinDisplaced=true;
       posPoint.setX(lastScrAvRight-myFourWin->width()-5);
    }
    myFourWin->move(posPoint);
    if (posPoint.x()+myFourWin->width()<firstScrAvRight)
       myFourWin->move(toInPrimaryScreen(posPoint));

    if (someWinDisplaced){
      QMessageBox::warning(this, "MC's PlotXY",
       "Some plot window(s) saved outside current available space.\n"
       "To allow seeing, they will be moved inside");
    }

  }else{
// FRAMEGEOMETRY_COMMENT (non cancellare serve come label per un commento incrociato)
/* A questo punto andrebbe settata la posiione default delle finestre in maniera analoga
 * a come viene fatto cliccando sul bottone "arrange". E' stato però visto che QWidget
 * vornisce informazioni leggermente errate sulla FrameGeometry se una finestra, pur
 * costruita, non è ancora visualizzata. La funzione fornisce invece l'indicazione corretta
 * quando siamo all'interno di ShowEvent.
 * Questo problema può essere risolto per la finestra DataSelWin scegliendo la posizione
 * orizzontale delle finestre Plot1 e Plot3 non da qui ma da ShowEvent, e così è stato fatto.
 * E' molto più complicato per la posizione verticale delle finestre Plot2 e Plot4,
 * e per la posizione orizzontale di Plot2 e Plot4. Al momento mantengo questo errore,
 * che è di piccola entità.
*/
    // on_arrTBtn_clicked();
  }


  //Eventuale attivazione tasto ricaricamento dello stato:
  QStringList groups = settings.childGroups();
  if(groups.contains("programState"))
  ui->loadStateTBtn->setEnabled(true);

  // ***
  // Fase A5: Inizializzazioni relative alla tabella File
  // ***
  //Tolgo la scrollbar verticale:
  ui->fileTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  //Inizializzazione degli items per la fileTable:
  QString hdrs[7]={"  ","f","  FileName  ","# of vars ","# of points", "Tmax", "Tshift"};

  for(int i=0;i<ui->fileTable->rowCount();i++){
    for(int j=0;j<ui->fileTable->columnCount();j++){
      QTableWidgetItem *item=new QTableWidgetItem;

      item->setFont(myFont);
      if(i==0){
        item->setText(hdrs[j]);
        item->setBackgroundColor(headerGray);
        item->setFlags(item->flags()&~ (Qt::ItemIsEditable+Qt::ItemIsSelectable));
//Tmax e TShift devono essere a fondo leggermente più chiaro per far capire che sono cliccabili
        if(j>4)
          item->setBackgroundColor(headerGray.lighter(110));
          item->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      }else{
        if(j!=6)
          item->setFlags(item->flags()&~Qt::ItemIsEditable);
        item->setBackgroundColor(neCellBkColor);
        //La sola colonna con TShift la metto bianca per far capire che è editabile:
        QColor white;
        white.setRgb(255,255,255);
        if(j==6)
          item->setBackgroundColor(white);
      }
      if(j<2)
        item->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      ui->fileTable->setItem(i,j,item);
    }
  }
  //Normalmente l'ultima colonna (dei tShift) è nascosta:
  ui->fileTable->hideColumn(ui->fileTable->columnCount()-1);

  ui->fileTable->resizeColumnsToContents();
  //Ora aumento la colonna FileName di ADJUST pixel e riduco di altrettanto quella di Tmax:
  #define ADJUST 0
  ui->fileTable->setColumnWidth(2,ui->fileTable->columnWidth(2)-ADJUST);
  ui->fileTable->setColumnWidth(5,ui->fileTable->columnWidth(5)+ADJUST);
  // rendo hide tutte le righe eccetto le prime
  if(ui->multifTBtn->isChecked()){
    for(int i=4; i<ui->fileTable->rowCount(); i++){
      ui->fileTable->hideRow(i);
    }
  }else{
    for(int i=2; i<ui->fileTable->rowCount(); i++){
      ui->fileTable->hideRow(i);
    }
  }


  // ***
  // FASE A6: Inizializzazioni relative alla tabella varMenuTable
  // ***
  //Tolgo la scrollbar orizzontale:
  sortType=noSort;
  ui->varMenuTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  ui->varMenuTable->setRowCount(0);

  // ***
  // FASE A7: Inizializzazioni relative alla tabella SelVarTable
  // ***
  ui->varTable1->neCellBkColor=neCellBkColor;
  ui->varTable2->neCellBkColor=neCellBkColor;
  ui->varTable3->neCellBkColor=neCellBkColor;
  ui->varTable4->neCellBkColor=neCellBkColor;

  myVarTable->setMultiFile(GV.multiFileMode);


  //***
  //Fase B: eventuale caricamento files per parametri passati
  //***
  //i nomi dei files vengono dopo le opzioni, quindi GV.PO.firstFileIndex contiene l'indice del primo file da caricare:
  QStringList fileNameLst;
  int i0=GV.PO.firstFileIndex;

   for(int i=i0; i<QCoreApplication::arguments().count(); i++){
    if(i==MAXFILES+i0-1)break;
    fileNameLst.append(QCoreApplication::arguments().at(i));
  }
  loadFileList(fileNameLst);

  //Fase C:  tutti i connect del programma eccetto quelli che hanno come origine e destinazione un medesimo widget diverso da CDataSelWin.
  //connect(myProgOptions,SIGNAL(programOptionsChanged(SOptions)),plotWin1,SLOT(updateChartOptions(SOptions)));
  connect(myProgOptions,&CProgOptions::programOptionsChanged,plotWin1,&CPlotWin::updateChartOptions);
  connect(myProgOptions,&CProgOptions::programOptionsChanged,plotWin2,&CPlotWin::updateChartOptions);
  connect(myProgOptions,&CProgOptions::programOptionsChanged,plotWin3,&CPlotWin::updateChartOptions);
  connect(myProgOptions,&CProgOptions::programOptionsChanged,plotWin4,&CPlotWin::updateChartOptions);

  connect(myProgOptions,&CProgOptions::programOptionsChanged,myFourWin,&CFourWin::updateChartOptions);


//    connect(ui->varMenuTable, SIGNAL(myCellClicked(int,int,bool)), this, SLOT(varMenuTable_cellClicked(int,int,bool)));
  connect(ui->varMenuTable, &CVarMenu::myCellClicked, this, &CDataSelWin::varMenuTable_cellClicked);
  // Il numero delle righe nelle varTable# è selezionato automaticamente nei rispettivo costruttore a TOTROWS che è pari a MAXVARS+1  (attualmente quest'ultimo è pari a 9)
  // connect che servono per superare lo strano problema dell'ultima cella che rimane gialla:
  connect(ui->varMenuTable,SIGNAL(draggingDone()),ui->varTable1,SLOT(blankCell()));
  connect(ui->varMenuTable,SIGNAL(draggingDone()),ui->varTable2,SLOT(blankCell()));
  connect(ui->varMenuTable,SIGNAL(draggingDone()),ui->varTable3,SLOT(blankCell()));
  connect(ui->varMenuTable,SIGNAL(draggingDone()),ui->varTable4,SLOT(blankCell()));

//    connect(ui->varMenuTable,SIGNAL(groupSelected(int,int)),this, SLOT(groupSelected(int,int)));
  connect(ui->varMenuTable,&CVarMenu::groupSelected,this, &CDataSelWin::groupSelected);

//    connect(ui->varTable1,SIGNAL(contentChanged()),this,SLOT(varTableChanged()));
  connect(ui->varTable1,&CVarTableComp::contentChanged,this,&CDataSelWin::varTableChanged);
  connect(ui->varTable2,&CVarTableComp::contentChanged,this,&CDataSelWin::varTableChanged);
  connect(ui->varTable3,&CVarTableComp::contentChanged,this,&CDataSelWin::varTableChanged);
  connect(ui->varTable4,&CVarTableComp::contentChanged,this,&CDataSelWin::varTableChanged);

  connect(ui->varTable1,&CVarTableComp::queryFileInfo,this,&CDataSelWin::giveFileInfo);
  connect(ui->varTable2,&CVarTableComp::queryFileInfo,this,&CDataSelWin::giveFileInfo);
  connect(ui->varTable3,&CVarTableComp::queryFileInfo,this,&CDataSelWin::giveFileInfo);
  connect(ui->varTable4,&CVarTableComp::queryFileInfo,this,&CDataSelWin::giveFileInfo);


//    connect(plotWin1,SIGNAL(winActivated(int)),this,SLOT(updateSheet(int)));
  connect(plotWin1,&CPlotWin::winActivated,this,&CDataSelWin::updateSheet);
  connect(plotWin2,&CPlotWin::winActivated,this,&CDataSelWin::updateSheet);
  connect(plotWin3,&CPlotWin::winActivated,this,&CDataSelWin::updateSheet);
  connect(plotWin4,&CPlotWin::winActivated,this,&CDataSelWin::updateSheet);


  //    D) passaggio delle opzioni di programma a plotWin e fourWin (che non sono più lette per accesso diretto a GV)
  emit myProgOptions->programOptionsChanged(GV.PO);

  //A questo punto posso mettere il cursore standard del mouse (il cursore di "busy" è stato attivato in main())
 qApp->restoreOverrideCursor();
}

void CDataSelWin::checkCalcData(SXYNameData calcData, QString & ret){
   /* Qui nella routine chiamante occorre verificare che i files e le variabili siano esistenti.
    */
  ret="";
  for(int i=0; i<calcData.varNumsLst.count(); i++){
    int iFile=calcData.varNumsLst[i].fileNum; //Numeri che contano a radice 1
    if (iFile > numOfLoadedFiles){
      ret="Requested variable from a non-existent file.\nFile N. "+
            QString::number(calcData.varNumsLst[i].fileNum);
      break;
    }
    if (calcData.varNumsLst[i].varNum > mySO[iFile-1]->numOfVariables){
      ret="Requested plot of a non-existent variable.\nFile N. "+
          QString::number(calcData.varNumsLst[i].fileNum) + ", variable n. " +
          QString::number(calcData.varNumsLst[i].varNum);
      break;
    }
  }
}


void CDataSelWin::closeEvent(QCloseEvent *){
  /* Questo evento è eseguito quando l'utente clicca sul bottone di chiusura della finestra della barra di systema.
  */
  QSettings settings;
  if(!GV.PO.rememberWinPosSize)
    goto quit;
  settings.setValue("dataSelWin/size", size());
  settings.setValue("dataSelWin/pos", pos());
  settings.setValue("multifileMode", GV.multiFileMode);

  settings.setValue("plotWin1/size", plotWin1->size());
  settings.setValue("plotWin2/size", plotWin2->size());
  settings.setValue("plotWin3/size", plotWin3->size());
  settings.setValue("plotWin4/size", plotWin4->size());
  settings.setValue("plotWin1/pos",  plotWin1->pos());
  settings.setValue("plotWin2/pos",  plotWin2->pos());
  settings.setValue("plotWin3/pos",  plotWin3->pos());
  settings.setValue("plotWin4/pos",  plotWin4->pos());

  settings.setValue("fourWin/pos",   myFourWin->pos());
  settings.setValue("fourWin/size",  myFourWin->size());

quit:
  /*  La cancellazione di plotWin# delle seguenti 4 righe non è indispensabile in quanto subito dopo vi è un qApp->quit() che causa l'uscita da a.exec() in main(), e quindi l'uscita dall'applicazione. A quanto si legge su Internet, all'uscita dall'applicazione il sistema operativo è in grado di liberare tutta la memoria allocata dal programma non già liberata durante l'esecuzione da un delete.
   * Il richiamo esplicito a delete qui riportato mi fa fare la chiusura delle finestre e la relativa disallocazione prima che il controllo del programma sia passato al sistema operativo.
   *
   * NOTA de avessi messo questi delete in ~CDataSelWin() vi sarebbe stato il seguente problema (che si evidenzia solo nel mio Win XP in VirtualBox del calcolatore fisso HP Pavillon):
   * se chiudo la DatSelWin con una finestra plot aperta, tale finesta plot rimane aperta.
   * Questo comportamento è facilmente spiegabile. Occorre per prima cosa rimarcare che si esce da application.exec()(cioè la riga a.exec() di main()) soltanto quando tutte le finestre primarie (quelle cioè che non hanno parent) vengono chiuse. Pertanto fintanto che rimangono aperte finestre plotWin#, non si esce da a.exec() di main(). E solo dopo che, in main(), si esce da a.exec(), si va oltre. Dopo a.exec() in main non vi sono altre righe, quindi si passa al delete delle variabili automatiche. Una di queste è CDataSelWin w; il delete di w provoca l'esecuzione di ~CDataSelWin(). Quindi se metto i delete di plotWin# in ~CDataSelWin() essi non vengono esequiti quando l'utente clicca sulla "x" di CDataSelWin, se è ancora visibile qualche finestra di plot, in quanto in tal caso CDataSelWin non è l'ultima finestra primaria che viene chiusa. Vengono solo esequiti successivamente, cioè quanto anche tutte le plotWin# sono state manualmente chiuse dall'utente.
   *
   * Infine una considerazione sul confronto fra "delete plotWin1" e "plotWin1->close()".
   * QWidget::close() non libera la memoria e non esegue il distruttore, a meno che l'attributo Qt::WA_DeleteOnClose non sia stato settato ad esempio alla creazione della finestra(non è settato per default).
   * Vista la situazione la soluzione con il delete, standard del C++,  appare preferibile.
  */
  delete plotWin1;
  delete plotWin2;
  delete plotWin3;
  delete plotWin4;
  /*La seguente riga non è necessaria in quanto a questo punto tutte le plotWin si sono chiuse e l'ultima finestra primaria, CDataSelWin si sta per chiudere.
    Quanto tutte le finestre primarie sono chiuse qApp->quit() viene chiamata automaticamente, e quindi si esce da a.exec(), e quindi da main().  Per riferimento guardare QApplication::lastWindowClosed():
    La riga non è necessaria ma non fa male! Se la lasciamo il programma si chiude correttamente anche qualora in un secondo momento (ma non credo proprio) si dovesse porre
    quitOnLastWindowClosed=false
    */
     qApp->quit();
}

QString CDataSelWin::computeCommonX(void){
  /* Questa funzione serve per valutare il nome a comune della variabile X nel caso in cui si operi in modalità multifile.
   Questo nome viene attribuito secondo i seguenti criteri:
   - se tutte le variabili X hanno medesimo nome, esso viene usato come nome
     comune
   - in caso contrario, se tutte le variabili X cominciano per t o f, esse sono
     interpretate rispettivamente per tempo o frequenza e al nome comune viene
     attribuita la stringa "t*" o "f*" rispettivamente
   - in tutti gli altri casi al nome comune viene attribuita la stringa "x".
   */
  bool allSameName=true, allSameFirstChar=true;
  int i;
  QString commonXName="", commonX;
  //Per prima cosa, tramite il seguente ciclo for, valuto il candidato a fare commonX, ovvero il nome comune per le variabili X:
  for(i=0; i<MAXFILES; i++){
    if(!freeFileIndex.contains(i)){
      commonXName=mySO[i]->varNames[0];
      commonX=commonXName[0];
      break;
    }
  }
  if(commonXName==""){
      QMessageBox::critical(this,"CDataSelWin","Error \"CommonX\" in CDataSelWin.cpp");
    return "??";
  }
  //Ora vedo se tutti i files hanno il medesimo CommonXName e CommonX
  for(i=0; i<MAXFILES; i++){
    //se il file i non è caricato non lo considero:
    if(freeFileIndex.contains(i))continue;
    if(mySO[i]->varNames[0]!=commonXName) allSameName=false;
    if(mySO[i]->varNames[0][0]!=commonX[0])  allSameFirstChar=false;
  }
  if(allSameName) return commonXName;
  if(allSameFirstChar)return commonX.append('*');
  if(!allSameFirstChar)return "x";
  return commonX;
}

void CDataSelWin::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();//mette il puntatore in posizione di accettazione
}


void CDataSelWin::dropEvent(QDropEvent *event)
{
    /* Funzione per il caricamento dei files che vengono "droppati" sulla finestra*/

    int i;
    QString ret;
    QStringList fileList;
    const QMimeData *mimeData = event->mimeData();
    for(i=0; i<mimeData->urls().count(); i++)
      fileList.append( mimeData->urls().at(i).path());
    ret=loadFileList(fileList);
//    QResizeEvent *e=NULL;
    resizeEvent(NULL);
    ui->varMenuTable->resizeColumnsToContents();
    if(ret=="")
      event->acceptProposedAction();
    else
      QMessageBox::critical(this,"CDataSelWin.cpp","unable to load file: "+ret);
    ui->multifTBtn->setEnabled(true);

}



void CDataSelWin::groupSelected(int beginRow, int endRow){
    QString varName, fileName;
    for(int row=beginRow; row<=endRow;row++){
        varName=ui->varMenuTable->item(row,1)->text();
        fileName=ui->fileTable->item(selectedFileRow,2)->text();
        int varNum=ui->varMenuTable->item(row,0)->text().toInt();
        myVarTable->setVar(varName,varNum,selectedFileIdx+1,false, false);
    }
    if(myVarTable->numOfTotVars>1){
        ui->plotTBtn->setEnabled(true);
        ui->saveVarsBtn->setEnabled(true);
    }
    if(myVarTable->numOfTotVars==2 && myVarTable->xInfo.isMonotonic)
        ui->fourTBtn->setEnabled(true);
    else
        ui->fourTBtn->setEnabled(false);
}


float *  CDataSelWin::integrate(float * x, float * y, int nPoints){
  /* Effettua il calcolo della funzione integrale y(x) fra 0 e X con il metodo dei trapezi.
   * Riscrive l'integrale nello stesso vettore y  di ingresso.
   */
    int i;
    float integ=0.0;
    for(i=0; i<nPoints-1; i++){
//      if(fabs(x[i]-x[i+1])) continue;
      integ +=(x[i+1]-x[i])*(y[i]+y[i+1])/2.;
      y[i]=integ;
    }
    for(i=nPoints-1; i>0; i--)
      y[i]=y[i-1];
    y[0]=0;
    return y;
}

QString CDataSelWin::integrateUnits(QString unitS_){
    QString unitS;
    if(unitS_!=""){
      if(unitS_!="W")
        unitS=unitS_+".s";
      else
        unitS="J";
    }
    return unitS;
}


QString CDataSelWin::loadFile(int fileIndex, QString fileName, bool refresh, bool refreshUpdate, QString tShift){
    /*
    fileIndex: indice del file da caricare, argomento di mySO (da 0 a MAXFILES)
               vista la possibilità che si dà all'utente di caricare e scaricare
               files arbitrariamente, gli indici possono non essere consecutivi.
               Il numero visualizzato in multiFile è fileIndex+1.
    fileName:  nome completo del file da caricare (con o senza percorso)

    Funzione per il caricamento di file sia generico che tipo "refresh".
    Il programma fa, nell'ordine:
    1. Carica in memoria, alla posizione fileIndex, il file il cui nome fullName
    2. Copia i relativi dati nella fileTable. In caso di refresh i dati vengono
       scritti sopra i dati preesistenti; in caso contrario il file caricato
       diventa quello selezionato
    3. Aggiorna numOfSelFiles
    4. I nomi delle variabili presenti nel file caricato vengono messi nella
       relativa varMenuTable
    5f. Se refresh==false i dati del file vengono messi nella prima riga libera
        della fileTable (che deve esistere); la varTable viene resettata e di
        essa viene compilata la prima riga
    5t. se invece  refresh==true si procede ad un refresh, secondo le seguenti
        fasi:
        . i dati del file letto vengono messi nella riga contenente già i dati
          del file "selezionato"
        . la selectedVarsGrid invece di essere resettata viene solo controllata
          per eliminare eventuali variabili non più presenti nel file rinfrescato
************************
      . viene effettuato l'aggiornamento dei grafici di tutte le finestre
  */
  bool updatingFile=false; //true se in singleFile copio un file sopra il precedente (rimane false in caso di refresh o refresUpdate)
  int i, freeGridRow;
  QFileInfo FI=fileName;
  //Il fileName passato può contenere il path oppure no. Il nome sicuramente senza path è il seguente strictName
  QString fullName=FI.absoluteFilePath(), strictName=FI.fileName(), ext=FI.suffix();
  QString ret;
  SReturn retS;

  //Fase 0: verifica di congruenza.
  if (freeFileIndex.isEmpty())
    return "File table already full";

  if(selectedFileIdx==fileIndex && !refresh && !refreshUpdate)
    updatingFile=true;

  //Fase 1: caricamento del file di nome fileName.
  ext=ext.toLower();
  if(ext!="adf" && ext!="cfg" && ext!="lvm" && ext!="mat" && ext!="pl4" && ext!="csv") {
    ret="file extension\""+ext+"\" is  invalid\n(only \nadf, cfg, csv, lvm, mat, or pl4 \nare allowed)";
    return ret;
  }
  mySO[fileIndex]->commasAreSeparators=GV.PO.commasAreSeparators;
  mySO[fileIndex]->trimQuotes=GV.PO.trimQuotes;
  retS.code=0;
  if(ext=="adf") ret=mySO[fileIndex]->loadFromAdfFile(fileName);
  if(ext=="cfg") ret=mySO[fileIndex]->loadFromComtradeFile(fileName);
  if(ext=="csv") ret=mySO[fileIndex]->loadFromAdfFile(fileName, true);
  if(ext=="lvm") ret=mySO[fileIndex]->loadFromLvmFile(fileName);
  if(ext=="mat") ret=mySO[fileIndex]->loadFromMatFile(fileName);

  if(ext=="pl4"){
    #ifdef EXCLUDEATPCODE
      retS.code=1;
      retS.msg="Unknown file extension \"pl4\".  Ignoring file:\n"
          +fileName+"\nand all subsequent files in the input list";
    #else
      retS=mySO[fileIndex]->loadFromPl4File(fileName);
    #endif
  }

  if (retS.msg!="") ret=retS.msg;
  if(retS.code==2){
      QMessageBox::warning(this,"",retS.msg);
      qDebug()<<"warning 1";
  } else
    if(ret!=""){
      QString msg;
      if(refresh)
        msg="Error when reloading file ";
      else
        msg="Error when loading file ";
      msg=msg + fullName + "\n\nReason:\n"+ret;
      ret=msg;
      return ret;
    }

  if(!updatingFile){
    freeFileIndex.remove(fileIndex);
//    varMaxNumsLst.append(mySO[fileIndex]->numOfVariables);
//    fileNamesLst.append(strictName);
//    fileNumsLst.append(fileIndex+1);
    varMaxNumsLst[fileIndex]=mySO[fileIndex]->numOfVariables;
    fileNamesLst[fileIndex]=strictName;
    fileNumsLst[fileIndex]=fileIndex+1;
  }
  ui->saveStateTBtn->setEnabled(true);

  /* Se il numero di punti è inferiore a 2 non posso fare il grafico. Mentre nel
   * caso di caricamento tramite pulsante Plot questa condizione viene intercettata
   * al momento della selezione delle variabili da plottare (così si consente
   * all'utente di vederne l'elenco), nel caso di Refresh viene intercettata qui.
   * Peraltro se il refresh è automatico non viene emesso messaggio di errore.
  */
  if(refresh && mySO[fileIndex]->numOfPoints<2){
    return 0;
  }

  //Fase 2: Copia dei dati nella fileTable (attraverso i suoi items).
  //Trovo la riga su fileTable su cui scrivere e la metto in selectedFileRow (se refresh è true selectedFileRow e selectedFileIdx non vanno cambiati):
  if(!refresh && !updatingFile){
    freeGridRow=-1;
    if(GV.multiFileMode){
      for(i=1; i<=MAXFILES; i++)
        if(ui->fileTable->item(i,FILENAMECOL)->text()=="")break;
      freeGridRow=i;
    }else
      freeGridRow=1;

    if (freeGridRow==-1)
      return "Internal code error N. 1";

    //Rendo la prima riga libera la riga selezionata, e copio i dati nelle celle
    if(selectedFileRow>-1)
      ui->fileTable->item(selectedFileRow,0)->setText("");
    selectedFileRow=freeGridRow;
    selectedFileIdx=fileIndex;
  } else {
      if(GV.multiFileMode)
        freeGridRow=selectedFileRow;
      else{
        freeGridRow=1;
      }
  }

  ui->fileTable->item(freeGridRow,0)->setText("x");
  QString str;
  str.setNum(fileIndex+1);

  ui->fileTable->item(freeGridRow,1)->setText(str);
  ui->fileTable->item(freeGridRow,2)->setText(strictName);
  ui->fileTable->item(freeGridRow,TSHIFTCOL)->setText(tShift);
  ui->fileTable->showRow(freeGridRow);

  QString fileTooltip, type;
  switch(mySO[fileIndex]->fileType){
    case ADF: type="Adf"; break;
    case CSV: type="Csv"; break;
    case COMTRADE: type="Comtrade"; break;
    case MAT_V4: type="Matlab V4"; break;
    case MAT: type="Matlab Ver>4"; break;
    case MAT_Modelica: type="Modelica-Matlab"; break;
    case PL4_1: type="Pl4 type 1"; break;
    case PL4_2: type="Pl4 type 2"; break;
    case LVM: type="LabView"; break;
  }

/**** IMPORTANTISSIMO ****
  Nelle seguenti righe è generato il tooltip del file name, che contiene, fra l'altro il nome del file completo di percorso.
  L'informazione del nome completo di percorso, variabile "fullName", è poi estratta dal tooltip stesso ogni volta che viene premuto il bottone di refresh.
  Pertanto AD OGNI MODIFICA CHE SI VOLESSE FARE DEL SEGUENTE CODICE DI GENERAZIONE DI TOOLTIP DOVRA' CORRISPONDERNE UN'ALTRA NEL CODICE CORRISPONDENTE IN "CDataSelWin::on_refrTBtn_clicked()"
  Inoltre AD OGNI MODIfICA CHE SI VOLESSE FARE DEL SEGUENTE CODICE DI GENERAZIONE DI TOOLTIP DOVRA' CORRISPONDERNE UN'ALTRA NEL CODICE CHE GESTISCE IL SALVATAGGIO E IL RIPRISTINO DELLO STATO"

*/
  fileTooltip="<p><B>Full name:</B> "+ fullName+"</p>";
  fileTooltip.append("<B>Type:</B> "+type);
  if(mySO[fileIndex]->runType==rtTimeRun)
      fileTooltip.append("; <I>time-based data</I>.");
  else if (mySO[fileIndex]->runType==rtFreqScan)
      fileTooltip.append("; <I>frequency scan</I>.");
  //Altrimenti runTime è rtUndefined e non aggiungo nulla
  if(GV.multiFileMode){
/*
    fileTabItems[selectedFileRow][2]->setToolTip(fileTooltip);
    fileTabItems[selectedFileRow][3]->setText(QString::number(mySO[fileIndex]->numOfVariables));
    fileTabItems[selectedFileRow][4]->setText(QString::number(mySO[fileIndex]->numOfPoints));
    fileTabItems[selectedFileRow][5]->setText(QString::number(mySO[fileIndex]->y[0][mySO[fileIndex]->numOfPoints-1]));
*/
    ui->fileTable->item(selectedFileRow,2)->setToolTip(fileTooltip);
    ui->fileTable->item(selectedFileRow,3)->setText(QString::number(mySO[fileIndex]->numOfVariables));
    ui->fileTable->item(selectedFileRow,4)->setText(QString::number(mySO[fileIndex]->numOfPoints));
    ui->fileTable->item(selectedFileRow,5)->setText(QString::number(mySO[fileIndex]->y[0][mySO[fileIndex]->numOfPoints-1]));
  }else{
/*
    fileTabItems[1][2]->setToolTip(fileTooltip);
    fileTabItems[1][3]->setText(QString::number(mySO[fileIndex]->numOfVariables));
    fileTabItems[1][4]->setText(QString::number(mySO[fileIndex]->numOfPoints));
    fileTabItems[1][5]->setText(QString::number(mySO[fileIndex]->y[0][mySO[fileIndex]->numOfPoints-1]));
*/
    ui->fileTable->item(1,2)->setToolTip(fileTooltip);
    ui->fileTable->item(1,3)->setText(QString::number(mySO[fileIndex]->numOfVariables));
    ui->fileTable->item(1,4)->setText(QString::number(mySO[fileIndex]->numOfPoints));
    ui->fileTable->item(1,5)->setText(QString::number(mySO[fileIndex]->y[0][mySO[fileIndex]->numOfPoints-1]));
  }
//  fileTabItems[selectedFileRow][6]->setText("0");
  GV.varNumsLst[fileIndex]=mySO[fileIndex]->numOfVariables;
  ui->varTable1->getVarNumVect(GV.varNumsLst);
  ui->varTable2->getVarNumVect(GV.varNumsLst);
  ui->varTable3->getVarNumVect(GV.varNumsLst);
  ui->varTable4->getVarNumVect(GV.varNumsLst);
  //Elimino eventuali caratteri di spaziatura presenti in cima o fondo ai nomi. Questo è utile quando si deve fare la marcatura sulla leggenda del grafico, in modo che il simbolo di marcatura sia il più possibile vicino all'ultimo carattere non bianco
  for(i=0; i<mySO[fileIndex]->numOfVariables; i++){
    QString str=mySO[fileIndex]->varNames[i];
    mySO[fileIndex]->varNames[i]=mySO[fileIndex]->varNames[i].trimmed();
  }

  // ***
  //Fase 3: Gestione numOfSelFiles
  // ***
  if(!updatingFile)
      numOfLoadedFiles++;
  if(numOfLoadedFiles>=MAXFILES){
    setAcceptDrops(false);
    ui->loadTBtn->setEnabled(false);
  }

  // ***
  //Fase 4: Caricamento dei nomi delle variabili sulla varMenuTable
  // ***
  fillVarMenuTable(fileIndex);

//Fase 5: Se è il primo file seleziono la variabile tempo:
//  if(myVarTable->numOfTotVars==0)
//    varMenuTable_cellClicked(0,0,false);
//    ui->resetTBtn->setEnabled(true);
  fileLoaded=true;

  /* A questo punto di norna non faccio un reset perché non voglio perdere le variabili che sono state selezionate. Se però il refresh ha comportato il cambiamento della variabile dell'asse x (cioè il tempo) allora il reset è necessario
*/
  QString varMenutime=ui->varMenuTable->item(0,1)->text();
  QString expectedVarTime=myVarTable->item(1,3)->text();
  if(!refresh || varMenutime!=expectedVarTime)
    on_resetTBtn_clicked();
  else{
      //devo eliminare dalle varie varTables eventuali variabili che non sono più presenti nel file rinfrescato. Faccio la verifica sulla base del nome, considerando solo nomi delle varrTable che si riferiscono al file corrente, che è quello rinfrescato.
      //Se un nome di una varTable non è presente nella varmenuTable, allora faccio il click sulla cella contenente il nome per togliere quella variabile.
    QList <QString> varList;
    for (int iVar=0; iVar<ui->varMenuTable->rowCount(); iVar++){
       varList.append(ui->varMenuTable->item(iVar,1)->text());
      }
    ui->varTable1->filterOutVars(varList);
    ui->varTable2->filterOutVars(varList);
    ui->varTable3->filterOutVars(varList);
    ui->varTable4->filterOutVars(varList);
  }
  ui->refrTBtn->setEnabled(true);
  return "";
}


QString CDataSelWin::loadFileList(QStringList fileNameList, QString tShift){
/* Funzione per il caricamento del contenuti dei files specificati in una lista.
 * Anche se molto compatta, è opportuno averla separata dal resto del codice
 * perché viene richiamata in tre occasioni: per la lettura di files passati
 * attraverso drag&drop, per la lettura da click sul bottone "load...", e
 * in ::CDataSelWin, FASE B.
 * La generaione di una funzione facilita la manutezione del codice, evitando
 * che cambiamenti futuri in uno dei due casi non venga riportato anche nell'altro.
*/
  int j;
  QString path, ret;

  if(fileNameList.count()>1 && !ui->multifTBtn->isChecked()){
    ui->multifTBtn->setChecked(true);
    on_multifTBtn_clicked(true);
  }

  foreach(path,fileNameList){
  //In windows, può accadere che il nome del file contenga prima dell'indicatore del disco un invalido carattere '/', ad esempio "/C:/QtSource/PlotXY/aaa.adf"
    //In tal caso devo eliminare il primo carattere:
    if(path.indexOf(':')!=-1)
      if(path[0]=='/')
         path.remove(0,1);
    // se ho droppato un unico file, sono in single file, ed esiste già un file selezionato rimango in single file e sostituisco il file precedente con quello droppato:
    if(fileNameList.count()==1 && !ui->multifTBtn->isChecked() && selectedFileIdx>-1){
      ret=loadFile(selectedFileIdx,path);  //aggiorna anche numOfSelFiles
      return ret;
    }
    //carico nel primo indice disponibile:
    for(j=0; j<MAXFILES; j++)
        if(freeFileIndex.contains(j))
            break;
    if(j==MAXFILES)break;
    // Se il file da caricare non è oltre il N. 8, ma è oltre il numero di righe correntemente visualizzate aumento la visualizzazione della filetable di una riga:
    if(numOfLoadedFiles>=visibleFileRows){
       int newVSize=ui->fileTable->height()+ui->fileTable->rowHeight(0)+1;
       ui->fileTable->setMaximumHeight(newVSize);
       ui->fileTable->setMinimumHeight(newVSize);
       visibleFileRows++;
       if(visibleFileRows>MAXFILES)
         QMessageBox::critical(this,"CDataSelWin","Critical error N. 1");
    }
    ret=loadFile(j,path,false,false,tShift);  //aggiorna anche numOfSelFiles e fileNumsLst
    if(ret!=""){
        QMessageBox::warning(this,"PlotXY-dataSelWin",ret);
        qDebug()<<"warning 2";
        return ret;
    }
    freeFileIndex.remove(j);
    if(ret!="")break;
    if(!GV.multiFileMode) break;
    if(GV.multiFileMode)
        myVarTable->setCommonX(computeCommonX());
  }
  return ret;
}

QString CDataSelWin::loadFileListLS(QStringList fileNamesList, QList <int>fileNumList, QStringList tShiftLst){
/* Versione modificata di loadFileList per fare il LoadState.
 * Fa alcune operazioni grafiche e di path e poi carica in memoria il contenuto dei files
 * i cui nomi sono presenti nella lista namesList
 * Essa va usata quando sto ripristinando lo stato del sistema, e quindi gli indici di
 * file non possono essere arbitrari, ma devono essere quelli che erano presenti durante
 * il salvataggio (e che sono stati salvati)
 * All'ingresso in questa routine è già stato ripristinato lo stato corretto di multiFile
*/
  QString pathName, ret, tShift;

  for (int i=0; i<fileNamesList.count(); i++){
    pathName=fileNamesList[i];
    tShift=tShiftLst[i];
  //In windows, può accadere che il nome del file contenga prima dell'indicatore del disco un invalido carattere '/', ad esempio "/C:/QtSource/PlotXY/aaa.adf"
    //In tal caso devo eliminare il primo carattere:
    if(pathName.indexOf(':')!=-1)
      if(pathName[0]=='/')
         pathName.remove(0,1);
     // Il seguente loadFile ha al suo interno la seguente riga:
    // fileNumsLst[fileIndex]=fileIndex+1;
    // in cui fileIndex è il primo argomento passato.
    //Questo perché è pensata per quando sto caricando un file ex novo e non ho già il numero presente in memoria.  In questo caso invece sto ripristinando lo stato del sistema il numero è già presente in fileNumsLst, e viene raddoppiato. Pertanto subito dopo il caricamento annullo gli effetti di quella riga.
    //Questo modo di procedere è artificioso ed è conseguenza del fatto che fileNumsLst è gestito come un array e non una vera lista: è inizializzato a 8 elementi all'inizio e un elemento "vuoto" è convenzionalmente rappresentato dalla presenza di un num=0
    ret=loadFile(fileNumList[i]-1,pathName,false,false,tShift);  //aggiorna anche numOfSelFiles
//    fileNumsLst.removeLast();
    // Per ragioni non scandagliata nemmeno la seguente riga va; se la commento, per ragioni al momento non scandagliate va tutto bene.
//    fileNumsLst[fileNumList[i]-1]=0;


    if(ret!=""){
        QMessageBox::warning(this,"PlotXY-dataSelWin",ret);
        qDebug()<<"warning 3";
        return ret;
    }
    freeFileIndex.remove(fileNumList[i]-1);
    if(ret!="")break;
    if(!GV.multiFileMode) break;
    if(GV.multiFileMode)
      myVarTable->setCommonX(computeCommonX());
  }
  // Adattamento altezza tabella:
  visibleFileRows=fileNamesList.count();
  int newVSize=(fileNamesList.count()+1)*ui->fileTable->rowHeight(0)+1;
  ui->fileTable->setMaximumHeight(newVSize);
  ui->fileTable->setMinimumHeight(newVSize);
  return ret;
}

void CDataSelWin::resizeEvent(QResizeEvent *){
  /* Qui gestisco il ridimensionamento della fileTable. In sostanza prima aggiusto
   * tutte le colonne al contenuto. Così facendo la larghezza può venire inferiore
   * di quella disponibile o superiore. In entrambi i casi faccio la compensazione
   * sul nome del file, il quale è sempre visualizzabile per intero tramite l'hint
   * della relativa cella.
*/
    ui->fileTable->resizeColumnsToContents();
    int maxNameWidth=this->width();
    //Ora dalla larghezza totale sottraggo quella delle celle che non voglio modificare   e il resto lo attribuisco alla cella del nome del file. Notare che le cose sono differenti sia per la differenza singleFile/Multifile, sia per il fatto che in ultima colonna posso avere Tmax o TShift. l'indice columnCOunt-1 è quello di TShift.
  if(ui->multifTBtn->isChecked()){
    maxNameWidth-=ui->fileTable->columnWidth(0); // "x" cell
    //La seguente riga dovrebbe esserci. Per ragioni sconosciute il programma va meglio se la lascio commentata
 //   maxNameWidth-=ui->fileTable->columnWidth(1); // "f" cell
  }
  maxNameWidth-=ui->fileTable->columnWidth(3); //# of vars
  maxNameWidth-=ui->fileTable->columnWidth(4); //# of Points

  if(ui->fileTable->isColumnHidden(5))
    maxNameWidth-=ui->fileTable->columnWidth(6); //Tshift
  else
    maxNameWidth-=ui->fileTable->columnWidth(5); //Tmax

  maxNameWidth-=2;
  ui->fileTable->setColumnWidth(2,maxNameWidth);

  ui->varMenuTable->resizeColumnsToContents();
    //In Windows le colonne 0 e 1 sono troppo larghe e le riduco un po':
#ifndef Q_OS_MAC
  ui->fileTable->setColumnWidth(0,0.4*ui->fileTable->columnWidth(0));
  ui->fileTable->setColumnWidth(1,0.5*ui->fileTable->columnWidth(1));
#endif

}

void CDataSelWin::showEvent(QShowEvent *){
  visibleFileRows=3;
  int tableHeight=(visibleFileRows+1)*ui->fileTable->rowHeight(0);
  ui->fileTable->setMaximumHeight(tableHeight);
  ui->fileTable->setMinimumHeight(tableHeight);

  // La posizione singleFile/MultiFile è quella default, se non ho settato GV.WinPosAndSize. Altrimenti va messo il valore settato da GV.WinPosAndSize. Se però sono passati più files vado comunque in multileMode
  //Il seguente if è particolarmente articolato in quanto garantisce che la posizione del multifile sia corretta a prescindere da come l'ho lasciato in Qt Designer
  int i0=GV.PO.firstFileIndex;
  if(QCoreApplication::arguments().count()>i0+1){
    GV.multiFileMode=true;
    ui->multifTBtn->setChecked(true);
    on_multifTBtn_clicked(true);
  } else if(GV.multiFileMode && !ui->multifTBtn->isChecked()){
    GV.multiFileMode=true;
    ui->multifTBtn->setChecked(true);
    on_multifTBtn_clicked(true);
  }else if(!GV.multiFileMode && ui->multifTBtn->isChecked()){
    GV.multiFileMode=false;
    ui->multifTBtn->setChecked(false);
    on_multifTBtn_clicked(false);
  }


  resizeEvent(NULL);
  // Per la spiegazione della seguente riga vedere il commento sotto FRAMEGEOMETRY_COMMENT
  if(!GV.PO.rememberWinPosSize)
    on_arrTBtn_clicked();

}

QPoint CDataSelWin::toInPrimaryScreen(QPoint inPoint, int pixelMargin){
    /* serve ad assicurarsi che un punto sia all'interno della zona visibile dello schermo. Anzi, esso viene posizionato all'interno di tale zona di una quantità di pixel pari a pixelMargin. Se ad esempio inPoint è pos() di una finestra esso verrà posizionato un po' dentro lo schermo in modo che l'utente possa vedere almeno un angolo della finestra */
    QPoint outPoint=inPoint;
    QScreen *screen=QGuiApplication::primaryScreen();
    QRect avGeometry=screen->availableGeometry();
    QMargins margins(0,0,pixelMargin,pixelMargin);

    avGeometry=avGeometry.marginsRemoved(margins);
    if(avGeometry.contains(inPoint))return inPoint;
    if(outPoint.x()<avGeometry.left())  outPoint.setX(avGeometry.left());
    if(outPoint.x()>avGeometry.right()) outPoint.setX(avGeometry.right());
    if(outPoint.y()<avGeometry.top())  outPoint.setY(avGeometry.top());
    if(outPoint.y()>avGeometry.bottom()) outPoint.setY(avGeometry.bottom());
    return outPoint;
}


void CDataSelWin::updateSheet(int i){
    ui->tabWidget->setCurrentIndex(i-1);
}

void CDataSelWin::varTableChanged (){
  /*Questa funzione è uno slot che viene automaticamente mandato in esecuzione
   *  dopo che è cambiata la varTable.
   * Gestisce l'attivazione dei bottoni in basso alla varTable stessa.
  */
  bool visible=myPlotWin->isVisible();
  if(visible)
    ui->updateTBtn->setEnabled(true);
  else
      ui->updateTBtn->setEnabled(false);
  if(myVarTable->numOfTotVars>1){
    ui->resetTBtn->setEnabled(true);
    ui->plotTBtn->setEnabled(true);
  }else{
    ui->resetTBtn->setEnabled(false);
    ui->plotTBtn->setEnabled(false);
    ui->updateTBtn->setEnabled(false);
  }
  if(myVarTable->numOfTotVars==2 && myVarTable->xInfo.isMonotonic)
    ui->fourTBtn->setEnabled(true);
  else
    ui->fourTBtn->setEnabled(false);
  bool test=myVarTable->allowSaving && myVarTable->numOfTotVars>1;
  ui->saveVarsBtn->setEnabled(test);
}


void CDataSelWin::selectFile(int row){
    QString str;
    //Se non c'è alcun file nella riga selezionata non faccio nulla:
    str=ui->fileTable->item(row,1)->text();
    if(str=="")return;
    ui->fileTable->item(selectedFileRow,0)->setText("");
    if(selectedFileIdx<0){
        QMessageBox::critical(this,"CDataSelWin","critical error 1");
        QCoreApplication::exit(0);
    }
    //memorizzo l'indice della prima variabile visualizzata del file finora visualizzato:
    topIndex[selectedFileIdx]=ui->varMenuTable->verticalScrollBar()->value();

    selectedFileIdx=ui->fileTable->item(row,1)->text().toInt()-1;
    str=ui->fileTable->item(row,1)->text();
    ui->fileTable->item(row,0)->setText("x");

    selectedFileRow=row;

    //Quando si commuta il file i nomi delle variabili nelle funzioni di variabile di tutte le schede di varMenuTable devono essere convertiti in nomi completi per maggior chiarezza
    ui->varTable1->fillFunNames();
    ui->varTable2->fillFunNames();
    ui->varTable3->fillFunNames();
    ui->varTable4->fillFunNames();

    //Aggiorno varMenuTable
    fillVarMenuTable(selectedFileIdx);

/* Scopo della seguente riga è ripristinare la posizione della varMenuTable.
E' stato verificato in un progetto-test a sé stante che questo comando ha l'effetto desiderato. (cartella "ProgramScrollTable").
E' stato inoltre verificato qui che riceve l'input giusto, ma per una ragione non chiarita è come se non venisse emesso! La lista infatti parte sempre dal primo valore (indice 0).*/
    ui->varMenuTable->verticalScrollBar()->setValue(topIndex[selectedFileIdx]);
}


void CDataSelWin::on_tabWidget_currentChanged(int index)
{
    if(index==0){myVarTable=ui->varTable1;  myPlotWin=plotWin1;}
    if(index==1){myVarTable=ui->varTable2;  myPlotWin=plotWin2;}
    if(index==2){myVarTable=ui->varTable3;  myPlotWin=plotWin3;}
    if(index==3){myVarTable=ui->varTable4;  myPlotWin=plotWin4;}
    myPlotWin->raise();
    myPlotWin->setFocus();
    myVarTable->setMultiFile(GV.multiFileMode);
    myVarTable->setCurrFile(selectedFileIdx);
    if(!fileLoaded)return;

    //Seleziono la variabile tempo:
    if(myVarTable->xInfo.idx==-1 && GV.multiFileMode){
       myVarTable->setCommonX(computeCommonX());
    }else{
      if(ui->varMenuTable->rowCount()>0 && myVarTable->isEmpty())
//        if(myVarTable->xInfo.idx==-1)
         varMenuTable_cellClicked(0,1,false);
    }
    //gestisco l'attivazione dei vari bottoni
    varTableChanged();
}


void CDataSelWin::on_resetTBtn_clicked()
{
    myVarTable->myReset();
    if(GV.multiFileMode && fileLoaded)
        myVarTable->setCommonX(computeCommonX());
    else
        varMenuTable_cellClicked(0,1,false);
//    varTableChanged ();
}


void CDataSelWin::varMenuTable_cellClicked(int row, int column, bool rightBtn)
{
  /* slot di cellClicked di varMenuTable
Quando ho cliccato su varMenuTable, in DataSelWin processo il comando e mando i dati a myVarTable
Da quando (lug 2015) la varMenu è stata realizzata in due colonne, per consentire un agevole sort, il click deve avvenire sempre sulla colonna di indice 1, e quindi se invece la colonna è 0 non faccio niente.
L'indice da passare a setVar è il numero presente nella prima colonna in corrispondenza della riga nella quale si è cliccato.
*/
   if(column==0)return;
   //se sono in multifile e si è selezionata la var. "t" (di riga 0) non faccio nulla:
   // if(row==0 && GV.multiFileMode)return;
  QString varName=ui->varMenuTable->item(row,column)->text();
  QString fileName=ui->fileTable->item(selectedFileRow,2)->text();

  bool bVar=ui->varMenuTable->monotonic[row];
  int varNum=ui->varMenuTable->item(row,0)->text().toInt();
  myVarTable->setVar(varName,varNum,selectedFileIdx+1,rightBtn,bVar);
}


void CDataSelWin::on_plotTBtn_clicked() {
    /* Funzione per l'esecuzione del plot.
- fase 0: analizzo fileTable e compilo timeShift
- fase 1: analizzo varTable
- fase 2: creo le matrici x1 e y1 (la loro struttura è descritta in developer.ods)
- fase 3: calcolo gli elementi di y1 connessi alle funzioni di variabili
*/

  int  iFile, iVar, iFileNew;
//  int lastIFile; //l'ultimo valore di iFile su cui si è scritto (utile per debug)
  float **x1, ***y1; //puntatori a vettori e matrici delle delle variabili selezionate.
  SFileInfo myFileInfo;
  QList <SFileInfo> filesInfo; //informazioni relative ai files di cui sono richiesti plot contiene sia le informazioni relative ai plot diretti da dati di input (uno per file) che a funzioni di variabili (uno per funzione). Pertanto il numero di elementi che contiene è pari a numOfTotPlotFiles
  QList <SCurveParam> y1Info[MAXFILES+MAXFUNPLOTS];
  QList <SXYNameData> funInfo; //una  voce della lista per ogni funzione di variabile
  CLineCalc myLineCalc;
  QString ret;
  float timeShift[MAXFILES];

 // fase 0: analizzo fileTable e compilo timeShift
  //Associo il tShift al numero di file:
  for (int i=0; i<MAXFILES; i++)
      timeShift[i]=0;
  for (int iRow=1; iRow<ui->fileTable->rowCount(); iRow++){
    bool ok;  //diventa true se la conversione in int del testo della cella fatta qui sotto ha successo
    int myIFile=-1;
    myIFile=ui->fileTable->item(iRow,1)->text().toInt(&ok)-1;
    QString str6=ui->fileTable->item(iRow,6)->text();
    if(ok)
      timeShift[myIFile]=ui->fileTable->item(iRow,6)->text().toFloat();
  }

 // - fase 1: analizzo varTable
  myVarTable->analyse();
  if(myVarTable->numOfPlotFiles>MAXFILES){
    QMessageBox::critical(this,"CDataSelWin", "Internal critical error\ncontact program maintenance",QMessageBox::Ok);
    return;
  }
  /* - fase 2: creo le matrici x1 e y1 (la loro struttura è descritta in developer.ods)    *
   * A questo punto la table è in grado di fornirmi i dati da inviare alla scheda del plot per il grafico.
   * Devo ora creare le matrici delle variabili x1[] e y1[] (il digit '1' è lasciato solo per compatibilità terminologica con la versione BCB).
   * Ogni elemento di y1 è una matrice. I primi numOfPlotFiles sono dedicati ai dati di variabili direttamente prelevate dai files di input; gli ultimi funinfo.count() sono dedicati alle variabili funzione.
   * Cosa analoga vale per le righe di x1[].
*/
  funInfo=myVarTable->giveFunInfo();
  x1=new float*[myVarTable->numOfPlotFiles+funInfo.count()];
  y1=new float**[myVarTable->numOfPlotFiles+funInfo.count()];

  /* Attribuzione alle matrici dei rispettivi valori. */
  //Nella copiatura su Y devo anche rendere contigui gli indici di files che possono essere sparpagliati:
  filesInfo.clear();
  iFileNew=-1;
  for(iFile=0; iFile<MAXFILES; iFile++){
    if(myVarTable->yInfo[iFile].count()==0)continue;
    iFileNew++;
    y1[iFileNew]=new float *[myVarTable->yInfo[iFile].count()];

//      x1[iFileNew]=mySO[iFile]->y[0];
    if(!myVarTable->xInfo.isFunction){
      x1[iFileNew]=mySO[iFile]->y[myVarTable->xInfo.idx];
    }
    myFileInfo.name=mySO[iFile]->fileInfo.fileName();
    myFileInfo.fileNum=iFile;
    myFileInfo.numOfPoints=mySO[iFile]->numOfPoints;
    myFileInfo.variableStep=mySO[iFile]->variableStep;
    myFileInfo.frequencyScan=
          mySO[iFile]->runType==rtFreqScan ||
          mySO[iFile]->runType==rtHFreqScan;
//    float dummy =fileTabItems[iFile][6]->text().toFloat();

    myFileInfo.timeShift=timeShift[iFile];
    filesInfo.append(myFileInfo);
    for(iVar=0; iVar<myVarTable->yInfo[iFile].count(); iVar++){
      int curVar=myVarTable->yInfo[iFile][iVar].idx;
      y1[iFileNew][iVar]=mySO[iFile]->y[curVar];
      y1Info[iFileNew]=myVarTable->yInfo[iFile];
    }
  }
/*
  if(myVarTable->numOfPlotFiles==1){ //in questo caso plotInfo è solo plotInfo[0]
    x1[0]=mySO[selectedFileIdx]->y[myVarTable->xInfo.idx];
  }
*/
  int plotFiles=myVarTable->numOfPlotFiles;


/* FASE 3 **** Adesso aggiungo il calcolo degli elementi di y1 collegati alle funzioni di variabili. ****/
//Nel caso in cui myVarTable.xInfo.isFunction=true una delle fun del seguente loop verrà messa come vettore delle x invece che come matrice ad una riga in y.
  for(int iFun=0; iFun<funInfo.count(); iFun++){
    SXYNameData data=funInfo[iFun];
    /* La gestione di funzioni di variabile è particolarmente complessa se non si ha
     * la garanzia che tutti i files coinvolti hanno dati in corrispondenza
     * degli stessi valori della variabile tempo.
     * Questo tipo di informazione viene fornita dall'utente di PlotXY attraverso
     * il checkbox presente nel dialog CFunStrInput.
     * Attualmente (settembre 2015), questa checkbox è disattivata e le funzioni di
     * variabili con dati provenienti da files differenti è effettuata sempre senza
     * "cross-interpolation". Si dà quindi per scontato che i files coinvolti abbiano
     * campioni tutti sugli stessi istanti, e l'unica verifica che si fa è che il
     * numero di valori presenti nei vari files sia identico, il che costituisce
     * "conditio sine qua non" le funzioni di variabili senza cross-interpolation
     * possono essere calcolate.
*/
  //  Come prima cosa devo verificare che tutti i file coinvolti nella funzione di variabili possiedono il medesimo numero di punti; altrimenti emetto un messaggio di errore
    int points=mySO[data.fileNums[0]-1]->numOfPoints;
    for (int fileInFun=1; fileInFun<data.fileNums.count(); fileInFun++){
      if(mySO[data.fileNums[fileInFun]-1]->numOfPoints!=points){
          QMessageBox::warning(this,"Invalid function",
             "Unable to plot this function:\n"
             "currently only functions operating on variables\n"
             "coming from files having the same number of points are allowed.");
            return;
        }
      }

    QVector<int> funFileIdx;
    for (int var=0; var<data.varNumsLst.count(); var++ )
      funFileIdx.append(data.varNumsLst[var].fileNum-1);

    /* Le matrici y1[i] e i vettori x[i] con i a partire da plotFiles, vanno allocati (dettagli in Developer.odt):*/
    y1[plotFiles+iFun]=CreateFMatrix(1,points);
    x1[plotFiles+iFun]=new float[points];
    //Passo la linea a myLineCalc():
    myLineCalc.getFileInfo(fileNumsLst, fileNamesLst, varMaxNumsLst);
    ret=myLineCalc.getLine(data.lineInt,selectedFileIdx+1);

    /*  In LineCalc devo avere dei puntatori ai vettori dei valori fra loro adiacenti.
     * Creo pertanto varMatrix che è un puntatore che punta ad un array di puntatori
     * contigui ai dati da utilizzare per il calcolo della funzione.
     * In namesMatrix invece metto i corrispondenti nomi delle variabili.
    */
    float**varMatrix= new float*[data.varNumsLst.count()];
    QList <QString *> namesFullList;
    int size=sizeof(float)*points;
    //Creo i valori di x1, cioè sull'asse x delle variabili funzione, nel caso in cui proprio sull'asse x non vi sia una funzione:
    if(!myVarTable->xInfo.isFunction){
      memcpy(x1[plotFiles+iFun],mySO[funFileIdx[0]]->y[myVarTable->xInfo.idx], size);
      // Qui va messa la gestione del timeshift nel caso di funzione di variabili. Occorrerà comporre la x1 considerando differenti time shifts per i vari files che compongono la funzione. Pertanto il seguente for non è sufficiente in quanto traslerebbe uniformemente la funzione e non selettivamente gli assi x dei vari files presenti nella funzione:
//      for(int iPoint=0; iPoint<points; iPoint++)
//          x1[plotFiles+i][iPoint]+=timeShift[funFileIdx];
    }

    for(int j=0; j<data.varNumsLst.count(); j++){
       varMatrix[j]=mySO[data.varNumsLst[j].fileNum-1]->y[data.varNumsLst[j].varNum-1];
    }
    for(int j=0; j<fileNumsLst.count(); j++){
       namesFullList.append(mySO[fileNumsLst[j]-1]->varNames);
    }
   /*Passo i nomi e il puntatore alla matrice contenente nelle varie righe i vettori
    * delle variabili da utilizzare per il calcolo della funzione
    * I valori di puntatori alle variabili-funzione rimangono allocati nel programma
    * chiamante.
    * lineCalc analizza la linea sostituendo le costanti esplicite con corrispondenti
    * puntatori e i riferimenti ai nomi di variabile con i corrispondenti puntatori
    * passati come secondo parametro
    * Il vettore dei nomi espliciti mySO[]->varNames serve solo per compilare
    * un'informazione completa del significato della stringa di funzione da visualizzare
    * poi all'utente. La linea di funzione corredata di nomi espliciti si chiama
    * "fullLine"
   */

    // La seguente riga manda in esecuzione indirettamente variablesToPointers.
    // Se il plot avviene dopo un cambiamento di file, la stringa della funzione è stata arricchita delle indicazioni dei files, ma la line interna è rimasta senza i nomi di file e quindi variablesToPointers viene eseguita con la stringa sbagliata: ad esempio con v2+v3 invece della corretta f2v2+f2v3.

    // ############
    // NOTA IMPORTANTE
    // Ora che le funzioni di variabile ammettono di mescolare dati da differenti files, occorre cambiare la seguente chiamata a funzione. Invece di passare solo i varNames di un unico mySO, e il solo selectedFileIdx, devo passare un array di puntatori ad arrays di stringhe, ed un vettore di indici di file. Questo è un prerequisito per evitare il crash di f1v2+f2v3 nel TODO.
    // ############

     ret=myLineCalc.getNamesAndMatrix(data.varNames, varMatrix, namesFullList,
                                      selectedFileIdx);
    if(ret!=""){
        QMessageBox::critical(this, "PlotXY",ret);
        return;
    }
    //Per soli scopi di visualizzazione per l'utente finale passo, tramite doppia indirezione per ragioni di efficienza, l'array di array dei nomi espliciti di tutte le variabili presenti nei files caricati:

    //Ora effettuo il calcolo della funzione. Il file è data.fileNums[0], ma con indice a base 1.
    //Se una funzione deve divenire la variabile x, essa la calcolo nel vettore ausiliario funXVar che devo allocare.
    if(myVarTable->xInfo.isFunction){
      delete[] funXVar;
      funXVar =new float[points];
    }
    int myIdx;
    if(myVarTable->xInfo.isFunction)
      myIdx=myVarTable->xInfo.idx;
    else
      myIdx=-1;
    int funXDone=0;  //Anche la var. sull'asse x può essere una funzione. Se ho già trattato la eventuale funzione sull'asse x metto funXDone a true.
    if(myVarTable->xInfo.isFunction && iFun>myVarTable->xInfo.idx)
      funXDone=1;
    for(int k=0; k<points; k++){
      float xxx;
      xxx=myLineCalc.compute(k);
      if(myLineCalc.divisionByZero){
          QString sampleIndex, timeValue, msg;
          if (k==0)
             sampleIndex="first";
          else if (k==1)
              sampleIndex="second";
          else if (k==2)
            sampleIndex="third";
          else
            sampleIndex=sampleIndex.setNum(k+1)+"th";
          timeValue=timeValue.setNum(x1[0][k]);
          msg= "Division by zero in function of variable; plot impossible\n"
               "Offending operation at "+sampleIndex+ " sample.\n"
               "The horizontal variable (possibly time) value is " + timeValue+ "\n";
         QMessageBox::warning(this, "PlotXY",msg);
         qDebug()<<"warning 5";
         myLineCalc.divisionByZero=false;
        return;
      }
      if(iFun==myIdx)
        funXVar[k]=xxx;
      else
        y1[plotFiles+iFun-funXDone][0][k]=xxx;
      ret=myLineCalc.ret;
      if(ret!="")break;
    }

    /* Per fare correttamente l'integrale valgono le seguenti considerazioni:
       * 1) se la variabile x è una funzione per essa l'integrazione non è ammessa
       * 2) L'integrale è inteso come integrale del tempo. Nel caso di multifile l'integrale è fatto rispetto al tempo relativo alla variabile considerata; nel caso di plotXY devo chiarire che il tempo è la variabile di indice 0 del file corrente.
      */
    if(myLineCalc.integralRequest && myVarTable->xInfo.isFunction){
      QString msg="integral of the x variable is not allowed.";
      QMessageBox::warning(this,"PlotXY-dataSelWin",msg);
      qDebug()<<"warning 6";
      return;
    }
    if(myLineCalc.integralRequest)
      y1[plotFiles+iFun-funXDone][0]=integrate(mySO[funFileIdx[0]]->y[0], y1[plotFiles+iFun-funXDone][0], mySO[funFileIdx[0]]->numOfPoints);

//    if(myLineCalc.integralRequest)
//      y1[plotFiles+i-funXDone][0]=integrate(x1[plotFiles+i-funXDone],
//        y1[plotFiles+i-funXDone][0],mySO[funFileIdx]->numOfPoints);
    delete[] varMatrix;
    if(myVarTable->xInfo.isFunction)
      continue;
    SCurveParam param;
    param.isFunction=true;  //Si ricordi che y1Info per funzioni ha un unico elemento per funzione "i"
    param.isMonotonic=false;
    param.name=data.name;
    param.midName=data.line;
    param.fullName=myLineCalc.giveLine("lineFullNames");
    param.color=data.color;
    param.rightScale=data.rightScale;
    param.unitS=myLineCalc.computeUnits();
    if(myLineCalc.integralRequest)
       param.unitS=integrateUnits(param.unitS);
    y1Info[plotFiles+iFun].append(param);
    myFileInfo.name=myLineCalc.giveLine("funText");
    myFileInfo.fileNum=plotFiles+iFun;
    myFileInfo.numOfPoints=points;
    myFileInfo.variableStep=true;
    // Nel caso di funzioni di variabili faccio per default diagrammi di linea e quindi metto falso a frequency scan:
    myFileInfo.frequencyScan=false;
    filesInfo.append(myFileInfo);
  }
  //Ora, nel caso in cui myVarTable.xInfo.isFunction==true devo ricopiare funXVar in x1
  if(myVarTable->xInfo.isFunction){
    for(int i=0; i<1+funInfo.count()-1; i++){
    //Il primo 1 nel target è numOfPlotFiles deve essere 1; il secondo, preceduto da segno '-', è necessario perché dobbiamo diminuire di 1 funInfo.count() in quanto funInfo contiene anche dati della funzione che deve andare sull'asse x
        x1[i]=funXVar;
//      memcpy(x1[i],funXVar, sizeof(float)*mySO[0]->numOfPoints);
    }
  }
/***********  inizio Software per debug **************
    float x_dbg0[3], y_dbg0[3][3];
    int
        var_dbg=min(3,myVarTable->yInfo[lastIFile].count()),
        pt_dbg=min(3,mySO[lastIFile]->numOfPoints);
    for(int iPt=0; iPt<pt_dbg; iPt++)
       for(iVar=0; iVar<var_dbg; iVar++){
           x_dbg0[iPt]=x1[iFileNew][iPt];
           y_dbg0[iVar][iPt]=y1[iFileNew][iVar][iPt];
       }
***********  fine Software per debug ****************/


    /*Trasmetto le informazioni alla finestra di plot, che farà una copia locale delle intere matrici x1 e y1.
    */

    //Si ricordi che lo show comporta un resize della finestra e quindi, attraverso la ui, anche del lineChart gestito dal designer all'interno della finestra. Pertanto il plot() deve seguire lo show().
  myPlotWin->getData(x1, y1, myVarTable->xInfo, y1Info, filesInfo);

  /* Le matrici y1[i] e i vettori x[i], con i a partire da plotFiles, erano stati allocati più sopra e quindi qui disalloco, ora che myPlotWin ne ha fatto copia locale:*/
  for(int i=0; i<funInfo.count(); i++){
    DeleteFMatrix(y1[plotFiles+i]);
    delete[] x1[plotFiles+i];
  }

  //Il seguente close serve per  fare un reset del bottone "data". Altrimenti può accadere che se il data cursor è visibile, al nuovo plot i numeri sono visualizzati nell'opzione vecchia: ad esempio se ho ridotto il numero di variabili da n a 1 rimane la finestra esterna e viceversa.
  myPlotWin->close();
  myPlotWin->show();
  myPlotWin->plot(updatingPlot);

  ui->updateTBtn->setEnabled(true);
  ui->updateTBtn->setChecked(false);
  updatingPlot=false;
  ui->eqTBtn->setEnabled(true);
  ui->arrTBtn->setEnabled(true);
  delete[] x1;
  for(iFile=0; iFile<myVarTable->numOfPlotFiles; iFile++)
    delete[] y1[iFile];
  delete[] y1;
}


void CDataSelWin::on_multifTBtn_clicked(bool checked){
  // Il valore di checked è quello già dopo l'effetto del click. Se ad esempio ero in multifile e clicco, qui troverò checked=false.
  if(!checked){
    //Vado in singleFile
    GV.multiFileMode=false;
    goneToSingleFile=true;
    // Il tooltip del nome deve essere quello del file selezionato, non quello della riga dove c'era, in multifile, la x. Faccio quindi lo swap dei tooltip:
    if(selectedFileRow>-1){
      QString strDummy=ui->fileTable->item(1,2)->toolTip();
      ui->fileTable->item(1,2)->setToolTip(ui->fileTable->item(selectedFileRow,2)->toolTip());
      ui->fileTable->item(selectedFileRow,2)->setToolTip(strDummy);
    }
    ui->fileTable->setColumnHidden(0,true);
    ui->fileTable->setColumnHidden(1,true);
    /*  NOTA IMPORTANTE
     * Dall'apparire di Windows 10 la riga di intestazione delle tabelle è divenuta
     * troppo chiara e non sono riuscito a trovare un sistema per renderla grigia.
     * Ho pertanto deciso di usare la prima riga utile di tabella come riga di
     * intestazione.
     * I dati dei files sono quindi a partire dalla riga 1. In più, il passaggio in
     * multifile l'ho fatto riducendo l'altezza della tabella e facendo lo swap del testo.
     * Non era forse meglio usare un comando "setRowHidden"?
    */

    int tableHeight=ui->fileTable->rowHeight(0)+ui->fileTable->rowHeight(1)+2;
    ui->fileTable->setMinimumHeight(tableHeight);
    ui->fileTable->setMaximumHeight(tableHeight);
    ui->multifTBtn->setToolTip(tr("Go to multi-file mode"));
    /* devo salvare il contenuto della riga di indice 1 e copiare il contenuto della riga del file selezionato in riga di indice 1:*/
    if(selectedFileIdx>-1 && selectedFileRow!=1)
      for(int icol=0; icol<ui->fileTable->columnCount(); icol++){
        saveStrings[icol]= ui->fileTable->item(1,icol)->text();
        ui->fileTable->item(1,icol)->setText(ui->fileTable->item(selectedFileRow,icol)->text());
      }
    // rendo hide tutte le righe eccetto le prime due
    for(int i=2; i<ui->fileTable->rowCount(); i++){
       ui->fileTable->hideRow(i);
    }
  }else{
    //Vado in multiFile
    GV.multiFileMode=true;
    if(goneToSingleFile&&selectedFileRow>-1){
      // Il tooltip del nome deve essere quello del file selezionato, non quello della riga dove c'era, in multifile, la x. Può esserci stato infatti un aggiornamento in singlefile del file caricato. Faccio quindi lo swap dei tooltip:
      QString strDummy=ui->fileTable->item(selectedFileRow,2)->toolTip();
      ui->fileTable->item(selectedFileRow,2)->setToolTip(ui->fileTable->item(1,2)->toolTip());
      ui->fileTable->item(1,2)->setToolTip(strDummy);
      //Se in singlefile è stato aggiornato un file, il suo numero è stato correttamente messo in selectedfileIdx, ma non nella tabella. Lo metto ora:
      strDummy.setNum(selectedFileIdx+1);
      ui->fileTable->item(selectedFileRow,0)->setText(strDummy);
    }
    ui->fileTable->setColumnHidden(0,false);
    ui->fileTable->setColumnHidden(1,false);
#ifndef Q_OS_MAC
    ui->fileTable->resizeColumnsToContents();
    ui->fileTable->setColumnWidth(0,0.8*ui->fileTable->columnWidth(0));
    ui->fileTable->setColumnWidth(1,0.8*ui->fileTable->columnWidth(1));
#endif
    if(numOfLoadedFiles==1){
      ui->fileTable->item(1,0)->setText("x");
      ui->fileTable->item(1,1)->setText("1");
    }

    // Visualizzo tutte le righe dotate di file, con un minimo di 3 più l'intestazione:
    visibleFileRows=max(3,numOfLoadedFiles);
    int tableHeight=(visibleFileRows+1)*ui->fileTable->rowHeight(0)+2;
    ui->fileTable->setMaximumHeight(tableHeight);
    ui->fileTable->setMinimumHeight(tableHeight);
    ui->multifTBtn->setToolTip(tr("Go to single-file mode"));
    // Rimetto nella riga di indice selectedFileRow (l'inidice 0 indica la riga di intestazione della tabella) il contenuto salvato precedentemente (o stringhe vuote se non era stato salvato nulla):
    if(goneToSingleFile && selectedFileRow!=1){
      for(int icol=0; icol<ui->fileTable->columnCount(); icol++){
        if(selectedFileIdx<0)
            break;
        ui->fileTable->item(selectedFileRow,icol)->setText(ui->fileTable->item(1,icol)->text());
        ui->fileTable->item(1,icol)->setText(saveStrings[icol]);
      }
    }
    // tolgo hide a tutte le righe che contengono files, con un minimo di 3
    ui->fileTable->showRow(2);
    ui->fileTable->showRow(3);
    for(int i=4; i<ui->fileTable->rowCount(); i++){
      if(ui->fileTable->item(i,1)->text()!="")
        ui->fileTable->showRow(i);
      else
        break;
    }
  }
  //Quando commuto fra multifile e singlefile ridimensiono la tabella file fra l'altro nascondendo o mostrando colonne. Quindi devo attivare il ridimensionamento personalizzato con il seguente resizeEvent:
  QResizeEvent * ev=0;
  resizeEvent(ev);

  myVarTable->setMultiFile(GV.multiFileMode);
  if(GV.multiFileMode && fileLoaded){
   //Il commonX non lo devo solo passare alla tabella corrente ma a tutte. Infatti se  parto da single file,  carico un file, e passo in multifile, resterebbe nelle altre tabellle "1! nelle colonna "f" invece di "a".
  QString str=computeCommonX();
  ui->varTable1->setMultiFile(true);
  ui->varTable2->setMultiFile(true);
  ui->varTable3->setMultiFile(true);
  ui->varTable4->setMultiFile(true);
  ui->varTable1->setCommonX(str);
  ui->varTable2->setCommonX(str);
  ui->varTable3->setCommonX(str);
  ui->varTable4->setCommonX(str);
  }else{
    /* qui occorre valutare quando cliccare sulla variabile tempo per selezionarla
     * Non basta verificare che il menù delle variabili non sia vuoto. Occorre
     * anche che il tempo non sia già stato selezionato. Se ad es. è stato
     * fatto un loadFile il tempo è già seleizionato Se invece arrivo qui perché
     * sto commutando da multifile esso non  selezionato. Maglio quindi chiedere
     * direttamene a myVarTable.
    */
    if(ui->varMenuTable->rowCount()>0 && myVarTable->isEmpty())
      varMenuTable_cellClicked(0,1,false);
  }
  ui->plotTBtn->setEnabled(false);
}

void CDataSelWin::on_fileTable_clicked(const QModelIndex &index)
{
    /* Nel caso di click semplice su riga non di intestazione si seleziona il file di cui visualizzare le variabili.
      Se il click semplice è sull'ultima colonna si fa il toggle fra Tmax e Tshift
Occorre inoltre considerare l'eventualità che il click semplice sia semplicemente il primo click di un doppio click.
    */

  int i=index.row(),j=index.column();

  //se il click è fatto nella colonna Tmax o Tshift, non in prima riga esco: altrimenti farei la commutazione o la deselezione del file corrente
  if(i>0 && j>4)
        return;
  if(doubleClicking){
    doubleClicking=false;
    return;
  }

  if(i==0){  //ora gestisco la commutazione Tshift-Tmax
     if (j==5){  //passaggio da Tmax a Tshift
        ui->fileTable->hideColumn(5);
        ui->fileTable->showColumn(6);
        ui->fileTable->resizeColumnsToContents();
     }else if (j==6){
        ui->fileTable->hideColumn(6);
        ui->fileTable->showColumn(5);
        ui->fileTable->resizeColumnsToContents();
      }  else
        return; //se click su prima riga ma non ultima colonna esco
  }
  if(i==0){
    resizeEvent(0);
    return;
  }
  selectFile(index.row());
  resizeEvent(0);
  //    ui->varMenuTable->resizeColumnsToContents();
}

void CDataSelWin::on_fileTable_doubleClicked(const QModelIndex &index){
    /*  Quando c'è un double click il file viene rimosso
     *
   */

    int i=index.row(),j=index.column();

    //se il doppio click è fatto nella colonna Tmax o Tshift, non in prima riga esco: altrimenti farei la deselezione del file corrente
    if(i>0 && j>4)
          return;
    //Alla prima riga il double-click non deve vare nulla:
    if(i==0)
        return;
  //devo comandare la rimozione solo se nella riga dove ho cliccato esiste realmente un file. Lo verifico attraverso la presenza di un qualsiasi contenuto nella FILENUMCOL
  if (ui->fileTable->item(i,FILENUMCOL)->text()!=""){
    removeFile(i);
    myVarTable->myReset();
    if(numOfLoadedFiles>0)
      myVarTable->setCommonX(computeCommonX());
    else
      ui->refrTBtn->setEnabled(false);
  }

}


void CDataSelWin::giveFileInfo(QList <int> &fileNums, QList <QString> &fileNames, QList <int> &varNums) {
    /* Questa funzione è una callback da CVarTable che serve per vedere quali numeri di files sono utilizzabili nelle funzioni di variabili.*/
    fileNums=fileNumsLst;
    fileNames=fileNamesLst;
    varNums=varMaxNumsLst;
}

void CDataSelWin::removeFile(int row_) {
  /* Funzione per "scaricare" dal programma un file.
   * Faccio lo scaricamento solo se siamo in multifile.
   * In realtà i nomi e i valori delle variabili non vengono cancellati nel MySO
   * corrispondente al file da scaricare, né le relative variabili vengono deallocate
   * Questo perché MySO all'atto del caricamento come primo passo disalloca la memoria
   * eventualmente allocata in precedenza ai vettori dei nomi e dei valori delle
   * variabili.
   * La memoria viene quindi liberata solo un attimo prima del riallocamento per
   * consentire il caricamento del nuovo file.
  */
  int row, col, fileIndex;
  if(!GV.multiFileMode)return;
  //Il file da rimuovere è sempre quello corrente, in quanto il doppio click è sempre interpretato anche come singolo click che ha selezionato il file.
  //Rendo corrente il primo file della tabella:
  for (row=1; row<=MAXFILES; row++)
    if(ui->fileTable->item(row_,1)->text()!="") break;
  selectFile(row);

  //rendo "free" l'indice del file che sto per far scomparire dalla tabella:
  fileIndex=ui->fileTable->item(row_,1)->text().toInt()-1;
  freeFileIndex<<fileIndex;
  GV.varNumsLst[fileIndex]=0;
  ui->varTable1->getVarNumVect(GV.varNumsLst);
  ui->varTable2->getVarNumVect(GV.varNumsLst);
  ui->varTable3->getVarNumVect(GV.varNumsLst);
  ui->varTable4->getVarNumVect(GV.varNumsLst);
  numOfLoadedFiles--;
  setAcceptDrops(true);
  ui->loadTBtn->setEnabled(true);
  if(numOfLoadedFiles==0){
    ui->saveStateTBtn->setEnabled(false);
    ui->varMenuTable->clear();
    ui->varMenuTable->setRowCount(0);
  }
  //Adesso a partire dalla riga su cui si è fatto click sposto più in alto tutti gli items, e i relativi tooltip
  if(row_<MAXFILES )
    for(row=row_; row<MAXFILES; row++){
      for(col=0; col<ui->fileTable->columnCount(); col++)
        ui->fileTable->item(row,col)->setText(ui->fileTable->item(row+1,col)->text());
      ui->fileTable->item(row,2)->setToolTip(ui->fileTable->item(row+1,2)->toolTip());
   }

  //ora devo "sbiancare" l'ultima riga. Non è visualizzata ma è bene cha abbia contenuto vuoto:
  for(col=0; col<ui->fileTable->columnCount(); col++)
     ui->fileTable->item(numOfLoadedFiles+1,col)->setText("");
  if(numOfLoadedFiles>2)
     ui->fileTable->hideRow(numOfLoadedFiles+1);

  // Se il numero di file visualizzati era superiore a 3 rendo invisibile l'ultima riga:
  if(visibleFileRows>3){
    visibleFileRows--;
    int newVSize=(visibleFileRows+1)*ui->fileTable->rowHeight(0)+2;
    ui->fileTable->setMaximumHeight(newVSize);
    ui->fileTable->setMinimumHeight(newVSize);
  }

  if(freeFileIndex.count()==MAXFILES)
    fileLoaded=false;

  //se il file che ho scaricato è l'ultimo devo fare una cancellazione completa del contenuto delle 4 tabelle
  if(numOfLoadedFiles==0){
    ui->varTable1->myReset(true);
    ui->varTable2->myReset(true);
    ui->varTable3->myReset(true);
    ui->varTable4->myReset(true);
    selectedFileIdx=-1;
//    fileNumsLst.clear();
//    varMaxNumsLst.clear();
//    fileNamesLst.clear();
  }else{
    //devo deselezionare eventuali variabili selezionate relative al file che ho eliminato, in tutte le pagine.
    ui->varTable1->unselectFileVars(fileIndex);
    ui->varTable2->unselectFileVars(fileIndex);
    ui->varTable3->unselectFileVars(fileIndex);
    ui->varTable4->unselectFileVars(fileIndex);
//    int i=fileNumsLst.indexOf(fileIndex+1);
//    fileNumsLst.removeAt(i);
//    fileNamesLst.removeAt(i);
//    varMaxNumsLst.removeAt(i);
    fileNumsLst[fileIndex]=0;
    fileNamesLst[fileIndex]=" ";
    varMaxNumsLst[fileIndex]=0;
  }

  //Se la tabella variabili corrente è vuota devo disattivare tutti i bottoni:
  if(myVarTable->numOfTotVars<2){
    ui->plotTBtn->setEnabled(false);
    ui->resetTBtn->setEnabled(false);
    ui->updateTBtn->setEnabled(false);
    ui->saveVarsBtn->setEnabled(false);
    ui->fourTBtn->setEnabled(false);
  }
}

void CDataSelWin::fillVarMenuTable(int fileIndex){
 /* Questa funzione privata fa operazioni connesse con il riempimento della VarMenuTable
  * Al momento della sua nascita (nov 2016) viene richiamata sia da loadFile che da
  * selectFile().
 */
    ui->varMenuTable->clearContents();
    sortType=noSort;
    ui->sortTBtn->setText("A <-> Z");

    /* ricordo che (Norobro sul Qt forum, sept. 2012):
  QTableWidget::clearContents() calls delete (link) on each QTableWidgetItem. Deletes being called on objects not allocated with new (stack allocated objects) which will yield unpredictable results. You need to allocate your QTableWidgetItem(s) on the heap.
  Quindi non si possono passare alle celle ad esempio elementi di un vettore
      QTableWidgetItem varItems[100];
  Si ricorda che:
  - lo stack è allocato con tecnica LIFO, mentre l'allocazione nell'heap può avvenire in qualsiasi area e per questo richiede più risorse di calcolo.
  - heap contiene gli oggetti allocati con l'operatore new
  - stack contiene tutte le altre variabili, quindi quelle statiche che vengono costruite prima del run-time, e le variabili automatiche (per le quali l'accesso LIFO è particolarmente conveniente).
  */
    ui->varMenuTable->setRowCount(mySO[fileIndex]->numOfVariables);
    for(int i=0; i<mySO[fileIndex]->numOfVariables; i++){
      int j=i+1;
      QString str;
      str.setNum(j);
     //Se il numero massimo supera 9 premetto un digit 0:
      if(mySO[fileIndex]->numOfVariables>9 &&j<10)
      str="0"+str;
      //Se il numero massimo supera 99 premetto un ulteriore digit 0:
       if(mySO[fileIndex]->numOfVariables>99 &&j<100)
       str="0"+str;
       //Se il numero massimo supera 999 premetto un ulteriore digit 0:
       if(mySO[fileIndex]->numOfVariables>999 &&j<1000)
       str="0"+str;
       QTableWidgetItem *item0 = new QTableWidgetItem(str);
       ui->varMenuTable->mySetItem(i,0,item0,true);
       item0->setFont(myFont);
       item0->setToolTip(str);
   /* In item 1 metto il numero della variabile. Il numero di digit con cui è scritto deve essere pari al numero di digit del più alto numero di variabile, per consentire un successivo sort efficace.*/
       item0->setBackgroundColor(neCellBkColor);

       QTableWidgetItem *item = new QTableWidgetItem(mySO[fileIndex]->varNames[i]);
       ui->varMenuTable->mySetItem(i,1,item,mySO[fileIndex]->timeVarIndex==i);
       item->setFont(myFont);
       item->setToolTip(mySO[fileIndex]->varNames[i]);
       item->setBackgroundColor(neCellBkColor);
       QFontMetrics fontMetrics(myFont);
       int myHeight=FACTORROWS*fontMetrics.height();
       ui->varMenuTable->setRowHeight(i,myHeight);
    }

    selectedFileIdx=fileIndex;
    ui->varMenuTable->setCurrFile(fileIndex);
    myVarTable->setCurrFile(fileIndex);
}


void CDataSelWin::on_fourTBtn_clicked() {
    struct SFourData fourData;
    qDebug()<<"Fourier button has been clicked";
    /* La seguente riga serve per evitare che il plot della finestra Plot
     * faccia riferimento, al momento della creazione della finestra Four,
     * a dati diversi da quelli di quest'ultima: si ricordi che entrambe le
     * finestre non mantengono una copia privata dei dati ma puntano a vettori
     *  esterni.
     * Con questa tecnica anche se in un secondo momento si rifà un plot
     * con dati diversi da quelli con cui si è costruita la finestra Fourier
     * non ci sono problemi.
  */
    on_plotTBtn_clicked();
    /* Con la riga precedente si sono passati i dati a myPlotWin, anche facendo
     *  calcoli nel casi di funzioni di variabili. Pertanto la cosa più
     * semplice per effettuare Fourier è di passare a myFourWin puntatori
     * dati asse x e y presi da myPlotWin. Con l'occasione faccio transitare
     * da myPloWin a myFourWin anche dati accessori quali il nome del file,
     * della variabile, ecc.
   */
    fourData=myPlotWin->giveFourData();
    myFourWin->getData(fourData);
    if(fourData.ret!=""){
      QMessageBox::information(this,"CDataSelWin", fourData.ret,"");
      qDebug()<<"Sent fourier Information";
    }
    myFourWin->close();
    myFourWin->show();
    fourTableIndex=ui->tabWidget->currentIndex();

 }

void CDataSelWin::on_optBtn_clicked()
{
    myProgOptions->getData(GV.PO);
    QScreen *screen=QGuiApplication::primaryScreen();
   if(!screen->geometry().contains(QPoint(0,0)))
      myProgOptions->move(toInPrimaryScreen(QPoint(0,0),10));

   bool oldColorScheme=GV.PO.useOldColors;
   myProgOptions->exec();

  if(GV.PO.useOldColors!=oldColorScheme){
    ui->varTable1->getColorScheme(GV.PO.useOldColors);
    ui->varTable2->getColorScheme(GV.PO.useOldColors);
    ui->varTable3->getColorScheme(GV.PO.useOldColors);
    ui->varTable4->getColorScheme(GV.PO.useOldColors);
    on_resetTBtn_clicked();
  }

    //Qui non è necessario fare un giveData() perché all'interno di myProgOptions le opzioni sono salvate in GV.PO
}


CDataSelWin::~CDataSelWin()
{
    delete ui;
}

void CDataSelWin::on_loadTBtn_clicked()
{
  QFileDialog dialog(this);
  dialog.setFileMode(QFileDialog::ExistingFiles);

  static QStringList filters;
  static bool filterSet=false;
  QString selectedNameFilter;
  if(!filterSet){
#ifdef EXCLUDEATPCODE
    filters
       << "All compatible files (*adf *.lvm *.cfg *.mat)"
       << "ADF and CSV (*.adf *.csv)"
       << "LAB-VIEW (*.lvm)"
       << "COMTRADE (*.cfg)"
       << "MATLAB(*.mat)";
#else
   filters
      << "All compatible files (*adf *.lvm *.pl4 *.cfg *.mat)"
      << "ADF and CSV (*.adf *.csv)"
      << "ATP (*.pl4)"
      << "LAB-VIEW (*.lvm)"
      << "COMTRADE (*.cfg)"
      << "MATLAB(*.mat)";
#endif
  }
  if(filterSet){
   dialog.setNameFilters(filters);
  } else{
     // In MAC al momento (Qt 5.2 Mar 2014) i filtri multipli non funzionano e quindi non li abilito:
    dialog.setNameFilters(filters);
  }

  #if defined(Q_OS_MAC)
     dialog.setNameFilter("ADF, ATP, CSV, CFG, LVM, MAT (*adf *.pl4 *.csv *.cfg *.lvm *.mat)");
  #endif

  dialog.setViewMode(QFileDialog::Detail);
  QStringList fileNameLst;
  if (dialog.exec())
    fileNameLst = dialog.selectedFiles();
  QString ret;

  /* Voglio che alle prossime riaperture i filtri rimangano quelli individuati la prima volta.
   * Se ad esempio sto lavorando su CSV, molto probabilmente vorrò riaprire csv nuovamente.
   *
   * Ma questo lo faccio solo per PC, perché per MAC i filtri multipli non funzionano
   *
   * */
#if defined(Q_OS_MAC)
;
#else
  selectedNameFilter=dialog.selectedNameFilter();
  if(selectedNameFilter!=filters[0]){
    //allora l'utente ha scelto un filtro diverso: faccio lo swap fra il primo e quello scelto.
    int i=filters.lastIndexOf(selectedNameFilter);
    if (i<0){
       QMessageBox::warning(this, "MC's PlotXY", "Unexpected error in CDataSelWin"
                            "\nno harm is done, work can proceed");
       return;
    }
    filters[i]=filters[0];
    filters[0]=selectedNameFilter;
  }
#endif
  filterSet=true;
  qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
  ret=loadFileList(fileNameLst);
  qApp->restoreOverrideCursor();

//  QResizeEvent *e=NULL;
  resizeEvent(NULL);
  ui->varMenuTable->resizeColumnsToContents();
  // La stringa ret non va trattata in un messageBox perché è già stato fatto all'interno di loadFile().

}

void CDataSelWin::on_refrTBtn_clicked()
{
  //Per trovare il nome del file completo del percorso parto dal tooltip, che lo contiene.
  //Il tooltip è fatto così:
  //  fileTooltip="<p><B>Full name:</B> "+ fullName+"</p>";
  // cui è aggiunta poi altra roba.
  //Pertanto prima taglio i primi 21 caratteri,poi taglio quello che c'è a partire dalla prima parentesi angolare aperta
  QString name=ui->fileTable->item(selectedFileRow,2)->toolTip();
  name.remove(0,21);
  name.chop(name.length()-name.indexOf('<',0));

  qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
  enum ESortType oldSortType=sortType;

  loadFile(selectedFileIdx,name,true,refreshUpdate);
  if(oldSortType==ascending)
    on_sortTBtn_clicked(); //modifica il sortType e vado in ascending!
  else if(oldSortType==descending){
    on_sortTBtn_clicked(); //modifica il sortType e vado in ascending
    on_sortTBtn_clicked(); //modifica il sortType e vado in descending
  }
  qApp->restoreOverrideCursor();
  updatingPlot=refreshUpdate;
  int currentShIndex=ui->tabWidget->currentIndex();
/*
    Qui devo:
 1) selezionare in sequenza i vari sheet (come se facessi un click non direttamente da  ui->tabWidget->setCurrentIndex(0); Questo è importante perché se uno sheet non ha variabili in questo modo il pulsante plot non è attivo
 2) se il pulsante plot è attivo lo clicco
 3) ripristino currentShIndex

*/
    for(int iTab=0; iTab<4; iTab++){
      on_tabWidget_currentChanged(iTab);
      if(ui->plotTBtn->isEnabled()) on_plotTBtn_clicked();
    }
    on_tabWidget_currentChanged(currentShIndex);
    ui->tabWidget->setCurrentIndex(currentShIndex);

    updatingPlot=false;
}


void CDataSelWin::on_refrUpdTBtn_clicked(bool checked)
{
    refreshUpdate=checked;
}


void CDataSelWin::on_saveVarsBtn_clicked()
{
  /*Questa funzione serve per salvare alcune variabili su file, provenienti da un unico file di input.
Il suo uso tipico è di estrarre poche variabili da un file che ne può contenere molte.
In futuro forse verrà estesa in modo da consentire salvataggi di variabili provenienti da più files, ma comunque dovranno avere lo stesso numero di punti
*/
  int i,
      myFileIdx=-1, //indice del file da cui salvara variabili
      *stoVarIdx, //vettore degli indici di variabili da prelevare dal file di indice myFileIdx e salvare
      nStoVars,  //numero di variabili da salvare
      nBkpVars;  //numero di variabili nel file da cui salvare a disco (e di cui backup-are i nomi)
  SCurveParam info;
  QString msg, fullName, ext,
          *bkpVarNames;  //Vettore temporaneo di nomi in cui copiare i nomi del file da cui salvare su disco
  QString comment;

  myVarTable->analyse();
  //Se ci sono funzioni emetto un messaggio ed esco:
  if(myVarTable->giveFunInfo().count()>0){
      qDebug()<<"warning 7";
      QMessageBox::warning(this, "PlotXY",
        "you can save only file vars:\nfunctions not allowed when saving");
      return;
  }
  //Se ci sono variabili provenienti da più files emetto un messaggio ed esco:
  for(i=0; i<MAXFILES; i++){
    if(myFileIdx>-1 && myVarTable->yInfo[i].count()>0){
      qDebug()<<"warning 8";
      QMessageBox::warning(this, "PlotXY",
          "you can save only vars coming from the same files:"
          "\nplease delete some vars and leave only vars from the same file"
          "\nbefore saving again");
      return;
    }
    if(myVarTable->yInfo[i].count()>0)myFileIdx=i;
  }
  //Il seguente "+1" è dovuto al fatto che oltre alla variabile selezionata salviamo sempre anche il tempo
  nStoVars=myVarTable->yInfo[myFileIdx].count()+1;
  nBkpVars=mySO[myFileIdx]->numOfVariables;
  stoVarIdx=new int [nStoVars];
  bkpVarNames=new QString[nBkpVars];

  // riempo gli stoVarIdx con gli indici delle variabili da salvare (la prima è il tempo):
  stoVarIdx[0]=0;
  for(i=0; i<nStoVars-1; i++)
    stoVarIdx[i+1]=myVarTable->yInfo[myFileIdx][i].idx;

 //A questo punto ho solo variabili dal file di indice myFileIdx, e posso fare il salvataggio.

  fullName="";
  if(mySO[myFileIdx]->allowsPl4Out)
    fullName = QFileDialog::getSaveFileName(this, "Save File",QDir::currentPath(),
      "ADF  (*.adf);;COMTRADE  (*.cfg);;MATLAB  (*.mat);;PL4  (*.pl4)");
  else
    fullName = QFileDialog::getSaveFileName(this, "Save File",QDir::currentPath(),
      "ADF  (*.adf);;COMTRADE  (*.cfg);;MATLAB  (*.mat)");
  if(fullName=="")
    return;

  /* La routine saveToAdfFile, originariamente pensata per il programma Converter.exe, che dopo la conversione scartava il programma di input, modifica tutti i nomi delle variabili per renderli compatibili con il formato di output. Pertanto occorre provvedere a salvare i nomi originari e poi ripristinarli.
  */
  for(i=0; i<nBkpVars; i++)
    bkpVarNames[i]=mySO[myFileIdx]->varNames[i];
  // ora devo verificare che l'estensione sia fra quelle consentite:
  ext=fullName.right(fullName.length()-fullName.lastIndexOf('.')-1);
  ext=ext.toLower();

  mySO[myFileIdx]->commasAreSeparators=GV.PO.commasAreSeparators;

  if(ext=="adf"){
    comment="ADF file created by \"";
    comment+="MC's PlotXY";
    comment+="\" program";
    msg=mySO[selectedFileIdx]->saveToAdfFile(fullName, comment, nStoVars, stoVarIdx);
  }
  else if(ext=="cfg"){
      comment="CFG file created by \"";
      comment+="MC's PlotXY";
      comment+="\" program";
    msg=mySO[selectedFileIdx]->saveToComtradeFile(fullName, comment, nStoVars, stoVarIdx);
  }
  else if(ext=="mat")
    msg=mySO[selectedFileIdx]->saveToMatFile(fullName, nStoVars, stoVarIdx);
  else if(ext=="pl4"){
    if(mySO[selectedFileIdx]->allowsPl4Out)
        msg=mySO[selectedFileIdx]->saveToPl4File(fullName, nStoVars, stoVarIdx);
     else{
        msg="Output type PL4 allowed only for variables selected from a PL4 file\n"
            "containing a time-run (not frequency Scan nor HFS)\n"
            "Please, select ADF or MAT output format.";
     }
  }else{
     msg="File "+fullName+" not saved.\nReason: extension \"" +
            ext+"\" is invalid.";
  }
  for(i=0; i<nStoVars; i++)
    mySO[selectedFileIdx]->varNames[i]=bkpVarNames[i];
  if(msg==""){
    QString shortName=fullName;
    shortName.chop(3);
     if(ext=="cfg")
        msg="Selected variables successfully saved into files:\n"
           +fullName+ "\n and\n" +shortName+"dat";
     else
      msg="Selected variables successfully saved into file:\n"+fullName;
    QMessageBox::information(this, "MC's PlotXY",msg);
  }else{
    QMessageBox::critical(this, "MC's PlotXY",msg);
  }
  delete [] stoVarIdx;
  delete [] bkpVarNames;
}


void CDataSelWin::on_eqTBtn_clicked()
{
    /* Questa funzione serve per equalizzare le dimensioni delle finestre di plot, o a un numero prefissato, o alle dimensioni della finestra 1. In quest'ultimo caso l'utente vede visivamente, agendo sulla finestra 1, come verranno le dimensioni delle varie finestre.
     *Le finestre mantengono l'angolo quperiore sinistro originale. Per risistemarle nello spazio a disposizione occorre cliccare sul bottone "arrange"
*/
    int w,h;
    QScreen *screen=QGuiApplication::primaryScreen();
    QRect avGeom=screen->availableGeometry();
    QMargins m(10,10,10,10);
    avGeom=avGeom.marginsRemoved(QMargins(10,10,10,10));

    if(ui->allToBtn->isChecked()){
        w=ui->winWEdit->text().toInt();
        h=ui->winHEdit->text().toInt();
        //Come unica verifica sui dati evito che larghezza e altezza superino le dimensioni dello schermo. Presumo infatti che le finestre siano in partenza con la sbarra superiore all'interno della parte utile dello schermo e pertanto se per casu una parte della finestra sforasse fuori schermo l'utente potrebbbe agevolmente risistemarle.
        w=min(w, avGeom.right()-avGeom.left());
        h=min(h, avGeom.bottom()-avGeom.top());
        plotWin1->resize(w,h);
        plotWin2->resize(w,h);
        plotWin3->resize(w,h);
        plotWin4->resize(w,h);
    } else if(ui->toWin1Btn->isChecked()){
        w=plotWin1->width();
        h=plotWin1->height();
        plotWin2->resize(w,h);
        plotWin3->resize(w,h);
        plotWin4->resize(w,h);
        QString s;
        ui->winWEdit->setText(s.setNum(w));
        ui->winHEdit->setText(s.setNum(h));
      }

}

void CDataSelWin::on_arrTBtn_clicked()
{
/* Questa funzione serve per riposizionare le finestre Win in maniera "armoniosa".
 * Per poter essere attivo questo bottone le finestre devono essere già state equalizzate
 * con il click di eqBtn.
 * Metto le finestre a matrice a destra di CDataSelWin, secondo l'ordine naturale
 * della scrittura occidentale, allineando al margine alto di dataSelWin
 * Prima di fare il lavoro verifico che questo sia effettivamente fattibile con lo
 * spazio a disposizione
*/


    bool sizeIsEnough=true;

/*
    QScreen *screen=QGuiApplication::primaryScreen();
    QRect avGeom=screen->availableGeometry();
    avGeom=avGeom.marginsRemoved(QMargins(10,10,10,10));
*/
    QRect win1Rect, win2Rect;
    if(plotWin1->isVisible())
      win1Rect=plotWin1->frameGeometry();
    if(plotWin2->isVisible())
      win2Rect=plotWin2->frameGeometry();
    QRect thisFrameGeom=this->frameGeometry();
    int screenCount=QGuiApplication::screens().count();
    //Lo spazio complessivamente a disposizione è il right() dell'availableGeometry
    // dell'ultimo schermo.
    QScreen *lastScreen=QGuiApplication::screens()[screenCount-1];
    int totAvailableWidth=lastScreen->availableGeometry().right();  //Width totale dell'insieme di tutti gli schermi!!
    QScreen * myScreen=0;
    for(int i=0; i<screenCount; i++){
      myScreen=QGuiApplication::screens()[i];
      int rightPix=myScreen->availableGeometry().right();
      if(this->x()<rightPix){
        break;
      }
    }
    int availableHeight=myScreen->availableGeometry().height();

    //Solo nel caso di unica finestra Plot lo spazio a disposizione deve essere quello di una sola finestra Plot:
    if(!plotWin2->isVisible() && GV.PO.rememberWinPosSize){
      if(totAvailableWidth-thisFrameGeom.width()-thisFrameGeom.x()< win1Rect.width())
        sizeIsEnough=false;
      if(availableHeight-thisFrameGeom.y() <win1Rect.height()) //devo avere altezza per la finestra plot1
        sizeIsEnough=false;
      if(!sizeIsEnough){
        qDebug()<<"warning 9";
        QMessageBox::warning(this,"",
         "Not enough room on screen to arrange plot window 1.\n"
          "Reduce its size or move it up and/or left\n"
          "the Data Selection Window" );
         return;
      }
    }

    if(plotWin2->isVisible()&& GV.PO.rememberWinPosSize){
      if(totAvailableWidth-thisFrameGeom.width()-thisFrameGeom.x()  <
                                                  win1Rect.width()+win2Rect.width())
        sizeIsEnough=false;
      if(availableHeight-thisFrameGeom.y() <win1Rect.height()+win2Rect.width())
        sizeIsEnough=false;
      if(!sizeIsEnough){
        if(screenCount==1)
          QMessageBox::warning(this,"",
            "Not enough room to arrange all plot windows.\n"
            "Reduce plot windows' size or move it left" );
         else //screenCount>1
           QMessageBox::warning(this,"",
             "Not enough room on screen to arrange all plot windows.\n"
             "Reduce plot windows' size or\n"
             "move Data Selection Window in the secondary screen or\n"
             "move it at left of used screen" );
         return;
      }
    }
    QRect r=thisFrameGeom;
    r.moveLeft(r.x()+r.width()+1);
    plotWin1->move(r.topLeft());

    r=plotWin1->frameGeometry();
    r.moveLeft(r.x()+win1Rect.width()+1);
    plotWin2->move(r.topLeft());

    r=plotWin1->frameGeometry();
    r.moveTop(r.y()+win1Rect.height()+1);
    plotWin3->move(r.topLeft());

    r=plotWin2->frameGeometry();
    r.moveTop(r.y()+win1Rect.height()+1);
    plotWin4->move(r.topLeft());

    //La posizione default di MyFourWin la scelgo in modo che sia ben combinata nel solo caso che sia presente un'unica finestra plot: la metto sotto di essa (stessa posizione di plot3).
    r=plotWin1->frameGeometry();
    r.moveTop(r.y()+win1Rect.height()+1);
    myFourWin->move(r.topLeft());
}

void CDataSelWin::on_saveStateTBtn_clicked()
{
    /* La pressione del tasto saveStateTBtn causa il salvataggio dello "stato" del programma.

    Fasi del salvataggio:
    1) salvataggio della condizione multifile
    2) salvataggio del nome, corredato di informazione, di data e orario, dei files correntemente caricati in memoria
    3) salvataggio del contenuto delle tabelle varTable#
    4) salvataggio dell'indice della tabella-plot correntemente visualizzata
    */
    QSettings settings;
    int iSheet, i,j, r, filesSaved;
    QString keyName, fileName, pathName, tShift;

    settings.beginGroup("programState");
    //Cancello lo stato salvato in precedenza:
    settings.remove("");

    // Fase 1: salvataggio del multifileMode. Occorre notare che il multifileMode viene salvato anche i GV.PO.multifilemode, ma in un momemto diverso. Non si può quindi fare affidamento a quel valore in quanto può essere diverso da quello che si ha al momento del salvataggio dello stato.
    settings.setValue("multifileMode",GV.multiFileMode);


    // Fase 2: salvataggio del nome dei files, e info su data e ora
    if(GV.multiFileMode)
      filesSaved=numOfLoadedFiles;
    else
      filesSaved=1;
    settings.setValue("Number Of Files",filesSaved);
    j=0;
    for(i=0; i<MAXFILES; i++){
      //Cerco se esiste una riga di file contenente come numero di file i+1:
      for (r=1; r<=MAXFILES; r++){
//        int kk=fileTabItems[r][FILENUMCOL]->text().toInt();
        if(ui->fileTable->item(r,FILENUMCOL)->text().toInt() == i+1)
            break;
      }
      //Se non ho trovato nessun file di indice i passo al file successivo:
      if(r==MAXFILES+1)
          continue;
      //A questo punto i è un indice di file presente in memoria, posizionato nella tabella fileTable alla riga r
      fileName=ui->fileTable->item(r,FILENAMECOL)->text();
      pathName=ui->fileTable->item(r,FILENAMECOL)->toolTip();
      tShift=ui->fileTable->item(r,TSHIFTCOL)->text();

      //Per trovare il nome del file completo del percorso parto dal tooltip, che lo contiene.
      //Il tooltip è fatto così:
      //  fileTooltip="<p><B>Full name:</B> "+ fullName+"</p>";
      // cui è aggiunta poi altra roba.
      //Pertanto prima taglio i primi 21 caratteri,poi taglio quello che c'è a partire dalla prima parentesi angolare aperta
      pathName.remove(0,21);
      pathName.chop(pathName.length()-pathName.indexOf('<',0));

      //Se non sono in multifile, anche se in memoria sono presenti più files salvo solo quello correntemente attivo:
      if(!GV.multiFileMode && i!=selectedFileIdx)
        continue;
      keyName="File_"+QString::number(++j);
      //Oltre al nome del file salvo anche l'informazione su data e ora dell'ultima modifica. In tal modo se al ricaricamento non corrispondono posso accorgermi che i due files sono in realtà diversi e quindi non mi devo attenderere lo stesso contenuto in termini di variabili.
      QFileInfo info1(pathName);
      QDateTime dateTime=info1.lastModified();
      settings.setValue(keyName+".name",pathName);
      settings.setValue(keyName+".date",dateTime);
//Salvo anche il numero attribuito al file, che può non essere sequenziale. Questo in quanto tale numero è poi usato dai dati salvati nelle tabelle delle variabili, e altrimenti si avrebbe un disallineamento fra la numerazione del file nella tabella file e nelle tabelle variabili
      settings.setValue(keyName+".num",i+1);
      //Salvo il tShift
      settings.setValue(keyName+".shift",tShift);
      settings.setValue("fourWin/visible",  myFourWin->isVisible());
      settings.setValue("fourWin/tableIndex", fourTableIndex);

      if(!GV.multiFileMode)
        break;
    }

    // Fase 3: Salvataggio dati delle varTables

    for (iSheet=0; iSheet<MAXSHEETS; iSheet++){
      CVarTableComp * myTable;
      SVarTableState  tableState;
      if(iSheet==0) myTable=ui->varTable1;
      if(iSheet==1) myTable=ui->varTable2;
      if(iSheet==2) myTable=ui->varTable3;
      if(iSheet==3) myTable=ui->varTable4;
      tableState=myTable->giveState();
      keyName="VarTable_"+QString::number(iSheet+1)+".names";
      settings.setValue(keyName, tableState.allNames);
      keyName="VarTable_"+QString::number(iSheet+1)+".xIsFunction";
      keyName="VarTable_"+QString::number(iSheet+1)+".xInfoIdx";
      settings.setValue(keyName, tableState.xInfoIdx);

      ui->loadStateTBtn->setEnabled(true);
    }

    // Fase 4: salvataggio dell'indice della tabella-plot correntemente visualizzata
    int tableIndex=ui->tabWidget->currentIndex();
    settings.setValue("tableIndex", tableIndex);

    // Indicazione del salvataggio avvenuto per 0.7 s (passato il quale il timer rimetterà le cose a posto)
    QRect rect0=saveStateLbl->geometry(), rect=rect0;
    QPoint center=ui->saveStateTBtn->geometry().topRight();
    rect.moveCenter(center);
    rect.moveTop(rect0.top());
    saveStateLbl->setGeometry(rect);
    saveStateLbl->setText("State Saved");

    ui->saveStateTBtn->setVisible(false);
    ui->loadStateTBtn->setVisible(false);
    saveStateLbl->setVisible(true);

    settings.endGroup();
    QTimer::singleShot(700, this, SLOT(resetStateBtns()));
}

void CDataSelWin::resetStateBtns(void){
    ui->saveStateTBtn->setVisible(true);
    ui->loadStateTBtn->setVisible(true);
    saveStateLbl->setVisible(false);

}

void CDataSelWin::on_loadStateTBtn_clicked()
{
    /*
    Fasi delle operazioni di caricamento stato:
  1) recupero il multifileMode e sua attivazione corretta
  2) recupero del nome, corredato di informazione, di data e orario, dei files
     salvati, e loro caricamento eliminando contestualmente quelli già in memoria
  3) Recupero il testo delle celle delle tableComp, e le invio loro.
     Quando tableComp riceve le stringhe ricostruisce gli altri dati interni che ne
     completano lo stato. Faccio i grafici
  4) operazioni finali
*/

  int filesStored;
  QDateTime dateTime;
  QFileInfo fileInfo;
  QSettings settings;
  QString keyName, pathName, tShift;
  settings.beginGroup("programState");

  //1) setto il multifileMode al valore salvato:
  bool multifileMode=settings.value("multifileMode").toBool();
  if(GV.multiFileMode!=multifileMode){
    on_multifTBtn_clicked(multifileMode);
    ui->multifTBtn->setChecked(!ui->multifTBtn->isChecked());
  }

  //Fase 2: recupero nomi files e gestione fileTable
  //Per la fase 1 seguo la seguente procedura:
  // 2.1) leggo le registrazioni dei files e faccio una verifica di validità
  // 2.2) se è tutto OK scarto i files eventualmente già presenti in memoria
  //      e carico quelli nuovi, altrimenti lascio tutto com'è ed emetto
  //      un messaggio di errore.

  filesStored=settings.value("Number Of Files",0).value<int>();
  if(filesStored<1){
    QMessageBox::critical(this,"State not restored", "Error 1 reading Registry");
    return;
  }
  for(int j=1; j<=filesStored; j++){
    keyName="File_"+QString::number(j)+".name";
    pathName=settings.value(keyName,"").value<QString>();
    keyName="File_"+QString::number(j)+".date";
    dateTime=settings.value(keyName).value<QDateTime>();
    //Verifico se il file esiste e se ora e data coincidono con quelli memorizzati
    fileInfo.setFile(pathName);
    bool valid=fileInfo.isFile();
    if(!valid || fileInfo.lastModified()!=dateTime){
      QMessageBox::critical(this,"State not restored",
        "Error: file \""+pathName+"\"\n"+
        "has time and date stamps that do not correspond to those stored when saving state");
      return;
    }
  }
  /*2.2) Se sono arrivato qui tutti i files sono validi, e posso procedere a eliminare quelli attualmente caricati e ricaricare quelli nuovi.
  La manovra è un po' delicata in quanto può darsi il caso che io sia in singleFile ma vi sono più files in memoria che sono visibili solo dopo commutazione in multiFile.
  Pertanto adotto la seguente tecnica:
  - Se numOfSelFiles>1 mi metto in multiFile
  - simulo un doppio click del mouse sulle varie righe di files.
  - mi metto in multiFile o singleFile a seconda del numero di files caricati.
  */
  int i;

  if(filesStored>1 && ! ui->multifTBtn->isChecked()){
    ui->multifTBtn->setChecked(true);
    on_multifTBtn_clicked(true);
  }

  //Eliminazione dei files (simulo ripetuti doppi click sulle varie righe):
  for(i=1;  i<ui->fileTable->rowCount(); i++) {
     if(ui->fileTable->item(1,2)->text()!="")
       removeFile(1);  //l'indice di riga è sempre 1 perché dopo ogni rimozione i nomi sono spostati tutti verso l'alto
  }

  // Caricamento nuovi files.
  // Si ricorda che è necessario che i numeri di files siano quelli usati durante il salvataggio, altrimenti vi sarebbe disallineamento con i dati della tabella delle variabili. Tali numeri sono stati segnati assieme ai nomi dei files e qui li ripristino

  QStringList fileNameLst, fileShiftLst;
//  fileNumsLst.clear();
//  varMaxNumsLst.clear();
  selectedFileIdx=-1;
  for(i=0; i<filesStored; i++){
    QString ret;
    keyName="File_"+QString::number(i+1);
    fileNameLst.append(settings.value(keyName+".name","").value<QString>());
    fileNumsLst[i]=settings.value(keyName+".num").toInt();
    fileShiftLst.append(settings.value(keyName+".shift").value<QString>());
    ret="";
  }

  qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
  loadFileListLS(fileNameLst,fileNumsLst,fileShiftLst);
  qApp->restoreOverrideCursor();

//  ui->varMenuTable->resizeColumnsToContents();
  QResizeEvent *event=0;
  resizeEvent(event);

  // Fase 3: Recupero il testo delle celle delle tableComp, e le invio loro.  Quando tableComp riceve le stringhe ricostruisce gli altri dati interni che ne completano lo stato.
  QStringList list;
  bool xIsFunction;
  int xInfoIdx;
  for (int iSheet=0; iSheet<MAXSHEETS; iSheet++){
    keyName="VarTable_"+QString::number(iSheet+1)+".names";
    list.clear();
    list=settings.value(keyName,"").value<QStringList>();

    keyName="VarTable_"+QString::number(iSheet+1)+".xIsFunction";
    xIsFunction=settings.value(keyName,"").value<bool>();

    keyName="VarTable_"+QString::number(iSheet+1)+".xInfoIdx";
    xInfoIdx=settings.value(keyName,"").value<int>();

  // Sembra che non si possa fare il getState di una varTable quando non è visualizzata. Provo a fare lo switch prima del getState.
//Notare che la seguente riga manda implicitamente in esecuzione un on_tabWidget_currentChanged(), il quale a sua volta manda in esecuzione, se necessario, un setCommon().
//    ui->tabWidget->setCurrentIndex(iSheet);

    if(iSheet==0)
      myVarTable=ui->varTable1;
    if(iSheet==1)
      myVarTable=ui->varTable2;
    if(iSheet==2)
      myVarTable=ui->varTable3;
    if(iSheet==3)
      myVarTable=ui->varTable4;

    myVarTable->getState(list, xIsFunction, xInfoIdx, multifileMode);
    myVarTable->getFileNums(fileNumsLst, varMaxNumsLst);
    if(myVarTable->numOfTotVars>1){
      ui->tabWidget->setCurrentIndex(iSheet);
//if (iSheet==0)
      on_plotTBtn_clicked();
    }

    keyName="fourWin/tableIndex";
    fourTableIndex=settings.value(keyName,"").value<int>();
    bool fourVisible=settings.value("fourWin/visible", false).toBool();
    if(myVarTable->numOfTotVars==2 && fourVisible && iSheet==fourTableIndex)
      on_fourTBtn_clicked();
  }

  //Fase 4: operazioni finali:
  //Seleziono la scheda di plot giusta e faccio le operazioni relative:
  int savedIndex=settings.value("tableIndex",0).value<int>();
  ui->tabWidget->setCurrentIndex(savedIndex);
  on_tabWidget_currentChanged(savedIndex);

  // Indicazione del salvataggio avvenuto per 0.7 s (passato il quale il timer rimetterà le cose a posto)

  QRect rect0=saveStateLbl->geometry(), rect=rect0;
  QPoint center=ui->saveStateTBtn->geometry().topRight();
  rect.moveCenter(center);
  rect.moveTop(rect0.top());
  saveStateLbl->setGeometry(rect);
  saveStateLbl->setText("State Loaded");

  ui->saveStateTBtn->setVisible(false);
  ui->loadStateTBtn->setVisible(false);
  saveStateLbl->setVisible(true);

  settings.endGroup();
  QTimer::singleShot(700, this, SLOT(resetStateBtns()));

}

void CDataSelWin::enterEvent(QEvent *){
  if(plotWin1->isVisible())
     plotWin1->raise();
  if(plotWin2->isVisible())
     plotWin2->raise();
  if(plotWin3->isVisible())
     plotWin3->raise();
  if(plotWin4->isVisible())
     plotWin4->raise();
  if(myFourWin->isVisible())
     myFourWin->raise();

}

void CDataSelWin::mousePressEvent(QMouseEvent *)  {
}

void CDataSelWin::moveEvent(QMoveEvent *){

  /*  Questa funzione serve per identificare lo schermo attuale, per vedere quando cambia
   * e attuare al cambiamento una personalizzazione delle finestre del programma in funzione
   * dell'effettivo DPI.
   *
   * Notare che c'è un problema in questa funzione, credo ascrivibile a Qt e/o a Windows: durante
   * il passaggio da uno schermo all'altro non tutti i font definiti in punti vencono
   * aggiornati. Vengono in particolare aggiornati i font dei bottoni e delle intestazioni
   * delle schede, ma non quelli degli items passati alle tabelle.
   * Andrebbe fatto un lavoro di aggiornamento manuale di tali font, ma questo sembra più
   * qualcosa da discutere nel forum di Qt.
   *
  */

  int screenCount=QGuiApplication::screens().count();
  int screenIdx;
  for (screenIdx=0; screenIdx<screenCount; screenIdx++){
    QRect currAvGeom=QGuiApplication::screens()[screenIdx]->availableGeometry();
    QPoint myPos;
    myPos=this->pos();
    if (currAvGeom.contains(myPos)){
      break;
    }
  }
  if (screenIdx>screenCount-1){
    // "out of range"
    return;
  }

  QScreen * screen=QGuiApplication::screens()[screenIdx];
  int DPI=screen->logicalDotsPerInch();
  qDebug()<<"dpi: "<<DPI;
  if (DPI!=currentDPI){
    //  Voglio che la massima altezza della parte utile sia pari alla massima utilizzabile sul desktop. L'altezza che passo a adaptToDPI è invece l'altezza della parte utile della finestra.
    //pertanto qui tengo conto di questo e riduco maxHeight di conseguenza:
    int maxHeight=FRACTIONOOFSCREEN*screen->availableGeometry().height()
            +frameGeometry().height()-geometry().height();
    adaptToDPI(DPI, maxHeight);
    currentDPI=DPI;
  }
}

void CDataSelWin::on_aboutBtn_clicked(){
    CHelpDlg dialog;
    dialog.exec();
}

void CDataSelWin::on_updateTBtn_clicked(bool checked)
{
    updatingPlot=checked;
}


void CDataSelWin::on_sortTBtn_clicked()
{
/* Con la pressione del bottone si comanda l'ordinamento delle variabili. Per l'ordinamento ascendente o discendente si ordina sulla base del contenuto della colonna delle variabili, colonna N. 1. Per l'inversione dell'ordinamento si ordina sulla base del contenuto della prima colonna, di numero "0".
Affinché questo possa funzionare i numeri in prima colonna devono essere scritti tutti con lo stesso numero di digits. Ad esempio se i numeri sono più di 9 e meno di 100 il primo e il secondo devono essere "01" e "02 e non "1" e "2*/
    switch (sortType){
    case noSort: //going to ascending
       sortType=ascending;
       ui->sortTBtn->setText("A -> Z");
       ui->varMenuTable->sortByColumn(1,Qt::AscendingOrder);
       break;
    case ascending:  //going to descending
        sortType=descending;
        ui->sortTBtn->setText("Z -> A"); //next level is noSort
        ui->varMenuTable->sortByColumn(1,Qt::DescendingOrder);
        break;
    case descending: //going to noSort
        ui->sortTBtn->setText("A <-> Z");
        sortType=noSort;
        ui->varMenuTable->sortByColumn(0,Qt::AscendingOrder);
        break;
    }

}
