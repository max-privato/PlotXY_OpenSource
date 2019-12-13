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
#undef EXCLUDEATPCODE
#include "ExcludeATPCode.h"
#include <QApplication>
#include <QDebug>
#include <QElapsedTimer>
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
extern SGlobalVars GV; //definite in main(); struttura in "Globals.h"


void CDataSelWin::adaptToDPI(qreal currentDPI_, int maxHeight_){

/* Function to adapt the program's behavior to DPI.
   * In the first implementation it was put in the DatasSelWin constructor.
   * It is now isolated because the program has also become multiple-screeen-aware.
   * It may very well happen that the second screen has a different resolution, for
   * example less than one of the first. If the difference is substantial it is probable
   * which has a different DPI value.
   * For example, the main screen can have 144 DPI and the secondary 96.
   * In this case I have to recognize when I'm moving to different DPI and adapt
   * the display.
   *
   * Note that the OS (at least Windows) automatically changes the pixel dimensions specified
   * in points when the window's center line crosses the screen. That would be the
   * ideal time to change also the customized settings of the program, but
   * unfortunately Qt does not raise (AFAIK) any event corresponding to that change.
   * With this function, therefore, the fonts are changed by the system when the
   * window center line crosses the edge of the screen, and then my personalizations when
   * the point (0,0) passes through it.
   * Notice that in Windows this point is not the upper left corner of the visible window;
   * instead, it is a point a little higher and aleft, since a margin is included, which
   * (in versions of Windows prior to 10 was opaque,  now is transparent) wich constitute
   * a border neeeded as a handlle of a window to resize it.
   *
   *
*/
    qreal factorW;
    qreal factorH;

    //Se sono su schermi 4k aumento la dimensione della finestra rispetto a quanto progettato in Designer, ma non la raddoppio; In futuro metterò un'opzione che consentirà all'utente di scegliere fra finestre medie, piccole o grandi. Quella che faccio qui è piccola

    // If you are on 4k screens, increase the window size compared to what was
    // designed in Designer, but not double it; In the future I will put an
    // option that will allow the user to choose between medium, small or
    // large windows. What I do here is small
    if(currentDPI_>100){
      factorW=qMin(1.7,0.9*currentDPI_/96.0);
      factorH=qMin(1.5,0.9*currentDPI_/96.0);
    }else {
      factorW=1.0;
      factorH=1.0;
    }
    resize(int(factorW*originalWidth),int(factorH*originalHeight));
    setMaximumWidth(int(factorW*originalMaxWidth));
    setMinimumWidth(int(factorW*originalMinWidth));

    // ALla fine la massima altezza è bene che sia pari al massimo spazio disponibile in verticale. QUesta logica è ad esempio quella di Qt Creator. Se infatti una finestra alta al massimo viene spostata verso i lbasso non si può più espandere agendo sul bordo superiore, proprio perché appare opportuno non superare l'altezza massima disponibile.

    // At the end the maximum height is good that it is equal to the maximum
    // available vertical space. This logic is, for example, that of Qt Creator.
    // If, in fact, a maximum window is moved towards the bottom, it can no
    // longer be expanded by acting on the upper edge, precisely because it
    // seems advisable not to exceed the maximum available height.
    setMaximumHeight(maxHeight_);

    saveStateLbl->resize(int(1.5*ui->loadTBtn->width()), ui->loadTBtn->height());

    /* Qui adatto le altezze delle tabelle file e varTable. Non lascio che sia fatto
     * in maniera automatica perché Qt lascia troppi spazi sopra e sotto il testo.
     * La cosa migliore è determinare l'altezza delle righe in funzione dell'altezza
     * del font contenuto. In tal modo ho la libertà di definire (in punti) in maniera
     * articolata il font delle celle delle tabelle, il quale è myFont, definito nel costruttore
     * sia in funzione dei tipi dello schermo che delle opzioni di programma che, infine,
     * del sistema operativo. myFont è poi passato passato sopra alle varTable e più sotto
     * alla fileTable.
    */

    /* Here the file table and varTable heights are suitable. I do not let it be done
     * automatically because Qt leaves too many spaces above and below the text.
     * The best thing is to determine the height of the rows according to the height
     * of the font. In this way I have the freedom to define (in points) in a manner
     * articulated the font of the table cells, which is myFont, defined in the constructor
     * depending on the types of the screen and the program options that, finally,
     * of the operating system. myFont then moved on to the varTable and below
     * to theTable file.
    */
    int fontPxHeight=int(currentDPI_/96.0*myFont.pointSize()*16.0/12.0+0.5);
    int cellRowHeight=int(1.4*fontPxHeight);

#if defined(Q_OS_UNIX)
    cellRowHeight*=0.85;
#endif
    for(int i=0;i<=MAXFILES;i++)
      ui->fileTable->setRowHeight(i,cellRowHeight);
    for (int i=0; i<TOTROWS; i++){
        for (int tab=0; tab<MAXPLOTWINS; tab++)
          varTable[tab]->setRowHeight(i,cellRowHeight);
    }
    //Sperimentale: leggo e riscrivo gli item nella prima riga della tabella file per fare aggiornare le dimensioni in pixel del contenuto
    // Experimental: I read and rewrite the items in the first row of the file table to update the pixel dimensions of the content
//      ui->fileTable->setFont(myFont);
// ui->tool468->setVisible(false);
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

/* This function, in addition to making the initializations related to the
   CDatSelWin window, does many general initializations of the whole program,
   this window being the base window of the program.
  Initialization steps:
  A) General setups
  B) possible loading files for past parameters
  C) all the connect of the program except those that have origin and destination
     a same widget other than CDataSelWin. These last distributed connections are:
     - connect the timer in CVarMenu
     - connect the timer in CLineChart
    Another connect is also performed since CDataSelWin does not have it
    visibility of both signal and slot instances:
    - giveUnits connect between CLineCalc and CVarTableComp
  D) passage of the program options to plotWin and fourWin (which are no more
     read for direct access to GV)

  Details of A:
  A1: setupUI and DPI-aware customization of widgets font and CDataSelWin dimensions
  A2: initialization of local simple variables (bool, int, other, in alphabetical order)
  A3: creation of windows (plotWin, FourWin, progOptions)
      ** Decide a uniform window creation rule here: only modals?
      ** modals and dialogs? **
  A4: reading settings and any customization of windows size and position
  A5: Initializations related to the FileTable
  A6: Initializations related to the VarMenuTable
  A7: Initializations related to SelVarTable
*/
    //La seguente riga è fondamentale per la compilazione sotto Ubuntu. Altrimenti i numeri vengono codificati usando la virgola come separatore decimale, con il che tutto il software di I/O da file di testo va in crisi.
    // In sostanza "The std::scanf family of functions are locale aware." Evidentemente il locale default in ubuntu non è quello C standard.

    // The following line is essential for compiling under Ubuntu. Otherwise
    // the numbers are coded using the comma as decimal separator, with which
    // all the I / O software from text files goes into crisis. Basically "The
    // std :: scanf family of functions are local aware." Evidently the default
    // locale in ubuntu is not the standard C.
     setlocale(LC_NUMERIC,"C");
  int i;
  QString ret;

  // Fase A1: setupUI e personalizzazione DPI-aware dei font dei widget e dimensioni di CDataSelWin
  // Phase A1: setupUI and DPI-aware customization of widgets font and CDataSelWin dimensions
  ui->setupUi(this);
  ui->moreLbl->setVisible(false);
  move(toInPrimaryScreen(QPoint(0,0)));
  ui->showParTBtn->setVisible(false);


  for (int tab=0; tab<MAXPLOTWINS; tab++){
    varTable[tab]= new CVarTableComp(this);
    varTable[tab]->setNameNumber(tab+1);
    varTable[tab]->getColorScheme(GV.PO.useOldColors);
    //rendo non visibili le tab che non sono messe nella QTabWidget:
    // I make the tabs that are not in QTabWidget invisible:
    varTable[tab]->setVisible(false);
}
  //La ui contiene anche la saveStateLbl:
  // The ui also contains the saveStateLbl:
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

  int maxHeight=int(FRACTIONOOFSCREEN*screen->availableGeometry().height());
    /* Il secondo parametro che passo nella seguente chiamata a funzione è la massima altezza che posso dare alla finestra. Vorrei che la finestra restasse sempre nello spazio disponibile, inclusa la propria intestazione. Purtroppo questo mi risulta al momento impossibile in quanto i due metodi della presente finestra geometry() e frameGeometry(), che dovrebbero dare valori differenti, danno gli stessi rettangoli e coincidono con la zona utile, al netto di frame e zona del titolo, cioè con quello che secondo la documentazione dovrebbe essre geometry(), ma non frameGeometry().
    * Quindi mi accontento di passare il 95% della altezza utile sul desktop. Il risultato sarà perfetto solo se l'altezza utile di CDataSelWIn è il 95% dell'altezza totale, cioè se le parti accessorie occupano il 5% del totale
*/

  /* The second parameter that I pass in the following function call is the
   * maximum height I can give to the window. I would like the window always
   * to remain in the available space, including its own header.
   * Unfortunately this is currently impossible because the two methods of
   * this window geometry () and frameGeometry (), which should give different
   * values, give the same rectangles and coincide with the useful area,
   * net of frames and area of ​​the title, ie with what according to the
   * documentation should be geometry (), but not frameGeometry ().
   * So I'm content to spend 95% of the useful height on the desktop.
   * The result will be perfect only if the useful height of CDataSelWIn
   * is 95% of the total height, ie if the accessory parts occupy 5% of the total
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

#if defined(Q_OS_DARWIN)
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
  // Increase the points of the text labels
  ui->EqualiseBox->setFont(myFont);
  ui->arrTBtn->setFont(myFont);
  ui->allToBtn->setFont(myFont);
  ui->fileTable->setFont(myFont);
  ui->toWin1Btn->setFont(myFont);
  ui->tabWidget->setFont(myFont);
  for (int tab=0; tab<MAXPLOTWINS; tab++)
    varTable[tab]->getFont(myFont);


  // Fase A2: inizializzazione variabili semplici locali (bool, int, altro,in ord. alfabetico)
  // Phase A2: initialization of local simple variables (bool, int, other, in alphabetical order)
  doubleClicking=false;
  fileLoaded=false;
  firstFourPerSession=true;
  goneToSingleFile=false;
  paramWinTableBuilt=false;
  refreshUpdate=ui->refrTBtn->isChecked();
  updatingPlot=false;

  actualPlotWins=MAXPLOTWINS/2;    // Initially instead of 8 I have 4 windows
  funXVar=nullptr;
  currentTableIndex=ui->tabWidget->currentIndex();
  numOfLoadedFiles=0;
  selectedFileIdx=-1;
  selectedFileRow=-1;  // Value unacceptable in normal use; it is used to indicate
                       // that no file has yet been selected

  headerGray.setRgb(210,210,210);
  neCellBkColor.setRgb(240,240,240);

  for(i=0; i<MAXFILES; i++){
    fileNamesLst.append(" ");
    fileNumsLst.append(0);
    varMaxNumsLst.append(0);
    topIndex[i]=0;
    freeFileIndex<<i;
  }
  myVarTable=varTable[0];
  //In alcuni PC la seguente riga crea un segment fault. Probabilmente perché a questo punto la tabWidget non è stata in realtà ancora creata
  // In some PCs the following line creates a segment fault. Probably because at this point the tabWidget has not actually been created yet
  //ui->tabWidget->setCurrentIndex(0);

  //generazione e inizializzazione delle saveStrings:
  // generation and initialization of saveStrings:
  saveStrings=new QString[ui->fileTable->columnCount()];
  for (int i=0; i<ui->fileTable->columnCount(); i++)
      saveStrings[i]="";

  // Creazione e inizializzazione degli oggetti mySO:
  // Creation and initialization of mySO objects:
  for(i=0;i<MAXFILES; i++) {
    mySO[i]=new CSimOut(this);
    mySO[i]->useMatLib=GV.PO.useMatLib;
  }


  //Fase A3: creazione di finestre  (plotWin, FourWin, progOptions)  ** Decidere una regola uniforme di creazione delle finestre qui: solo le modali? le modali e le dialog? **
  // Step A3: create windows (plotWin, FourWin, progOptions) ** Decide a uniform window creation rule here: only modals? modals and dialogs? **

  for (int win=0; win<MAXPLOTWINS; win++){
    plotWin[win] = new CPlotWin();
    QString title;
    title.setNum(win+1);
    title="Plot "+title;
    plotWin[win]->setWindowTitle(title);
    plotWin[win]->setDrawType(GV.PO.drawType);

    fourWin[win]=new CFourWin();
    title.setNum(win+1);
    title= "Fourier chart "+title;
    fourWin[win]->setWindowTitle(title);
  }
  myPlotWin=plotWin[0];
  myFourWin=fourWin[0];

  //Il seguenti for vanno messi qui perché l'aggiunta di tab comanda in esecuzione tabChanged, che a sua volta opera su plotWin, che devono essere create.
  // The following should be placed here because the addition of tab commands running tabChanged, which in turn operates on plotWin, which must be created.
  for (int tab=ui->tabWidget->count()-1; tab>=0; tab--){
    ui->tabWidget->removeTab(tab);
  }
  for (int tab=0; tab<actualPlotWins; tab++){
    QString label;
    label.setNum(tab+1);
    if(actualPlotWins<5)
        label="Plot "+label;
    ui->tabWidget->insertTab(tab,varTable[tab],label);
  }

  myParamWin= new CParamView(this);
  myParamWin->setWindowTitle("MC's PlotXY - Parameters");

  myProgOptions= new CProgOptions(this);
  myProgOptions->setWindowTitle("MC's PlotXY - Program options");


  // ***
  //Fase A4: lettura settings ed eventuale personalizzazione dimensione e posizione finestre
  // Step A4: read settings and any customization of windows size and position
  // ***
  QSettings settings;
  if(GV.PO.rememberWinPosSize){
    GV.multiFileMode=settings.value("multifileMode").toBool();
    //La seguente riga non può essere usata perché in questo momento non sono stati ancora allocati gli items per la varTable, e quindi non si possono fare su di essa le trasformazioni richieste (altrimenti: SEGMENT FAULT!) Pertanto essa viene spostata in showEvent().

    // The following line can not be used because at this time the items for
    // the varTable have not yet been allocated, and therefore the required
    // transformations can not be done on it (otherwise: SEGMENT FAULT!)
    // Therefore it is moved to showEvent ( ).

    //if(!multiFile) on_multifTBtn_clicked(true);

    resize(settings.value("dataSelWin/size", size()).toSize());

    for (int win=0; win<MAXPLOTWINS; win++){
      QString value;
      value.setNum(win+1);
      value="plotWin"+value+"/size";
      plotWin[win]->resize(settings.value(value).toSize());

      value.setNum(win+1);
      value="fourWin"+value+"/size";
      fourWin[win]->resize(settings.value(value).toSize());
    }


  /* Gestione smart degli schermi.
   * 1) mi devo assicurare che le finestre plot siano all'interno dello spazio
   *    di visualizzazione disponibile. Questo non è sempre vero ad esempio se
   *    sono state salvate quando ero con risoluzione più elevata oppure con schermo
   *    secondario attivo e ora la risoluzione o il numero di schermi è minore
   * 2) mi devo assicurare che la posizione delle finestre non copra le barre
   *    di sistema. Questo accadrebbe ad es. sul mac nella posizione default
   *    delle finestre, cioè (0,0)
   *
   * Se sto spostando una finestra da fuori a dentro emetto un MessageBox solo se la
   * finestra è visibile
  */

  /* Smart screen management.
   * 1) I have to make sure that the plot windows are inside the space
   * display available. This is not always true for example if
   * were saved when I was with a higher resolution or with a screen
   * active secondary and now the resolution or number of screens is lower
   * 2) I have to make sure that the position of the windows does not cover the bars
   * of system. This would happen eg. on the mac in the default position
   * of the windows, that is (0,0)
   *
   * If I am moving a window from outside to inside I will issue a MessageBox only if the
   * window is visible
  */
    bool someWinDisplaced=false, isDisplacedVisible=false;

    QPoint posPoint;
    int screenCount=QGuiApplication::screens().count();
    QScreen *firstScreen=QGuiApplication::primaryScreen();
    QScreen *lastScreen=QGuiApplication::screens()[screenCount-1];
    QRect firstScrAvGeometry=firstScreen->availableGeometry();
    QRect lastScrAvGeometry=lastScreen->availableGeometry();
    int firstScrAvRight=firstScrAvGeometry.right();
    int lastScrAvRight=lastScrAvGeometry.right();

    posPoint=settings.value("dataSelWin/pos").toPoint();

    // First step: bring in if the horizontal space available
    // has been reduced, e.g. because I no longer have the secondary screen:
    if(posPoint.x()+0.5*this->width()>lastScrAvRight){
       someWinDisplaced=true;
       posPoint.setX(lastScrAvRight-this->width()-5);
    }
    this->move(posPoint);
    //Secondo passaggio: se sono nello schermo primario devo evitare di andare
    // a finire nelle zone riservate dalle barre di sistema

    // Second passage: if I'm on the primary screen, I have to avoid going
    // to finish in the areas reserved by the system bars
    if (posPoint.x()+this->width()<firstScrAvRight)
        this->move(toInPrimaryScreen(posPoint));


    for (int win=0; win<MAXPLOTWINS; win++){
      QString value;
      value.setNum(win+1);
      value="plotWin"+value+"/pos";
      posPoint=settings.value(value).toPoint();
      if(posPoint.x()+0.5*plotWin[win]->width()>lastScrAvRight){
         someWinDisplaced=true;
         posPoint.setX(lastScrAvRight-plotWin[win]->width()-5);
         if(plotWin[win]->isVisible())
           isDisplacedVisible=true;
      }
      plotWin[win]->move(posPoint);
      if (posPoint.x()+plotWin[win]->width()<firstScrAvRight)
        plotWin[win]->move(toInPrimaryScreen(posPoint));
      value.setNum(win+1);
      value="fourWin"+value+"/pos";
      posPoint=settings.value(value).toPoint();

      // for the time being non smart management of fourier windows:
      fourWin[win]->move(posPoint);
      if(posPoint.x()+fourWin[win]->width()<firstScrAvRight)
        fourWin[win]->move(toInPrimaryScreen(posPoint));
    }

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

// FRAMEGEOMETRY_COMMENT (do not delete serves as a cross comment label)
/* At this point the default position of the windows should be set in a similar way
 * how it is done by clicking on the "arrange" button. However, it was seen that QWidget
 * returns slightly incorrect information about the FrameGeometry if a window, though
 * built, it is not yet displayed. The function instead provides the correct indication
 * when we are inside ShowEvent.
 * This problem can be solved for the DataSelWin window by choosing the location
 * horizontal of the Plot1 and Plot3 windows not from here but from ShowEvent, and so it was done.
 * It is much more complicated for the vertical position of the Plot2 and Plot4 windows,
 * and for the horizontal position of Plot2 and Plot4. At the moment I keep this mistake,
 * that is small.
*/
    // on_arrTBtn_clicked();
  }


  //Eventuale attivazione tasto ricaricamento dello stato:
  // Possible activation of the status reload button:
  QStringList groups = settings.childGroups();
  if(groups.contains("programState"))
  ui->loadStateTBtn->setEnabled(true);

  // ***
  // Fase A5: Inizializzazioni relative alla tabella File
  // Step A5: Initializations related to the File table
  // ***
  //Tolgo la scrollbar verticale:
  // I remove the vertical scrollbar:
  ui->fileTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  //Inizializzazione degli items per la fileTable:
  // Initialization of items for the fileTable:
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
// Tmax and TShift must be thoroughly clearer to make it clear that they are clickable
        if(j>4)
          item->setBackgroundColor(headerGray.lighter(110));
          item->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      }else{
        if(j!=6)
          item->setFlags(item->flags()&~Qt::ItemIsEditable);
        item->setBackgroundColor(neCellBkColor);
        //La sola colonna con TShift la metto bianca per far capire che è editabile:
        // The only column with TShift I put it white to make it clear that it is editable:
        if(j==6)
          item->setBackgroundColor(Qt::white);
      }
      if(j<2)
        item->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      ui->fileTable->setItem(i,j,item);
    }
  }
  //Normalmente l'ultima colonna (dei tShift) è nascosta:
  // Normally the last column (of the tShift) is hidden:
  ui->fileTable->hideColumn(ui->fileTable->columnCount()-1);

  ui->fileTable->resizeColumnsToContents();
  //Ora aumento la colonna FileName di ADJUST pixel e riduco di altrettanto quella di Tmax:
  // Now I increase the FileName column of ADJUST pixels and I reduce the Tmax one too:
  #define ADJUST 0
  ui->fileTable->setColumnWidth(2,ui->fileTable->columnWidth(2)-ADJUST);
  ui->fileTable->setColumnWidth(5,ui->fileTable->columnWidth(5)+ADJUST);
  // rendo hide tutte le righe eccetto le prime
  // I render hide all the lines except the first ones
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
  // STEP A6: Initializations related to the varMenuTable table
  // ***
  //Tolgo la scrollbar orizzontale:
  // I remove the horizontal scrollbar:
  sortType=noSort;
  ui->varMenuTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  ui->varMenuTable->setRowCount(0);

  // ***
  // FASE A7: Inizializzazioni relative alla tabella SelVarTable
  // STEP A7: Initializations related to the SelVarTable table
  // ***
  for (int tab=0; tab<MAXPLOTWINS; tab++)
    varTable[tab]->neCellBkColor=neCellBkColor;

  myVarTable->setMultiFile(GV.multiFileMode);
  //L'altezza delle righe in CDataSelWin.::adaptToDPI
  // The height of the lines in CDataSelWin.::adaptToDPI

  //***
  //Fase B: eventuale caricamento files per parametri passati
  // Phase B: possible loading files for past parameters
  //***
  //i nomi dei files vengono dopo le opzioni, quindi GV.PO.firstFileIndex contiene l'indice del primo file da caricare:
  // file names come after the options, so GV.PO.firstFileIndex contains the index of the first file to load:
  QStringList fileNameLst;
  int i0=GV.PO.firstFileIndex;

   for(int i=i0; i<QCoreApplication::arguments().count(); i++){
    if(i==MAXFILES+i0-1)break;
    fileNameLst.append(QCoreApplication::arguments().at(i));
  }
  loadFileList(fileNameLst);

  //Fase C:  tutti i connect del programma eccetto quelli che hanno come origine e destinazione un medesimo widget diverso da CDataSelWin, e quelli le cui funzioni non sono acceswsibili (ad es. i signal di ClineChart collegati agli Slot di CPlotWin)
  // Phase C: all the connections of the program except those whose source and destination have the same widget different from CDataSelWin.

  for (int win=0; win<MAXPLOTWINS; win++){
    connect(myProgOptions,&CProgOptions::programOptionsChanged,
            plotWin[win],&CPlotWin::updateChartOptions);
  }
  connect(myProgOptions,&CProgOptions::programOptionsChanged,
          myFourWin,&CFourWin::updateChartOptions);
  connect(ui->varMenuTable, &CVarMenu::myCellClicked, this,
          &CDataSelWin::varMenuTable_cellClicked);
  // Il numero delle righe nelle varTable# è selezionato automaticamente nei rispettivo costruttore a TOTROWS che è pari a MAXVARS+1  (attualmente quest'ultimo è pari a 9)
  // connect che servono per superare lo strano problema dell'ultima cella che rimane gialla:

  // The number of lines in the varTable # is automatically selected in the
  // respective constructor to TOTROWS which is equal to MAXVARS + 1
  // (currently the latter is equal to 9) connect that serve to overcome
  // the strange problem of the last cell that remains yellow:
  for (int tab=0; tab<MAXPLOTWINS; tab++){
    connect(ui->varMenuTable,SIGNAL(draggingDone()),varTable[tab],SLOT(blankCell()));
    connect(varTable[tab],&CVarTableComp::contentChanged,this,&CDataSelWin::varTableChanged);
    connect(varTable[tab],&CVarTableComp::queryFileInfo,this,&CDataSelWin::giveFileInfo);
  }

  connect(ui->varMenuTable,&CVarMenu::groupSelected,this, &CDataSelWin::groupSelected);

  for (int win=0; win<MAXPLOTWINS; win++){
    connect(plotWin[win],&CPlotWin::winActivated,this,&CDataSelWin::updateSheet);
    connect(fourWin[win],&CFourWin::winActivated,this,&CDataSelWin::updateSheet);
  }


  //    D) passaggio delle opzioni di programma a plotWin e fourWin (che non sono più lette per accesso diretto a GV)
  // D) passing program options to plotWin and fourWin (which are no longer read for direct access to GV)
  emit myProgOptions->programOptionsChanged(GV.PO);

  //A questo punto posso mettere il cursore standard del mouse (il cursore di "busy" è stato attivato in main())
  // At this point I can put the standard mouse cursor (the "busy" cursor has been activated in main ())
 qApp->restoreOverrideCursor();
}

void CDataSelWin::checkCalcData(SXYNameData calcData, QString & ret){
   /* Qui nella routine chiamante occorre verificare che i files e le variabili siano esistenti.
    */
   /* Here in the calling routine it is necessary to verify that files and variables exist.
    */
  ret="";
  for(int i=0; i<calcData.varNumsLst.count(); i++){
    int iFile=calcData.varNumsLst[i].fileNum; //Numeri che contano a radice 1
                                              // Numbers that count at root 1
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
  /* This event is executed when the user clicks on the close button of the system bar window.
  */
  QSettings settings;
  if(!GV.PO.rememberWinPosSize)
    goto quit;
  settings.setValue("dataSelWin/size", size());
  settings.setValue("dataSelWin/pos", pos());
  settings.setValue("multifileMode", GV.multiFileMode);

  for (int win=0; win<MAXPLOTWINS; win++){
    QString valueNum, value;
    valueNum.setNum(win+1);
    value="plotWin"+valueNum+"/size";
    settings.setValue(value, plotWin[win]->size());
    value="plotWin"+valueNum+"/pos";
    settings.setValue(value,  plotWin[win]->pos());
    value="fourWin"+valueNum+"/size";
    settings.setValue(value, fourWin[win]->size());
    value="fourWin"+valueNum+"/pos";
    settings.setValue(value,  fourWin[win]->pos());
  }

quit:
  /*  La cancellazione di plotWin# delle seguenti 4 righe non è indispensabile in quanto
   * subito dopo vi è un qApp->quit() che causa l'uscita da a.exec() in main(), e quindi
   * l'uscita dall'applicazione. A quanto si legge su Internet, all'uscita dall'
   * applicazione il sistema operativo è in grado di liberare tutta la memoria allocata
   * dal programma non già liberata durante l'esecuzione da un delete.
   * Il richiamo esplicito a delete qui riportato mi fa fare la chiusura delle finestre
   * e la relativa disallocazione prima che il controllo del programma sia passato al
   * sistema operativo.
   *
   * NOTA de avessi messo questi delete in ~CDataSelWin() vi sarebbe stato il seguente
   * problema (che si evidenzia solo nel mio Win XP in VirtualBox del calcolatore fisso
   * HP Pavillon):
   * se chiudo la DatSelWin con una finestra plot aperta, tale finesta plot rimane aperta.
   * Questo comportamento è facilmente spiegabile. Occorre per prima cosa rimarcare che
   * si esce da application.exec()(cioè la riga a.exec() di main()) soltanto quando tutte
   * le finestre primarie (quelle cioè che non hanno parent) vengono chiuse. Pertanto
   * fintanto che rimangono aperte finestre plotWin#, non si esce da a.exec() di main().
   * E solo dopo che, in main(), si esce da a.exec(), si va oltre. Dopo a.exec() in main
   * non vi sono altre righe, quindi si passa al delete delle variabili automatiche. Una
   * di queste è CDataSelWin w; il delete di w provoca l'esecuzione di ~CDataSelWin().
   * Quindi se metto i delete di plotWin# in ~CDataSelWin() essi non vengono esequiti
   * quando l'utente clicca sulla "x" di CDataSelWin, se è ancora visibile qualche
   * finestra di plot, in quanto in tal caso CDataSelWin non è l'ultima finestra primaria
   * che viene chiusa. Vengono solo esequiti successivamente, cioè quanto anche tutte le
   * plotWin# sono state manualmente chiuse dall'utente.
   *
   * Infine una considerazione sul confronto fra "delete plotWin1" e "plotWin1->close()".
   * QWidget::close() non libera la memoria e non esegue il distruttore, a meno che
   * l'attributo Qt::WA_DeleteOnClose non sia stato settato ad esempio alla creazione
   * della finestra(non è settato per default).
   * Vista la situazione la soluzione con il delete, standard del C++,  appare preferibile.
  */

  /* The deletion of plotWin # of the following 4 lines is not indispensable
   * since immediately after there is a qApp-> quit () which causes the exit
   * from a.exec () in main (), and then the exit from application. As we read
   * on the Internet, at the exit from the application the operating system
   * is able to free all the memory allocated by the program not already freed
   * during execution by a delete.
   * The explicit recall to delete reported here makes me close the windows
   * and the relative disallocation before the control of the program is
   * passed to the operating system.
   *
   * NOTE I had put these delete in ~ CDataSelWin () there would have been the
   * following problem (which is highlighted only in my Win XP in VirtualBox
   * of the HP Pavilion fixed computer):
   * if I close the DatSelWin with an open plank window, this plank remains open.
   * This behavior is easily explained. First of all, we need to remark that we
   * exit application.exec () (that is, the line a.exec () of main ()) only when
   * all the primary windows (ie those that do not have a parent) are closed.
   * Therefore, as long as open plotWin # windows remain, you do not exit
   * a.exec () of main (). And only after that, in main (), we leave a.exec (),
   * we go further. After a.exec () in main, there are no other lines,
   * so the automatic variables are deleted. One of these is CDataSelWin w;
   * the delete of w causes the execution of ~ CDataSelWin (). So if I put the
   * delete of plotWin # in ~ CDataSelWin () they are not performed when the
   * user clicks on the "x" of CDataSelWin, if some plot window is still visible,
   * because in this case CDataSelWin is not the last window primary that is closed.
   * They come only esequiti subsequently, that is how much also all the plotWin #
   * have been manually closed by the user.
   *
   * Finally a consideration on the comparison between "delete plotWin1" and
   * "plotWin1-> close ()".
   * QWidget :: close () does not free the memory and does not perform the
   * destructor unless the Qt :: WA_DeleteOnClose attribute has been set for
   * example when the window is created (it is not set by default).
   * Given the situation the solution with the delete, C ++ standard, appears preferable.
  */

  for (int win=0; win<MAXPLOTWINS; win++)
    delete plotWin[win];
  /*La seguente riga non è necessaria in quanto a questo punto tutte le plotWin si sono chiuse e l'ultima finestra primaria, CDataSelWin si sta per chiudere.
    Quanto tutte le finestre primarie sono chiuse qApp->quit() viene chiamata automaticamente, e quindi si esce da a.exec(), e quindi da main().  Per riferimento guardare QApplication::lastWindowClosed():
    La riga non è necessaria ma non fa male! Se la lasciamo il programma si chiude correttamente anche qualora in un secondo momento (ma non credo proprio) si dovesse porre
    quitOnLastWindowClosed=false
    */

  /* The following line is not needed because at this point all the PlotWin have
    been closed and the last primary window, CDataSelWin, is about to close.
    When all the primary windows are closed, qApp-> quit () is called
    automatically, and then exits from a.exec (), and then from main ().
    For reference look at QApplication :: lastWindowClosed ():
    The line is not necessary but it does not hurt! If we leave it,
    the program closes correctly even if at a later time (but I do not think so)
    it should be placed
    quitOnLastWindowClosed = false
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

  /* This function is used to evaluate the common name of the variable X in the case of operating in multi-mode mode.
   This name is assigned according to the following criteria:
   - if all the X variables have the same name, it is used as a name
     common
   - otherwise, if all the X variables start with t or f, they are
     interpreted respectively by time or frequency and by the common name comes
     attributed the string "t *" or "f *" respectively
   - in all other cases, the string "x" is assigned to the common name.
   */
  bool allSameName=true, allSameFirstChar=true;
  int i;
  QString commonXName="", commonX;
  //Per prima cosa, tramite il seguente ciclo for, valuto il candidato a fare commonX, ovvero il nome comune per le variabili X:
  // First, through the following for loop, I evaluate the candidate to make commonX, which is the common name for the X variables:
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
  // Now I see if all the files have the same CommonXName and CommonX
  for(i=0; i<MAXFILES; i++){
    //se il file i non è caricato non lo considero:
    // if the file i is not loaded I do not consider it:
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
                                  // puts the pointer in the accept position
}


void CDataSelWin::dropEvent(QDropEvent *event)
{
  /* Function for loading files that are "dropped" on the window */

  int i;
  QString ret;
  QStringList fileList;
  const QMimeData *mimeData = event->mimeData();
  for(i=0; i<mimeData->urls().count(); i++)
    fileList.append( mimeData->urls().at(i).path());
  ret=loadFileList(fileList);
  resizeEvent(nullptr);
  ui->varMenuTable->resizeColumnsToContents();
  if(ret=="")
    event->acceptProposedAction();
  else{
    // Qui in realtà il messaggio non va emesso perché ci pensa già LoadFileList:
    //QMessageBox::critical(this,"CDataSelWin.cpp","unable to load file: "+ret)
  }
  ui->multifTBtn->setEnabled(true);
}



void CDataSelWin::groupSelected(int beginRow, int endRow){
  /* Questo slot gestisce la selezione di gruppo dalla varMenu
  */
  /* This slot handles group selection from the varMenu
  */
  QString varName, fileName;
  for(int row=beginRow; row<=endRow;row++){
    varName=ui->varMenuTable->item(row,1)->text();
    fileName=ui->fileTable->item(selectedFileRow,2)->text();
    int varNum=ui->varMenuTable->item(row,0)->text().toInt();
    if(mySO[selectedFileIdx]->fileType==MAT_Modelica){
    // In this case, the unit of measure is in the sVars list of SVar structures of the current mySO file.
    // The right element of sVars is found following the sequential order: the variables selected and
    // displayed in fileMenuTable following the order of the variables in mySO [selectedFileIdx].

       QString myUnit=mySO[selectedFileIdx]->sVars[row].unit;
       myVarTable->setVar(varName,varNum,selectedFileIdx+1,false, false,myUnit);
     }else
       myVarTable->setVar(varName,varNum,selectedFileIdx+1,false, false,"");
  }
  if(myVarTable->numOfTotVars>1){
    ui->plotTBtn->setEnabled(true);
    ui->saveVarsBtn->setEnabled(true);
  }
  if(myVarTable->numOfTotVars==2 && myVarTable->xInfo.isMonotonic)
    ui->fourTBtn->setEnabled(true);
  else
    ui->fourTBtn->setEnabled(false);

  //Se ho selezionato più di quante variabili possono essere visualizzate nella tabella SelectedVars rendo visibilie la label "more...", che viene poi resa nuovamente visibile se l'utente allarga la tabella al punto che non è più necessaria.
  //rettangolo dell'ultima riga visualizzabile di myVarTable:

  // If I have selected more than how many variables can be displayed in the
  // SelectedVars table I make the "more ..." label visible, which is then
  // made visible again if the user widens the table to the point where it is no longer needed.
  // rectangle of the last visible row of myVarTable:
  QRegion region=myVarTable->visibleRegion();
  QVector <QRect> rects=region.rects();
  int maxRectHeight=0;
  for (int i=0; i<rects.count(); i++){
    maxRectHeight=max(maxRectHeight,rects[i].height());
  }
  // int iii=myVarTable->givehighestUsedRowIdx();
  // int jjj=(myVarTable->givehighestUsedRowIdx()+1)*myVarTable->rowHeight(0);
  if((myVarTable->givehighestUsedRowIdx()+1)*myVarTable->rowHeight(0)>maxRectHeight+1 )
    ui->moreLbl->setVisible(true);
//  if(lastRow*myVarTable->rowHeight(0)>maxRectHeight )
//      ui->moreLbl->setVisible(true);
}


float *  CDataSelWin::integrate(float * x, float * y, int nPoints){
  /* Effettua il calcolo della funzione integrale y(x) fra 0 e X con il metodo dei trapezi.
   * Riscrive l'integrale nello stesso vettore y  di ingresso.
   */

  /* Performs the calculation of the integral function y (x) between 0 and X with the trapezoidal method.
   * Rewrite the integral in the same input vector y.
   */
    int i;
    float integ=0.0;
    for(i=0; i<nPoints-1; i++){
//      if(fabs(x[i]-x[i+1])) continue;
      integ +=(x[i+1]-x[i])*(y[i]+y[i+1])/2.f;
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
   5. Caricamento della SelectedVarsTable e della ParamWinTable

************************
      . viene effettuato l'aggiornamento dei grafici di tutte le finestre
  */
/*
    fileIndex: index of the file to be loaded, mySO argument (from 0 to MAXFILES)
               given the possibility that the user can upload and download
               files arbitrarily, the indexes may not be consecutive.
               The number displayed in multiFile is indexIndex + 1.
    fileName: full name of the file to upload (with or without path)

    Function for loading both generic and "refresh" files.
    The program does, in order:
    1. Load in memory, at fileIndex location, the file whose name fullName
    2. Copy the related data to theTable file. In case of refresh the data come
       written on the pre-existing data; otherwise the uploaded file
       becomes the selected one
    3. Update numOfSelFiles
    4. The variable names in the uploaded file are placed in the
       relative varMenuTable
   5. Uploading the SelectedVarsTable and the ParamWinTable

************************
      . the graphics of all the windows are updated
  */
  bool updatingFile=false; //true se in singleFile copio un file sopra il precedente (rimane false in caso di refresh o refresUpdate)
                           // true if in singleFile I copy a file over the previous one (remains false in case of refresh or refresUpdate)
  int i, freeGridRow;
  QFileInfo FI=fileName;
  //Il fileName passato può contenere il path oppure no. Il nome sicuramente senza path è il seguente strictName
  // The pastName file can contain the path or not. The name definitely without path is the following strictName
  QString fullName=FI.absoluteFilePath(), strictName=FI.fileName(), ext=FI.suffix();
  QString ret;
  SReturn retS;


  QElapsedTimer timer;
  timer.start();

  //Fase 0: verifica di congruenza.
  // Phase 0: congruence check.
  if (freeFileIndex.isEmpty())
    return "File table already full";

  if(selectedFileIdx==fileIndex && !refresh && !refreshUpdate)
    updatingFile=true;

  //Fase 1: caricamento del file di nome fileName.
  // Step 1: upload the file named fileName.
  ext=ext.toLower();
  if(ext!="adf" && ext!="cfg" && ext!="lvm" && ext!="mat" && ext!="pl4" && ext!="csv") {
    ret="file extension\""+ext+"\" is  invalid\n(only \nadf, cfg, csv, lvm, mat, or pl4 \nare allowed)";
    return ret;
  }
  mySO[fileIndex]->commasAreSeparators=GV.PO.commasAreSeparators;
  mySO[fileIndex]->trimQuotes=GV.PO.trimQuotes;
  retS.code=0;
  retS.msg="";
  if(ext=="adf")
    ret=mySO[fileIndex]->loadFromAdfFile(fileName);
  if(ext=="cfg")
    ret=mySO[fileIndex]->loadFromComtradeFile(fileName);
  if(ext=="csv")
    ret=mySO[fileIndex]->loadFromAdfFile(fileName, true);
  if(ext=="lvm")
    ret=mySO[fileIndex]->loadFromLvmFile(fileName);
  if(ext=="mat")
    ret=mySO[fileIndex]->loadFromMatFile(fileName,!GV.PO.compactMMvarMenu,true);
  if(ext=="pl4"){
    #ifdef EXCLUDEATPCODE
      retS.code=1;
      retS.msg="Unknown file extension \"pl4\".  Ignoring file:\n"
          +fileName+"\nand all subsequent files in the input list";
    #else
      retS=mySO[fileIndex]->loadFromPl4File(fileName);
    #endif
  }
  if (retS.msg!="")
    ret=retS.msg;
  if(retS.code==2){
    //Mi sembra che le tre righe qui sotto non vadano bene in quanto mi fanno emettere ora un messaggio di errore, mentre negli altri casi il messaggio è riportato in ret ed è compito del programma chiamante emettere il messaggio.
//  Altrimenti viene emesso due volte di seguito.
//  Le sostituisco con la semplice assegnazione a ret di retS.msg:
    ret=retS.msg;
    /*
    qApp->setOverrideCursor(QCursor(Qt::ArrowCursor));
    QMessageBox::warning(this,"",retS.msg);
    qApp->restoreOverrideCursor();
    */
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
  /* If the number of points is less than 2 I can not make the graph. While in the
   * loading case via button Plot this condition is intercepted
   * when selecting the variables to plot (this is allowed
   * to see the list), in the case of Refresh is intercepted here.
   * However, if the refresh is automatic, no error message is output.
  */
  if(refresh && mySO[fileIndex]->numOfPoints<2){
    return "";
  }

  //Fase 2: Copia dei dati nella fileTable (attraverso i suoi items).
  //Trovo la riga su fileTable su cui scrivere e la metto in selectedFileRow (se refresh è true selectedFileRow e selectedFileIdx non vanno cambiati):

  // Step 2: Copy data to the fileTable (through its items).
  // I find the line on fileTable to write on and put it in selectedFileRow
  // (if refresh is true selectedFileRow and selectedFileIdx should not be changed):
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
    // I make the first line free the selected row, and copy the data into the cells
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

/**** VERY IMPORTANT ****
  In the following lines the tooltip of the file name is generated, which
  contains, among other things, the name of the complete path file.
  The information of the full path name, variable "fullName", is then extracted
  from the tooltip itself each time the refresh button is pressed.
  Therefore, EVERY CHANGE THAT YOU WANT TO MAKE THE FOLLOWING TOOLTIP GENERATION
  CODE WILL NEED TO CORRECT ANOTHER IN THE CORRESPONDING CODE IN "CDataSelWin :: on_refrTBtn_clicked ()"
  Furthermore, EVERY CHANGE THAT YOU WANTED TO DO THE NEXT TOOLTIP GENERATION
  CODE WILL NEED TO CORRECT ANOTHER IN THE CODE THAT MANAGES THE RESCUE AND RESTORATION OF THE STATE "

*/
  fileTooltip="<p><B>Full name:</B> "+ fullName+"</p>";
  fileTooltip.append("<B>Type:</B> "+type);
  if(mySO[fileIndex]->runType==rtTimeRun)
      fileTooltip.append("; <I>time-based data</I>.");
  else if (mySO[fileIndex]->runType==rtFreqScan)
      fileTooltip.append("; <I>frequency scan</I>.");
  //Altrimenti runTime è rtUndefined e non aggiungo nulla
  if(GV.multiFileMode){
    ui->fileTable->item(selectedFileRow,2)->setToolTip(fileTooltip);
    ui->fileTable->item(selectedFileRow,3)->setText(QString::number(mySO[fileIndex]->numOfVariables));
    ui->fileTable->item(selectedFileRow,4)->setText(QString::number(mySO[fileIndex]->numOfPoints));
    ui->fileTable->item(selectedFileRow,5)->setText(QString::number(double(mySO[fileIndex]->y[0][mySO[fileIndex]->numOfPoints-1])));
  }else{
    ui->fileTable->item(1,2)->setToolTip(fileTooltip);
    ui->fileTable->item(1,3)->setText(QString::number(mySO[fileIndex]->numOfVariables));
    ui->fileTable->item(1,4)->setText(QString::number(mySO[fileIndex]->numOfPoints));

    ui->fileTable->item(1,5)->setText(QString::number(double(mySO[fileIndex]->y[0][mySO[fileIndex]->numOfPoints-1])));
  }
//  fileTabItems[selectedFileRow][6]->setText("0");
  GV.varNumsLst[fileIndex]=mySO[fileIndex]->numOfVariables;
  //Elimino eventuali caratteri di spaziatura presenti in cima o fondo ai nomi. Questo è utile quando si deve fare la marcatura sulla leggenda del grafico, in modo che il simbolo di marcatura sia il più possibile vicino all'ultimo carattere non bianco

  // Remove any space characters at the top or bottom of the names.
  // This is useful when you have to mark the legend of the chart,
  // so that the mark symbol is as close as possible to the last non-white character
  for(i=0; i<mySO[fileIndex]->numOfVariables; i++){
    QString str=mySO[fileIndex]->varNames[i];
    mySO[fileIndex]->varNames[i]=mySO[fileIndex]->varNames[i].trimmed();
  }

  // ***
  //Fase 3: Gestione numOfLoadedFiles. Si ricordi che il refresh ricarica il solo file selezionato. Il numero di files caricati non cambia.
  // Phase 3: Management of numOfLoadedFiles. Remember tht when refreshing we re-load just the selected file, and the number of total loaded files does not change
  // ***
  if(!updatingFile && GV.multiFileMode &&!refresh)
      numOfLoadedFiles++;
  if(numOfLoadedFiles>=MAXFILES){
    setAcceptDrops(false);
    ui->loadTBtn->setEnabled(false);
  }

  // ***
  //Fase 4: Caricamento dei nomi delle variabili sulla varMenuTable
  // Step 4: Upload variable names to the varMenuTable
  // ***
  fillVarMenuTable(fileIndex);

  fileLoaded=true;

  // ***
  //Fase   5. Caricamento della SelectedVarsTable e della ParamWinTable
  // Phase 5. Uploading the SelectedVarsTable and the ParamWinTable
  // ***


  /* A questo punto di norma non faccio un reset perché non voglio perdere le variabili che sono state selezionate. Se però il refresh ha comportato il cambiamento della variabile dell'asse x (cioè il tempo) allora il reset è necessario
*/
  /* At this point I do not normally reset because I do not want to lose the
   * variables that have been selected. However, if the refresh led to the
   * change of the x-axis variable (i.e. time), then the reset is necessary
  */
  QString varMenutime=ui->varMenuTable->item(0,1)->text();
  QString expectedVarTime=myVarTable->item(1,3)->text();

  // Ora devo resettare la tabella sia se non sto facendo un refresh, sia se le variabili selezionate nella varTable non sono compatibili con quelle caricate. La compatibilità al monmmento è valutata solo sulla variabile tempo, nell'ipotesi che sia posizionata nella prima riga della mvarMenuTable. In particolare::
  //- se la variabile in prima posizione nella varMenuTable (la variabile tempo è uguale a quella in prima posizione della varMenuTable c'è compatibilità
  // c'è compatibilità anche nel caso in cui la variabile della prima riga della varMenuTable comincia per 't', e la variabile ion prima riga della varMenuTable è "t*". Infatti attraverso la funzione computeCommonX() si sceglie una unità comune per la variabile x, che cviene posta pari a "t*" se tutte le variabili in prima posizione delle varMenuTable cominciano per t, e sono quindi interpretate come variabili - tempo.
  if(!refresh || (varMenutime!=expectedVarTime && !(varMenutime[0]=='t' && expectedVarTime=="t*")))
    on_resetTBtn_clicked();
  else{
      //devo eliminare dalle varie varTables eventuali variabili che non sono più presenti nel file rinfrescato. Faccio la verifica sulla base del nome, considerando solo nomi delle varTable che si riferiscono al file corrente, che è quello rinfrescato.
      //Se un nome di una varTable non è presente nella varMenuTable, allora faccio il click sulla cella contenente il nome per togliere quella variabile.

      // I have to delete from the various varTables any variables that are no
      // longer present in the refreshed file. I do the verification based on
      // the name, considering only varTable names that refer to the current
      // file, which is the refreshed one.
      // If a name of a varTable is not present in the varMenuTable, then I
      // click on the cell containing the name to remove that variable.
    QList <QString> varList;
    for (int iVar=0; iVar<ui->varMenuTable->rowCount(); iVar++){
       varList.append(ui->varMenuTable->item(iVar,1)->text());
      }
    for (int tab=0; tab<actualPlotWins; tab++)
      varTable[tab]->filterOutVars(varList);
  }
  ui->refrTBtn->setEnabled(true);
  QElapsedTimer timer2;
  timer2.start();

  if (mySO[selectedFileIdx]->fileType==MAT_Modelica){
     ui->showParTBtn->setVisible(true);
     ParamInfo parInfo=mySO[fileIndex]->giveParamInfo();
     myParamWin->getData(parInfo.names, parInfo.values, parInfo.units,
                           parInfo.description);
  }else{
     ui->showParTBtn->setVisible(false);
     myParamWin->hide();
  }

  paramWinTableBuilt=false;

//  qDebug() << "Fill Param Table took" << timer2.elapsed() << "milliseconds";
//  qDebug() << "LoadFile took" << timer.elapsed() << "milliseconds";

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
/* Function for loading the contents of the files specified in a list.
 * Although very compact, it is advisable to have it separated from the rest of the code
 * because it is called on three occasions: for reading past files
 * through drag & drop, for reading by clicking on the "load ..." button, and
 * in :: CDataSelWin, STEP B.
 * The generation of a function facilitates the maintenance of the code, avoiding
 * that future changes in one of the two cases will not be reported in the other.
*/
  int j;
  QString path, ret;

  if(fileNameList.count()>1 && !ui->multifTBtn->isChecked()){
    ui->multifTBtn->setChecked(true);
    on_multifTBtn_clicked(true);
  }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  foreach(path,fileNameList){
  //In windows, può accadere che il nome del file contenga prima dell'indicatore del disco un invalido carattere '/', ad esempio "/C:/QtSource/PlotXY/aaa.adf"
    //In tal caso devo eliminare il primo carattere:

    // In windows, it may happen that the file name contains an invalid '/'
    // character before the disk indicator, for example "/C:/QtSource/PlotXY/aaa.adf"
    // In this case I have to delete the first character:
    if(path.indexOf(':')!=-1)
      if(path[0]=='/')
         path.remove(0,1);
    // se ho droppato un unico file, sono in single file, ed esiste già un file selezionato rimango in single file e sostituisco il file precedente con quello droppato:

    // if I have dropped a single file, they are in single file, and there is
    // already a selected file remaining in single file and replacing the
    // previous file with the dropped file:
    if(fileNameList.count()==1 && !ui->multifTBtn->isChecked() && selectedFileIdx>-1){
      ret=loadFile(selectedFileIdx,path);  //aggiorna anche numOfSelFiles
                                           // also update numOfSelFiles
      QApplication::restoreOverrideCursor();
      return ret;
    }
    //carico nel primo indice disponibile:
    // load in the first available index:
    for(j=0; j<MAXFILES; j++)
        if(freeFileIndex.contains(j))
            break;
    if(j==MAXFILES)
        break;
    // Se il file da caricare non è oltre il N. 8, ma è oltre il numero di righe correntemente visualizzate aumento la visualizzazione della filetable di una riga:

    // If the file to be loaded is not more than N. 8, but it is beyond the
    // number of rows currently displayed, I increase the display of the filetable of a row:
    if(numOfLoadedFiles>=visibleFileRows){
       int newVSize=ui->fileTable->height()+ui->fileTable->rowHeight(0)+1;
       ui->fileTable->setMaximumHeight(newVSize);
       ui->fileTable->setMinimumHeight(newVSize);
       visibleFileRows++;
       if(visibleFileRows>MAXFILES)
         QMessageBox::critical(this,"CDataSelWin","Critical error N. 1");
    }
    ret=loadFile(j,path,false,false,tShift);  //aggiorna anche numOfSelFiles e fileNumsLst
                                              // also update numOfSelFiles and fileNumsLst
    if(ret!=""){
        QMessageBox::warning(this,"PlotXY-dataSelWin",ret);
        qDebug()<<"warning 2";
        QApplication::restoreOverrideCursor();
        return ret;
    }
    freeFileIndex.remove(j);
    if(ret!="")break;
    if(!GV.multiFileMode) break;
    if(GV.multiFileMode)
        myVarTable->setCommonX(computeCommonX());
  }

  QApplication::restoreOverrideCursor();
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
/* Modified version of loadFileList to do LoadState.
 * Makes some graphics and path operations and then loads the contents of the files into memory
 * whose names are present in the namesList list
 * It should be used when I am restoring the system state, and therefore the indexes of
 * files can not be arbitrary, but must be those that were present during
 * saving (and that have been saved)
 * The correct state of multiFile has already been restored on entering this procedure
*/
  QString pathName, ret, tShift;

  for (int i=0; i<fileNamesList.count(); i++){
    pathName=fileNamesList[i];
    tShift=tShiftLst[i];
  //In windows, può accadere che il nome del file contenga prima dell'indicatore del disco un invalido carattere '/', ad esempio "/C:/QtSource/PlotXY/aaa.adf"
    //In tal caso devo eliminare il primo carattere:

    // In windows, it may happen that the file name contains an invalid '/'
    // character before the disk indicator, for example "/C:/QtSource/PlotXY/aaa.adf"
    // In this case I have to delete the first character:
    if(pathName.indexOf(':')!=-1)
      if(pathName[0]=='/')
         pathName.remove(0,1);
     // Il seguente loadFile ha al suo interno la seguente riga:
    // The following loadFile has the following line inside:

    // fileNumsLst[fileIndex]=fileIndex+1;
    // in cui fileIndex è il primo argomento passato.
    //Questo perché è pensata per quando sto caricando un file ex novo e non ho già il numero presente in memoria.  In questo caso invece sto ripristinando lo stato del sistema il numero è già presente in fileNumsLst, e viene raddoppiato. Pertanto subito dopo il caricamento annullo gli effetti di quella riga.
    //Questo modo di procedere è artificioso ed è conseguenza del fatto che fileNumsLst è gestito come un array e non una vera lista: è inizializzato a 8 elementi all'inizio e un elemento "vuoto" è convenzionalmente rappresentato dalla presenza di un num=0

    // where fileIndex is the first argument passed.
    // That's because it's meant for when I'm loading a file from scratch and I
    // do not already have the number in memory. In this case, however, I am
    // restoring the system state the number is already present in fileNumsLst,
    // and is doubled. Therefore, immediately after loading, the effects of that
    // line are canceled.
    // This way of proceeding is artificial and is a consequence of the fact
    // that fileNumsLst is managed as an array and not a real list: it is
    // initialized to 8 elements at the beginning and an "empty" element is
    // conventionally represented by the presence of a num = 0
    ret=loadFile(fileNumList[i]-1,pathName,false,false,tShift);  //aggiorna anche numOfSelFiles
                                                                 // also update numOfSelFiles
//    fileNumsLst.removeLast();
    // Per ragioni non scandagliata nemmeno la seguente riga va; se la commento, per ragioni al momento non scandagliate va tutto bene.

    // For reasons not fathomed, not even the following line goes;
    // if the comment, for reasons not currently sounded, everything is fine.

//    fileNumsLst[fileNumList[i]-1]=0;

/*
    if(myVarTable->numOfTotVars>1){
      ui->plotTBtn->setEnabled(true);
      ui->saveVarsBtn->setEnabled(true);
    }
*/

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
  // Table height adaptation:
  visibleFileRows=fileNamesList.count();
  if(GV.multiFileMode)
    visibleFileRows=qMax(fileNamesList.count(),3);
  else
    visibleFileRows=fileNamesList.count();
  int newVSize=(visibleFileRows+1)*ui->fileTable->rowHeight(0)+1;
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
  /* Here I manage the resizing of the file. Basically I first adjusted
   * all columns to the content. In this way, the width may be smaller
   * of the available or higher one. In both cases I make the compensation
   * on the file name, which is always viewable in full through the hint
   * of the relative cell.
*/
    ui->fileTable->resizeColumnsToContents();
    int maxNameWidth=this->width();
    //Ora dalla larghezza totale sottraggo quella delle celle che non voglio modificare e il resto lo attribuisco alla cella del nome del file. Notare che le cose sono differenti sia per la differenza singleFile/Multifile, sia per il fatto che in ultima colonna posso avere Tmax o TShift. l'indice columnCount-1 è quello di TShift.

    // Now the total width subtracts that of the cells I do not want to change
    // and the rest I attribute to the cell of the file name. Note that things
    // are different both for the singleFile / Multifile difference, and for
    // the fact that in the last column I can have Tmax or TShift. the
    // columnCount-1 index is that of TShift.
  if(ui->multifTBtn->isChecked()){
    maxNameWidth-=ui->fileTable->columnWidth(0); // "x" cell
    //La seguente riga dovrebbe esserci. Per ragioni sconosciute il programma va meglio se in Win la tolgo
    // The following line should be there. For unknown reasons, the program is better if I remove it in Win
#ifdef Q_OS_MAC
    maxNameWidth-=ui->fileTable->columnWidth(1); // "f" cell
#endif
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
    // In Windows, columns 0 and 1 are too large and I reduce them a bit:
#ifndef Q_OS_MAC
  ui->fileTable->setColumnWidth(0,int(0.4f*ui->fileTable->columnWidth(0)));
  ui->fileTable->setColumnWidth(1,int(0.5f*ui->fileTable->columnWidth(1)));
#endif

  //Gestione della label "more..."
  // Management of the "more..." label
  if (!myVarTable->isVisible())
      return;
  QRegion region=myVarTable->visibleRegion();
  QVector <QRect> rects=region.rects();
  int maxRectHeight=0;
  for (int i=0; i<rects.count(); i++){
    maxRectHeight=max(maxRectHeight,rects[i].height());
  }
  if((myVarTable->givehighestUsedRowIdx()+1)*myVarTable->rowHeight(0)>maxRectHeight+1)
    ui->moreLbl->setVisible(true);
  else
    ui->moreLbl->setVisible(false);
}

void CDataSelWin::setActualPlotWins(int wins){
    QString label;
    switch(wins){
    case 4:
      for (int tab=ui->tabWidget->count()-1; tab>=0; tab--){
        ui->tabWidget->removeTab(tab);
      }
      for (int tab=0; tab<4; tab++){
        QString label;
        label.setNum(tab+1);
        label="Plot "+label;
        ui->tabWidget->insertTab(tab,varTable[tab],label);
      }
      break;
    case 6:
    case 8:
      for (int tab=0; tab<wins; tab++){
        label.setNum(tab+1);
        ui->tabWidget->setTabText(tab,label);
      }
      for (int tab=actualPlotWins; tab<wins; tab++){
        label.setNum(tab+1);
        ui->tabWidget->insertTab(tab,varTable[tab],label);
      }
    }
    actualPlotWins=wins;
}

void CDataSelWin::showEvent(QShowEvent *){
  visibleFileRows=3;
  int tableHeight=(visibleFileRows+1)*ui->fileTable->rowHeight(0);
  ui->fileTable->setMaximumHeight(tableHeight);
  ui->fileTable->setMinimumHeight(tableHeight);

  // La posizione singleFile/MultiFile è quella default, se non ho settato GV.WinPosAndSize. Altrimenti va messo il valore settato da GV.WinPosAndSize. Se però sono passati più files vado comunque in multileMode
  //Il seguente if è particolarmente articolato in quanto garantisce che la posizione del multifile sia corretta a prescindere da come l'ho lasciato in Qt Designer

  // The singleFile / MultiFile position is the default if I have not set
  // GV.WinPosAndSize. Otherwise set the value set by GV.WinPosAndSize.
  // If, however, more files have been passed, I still go to multileMode
  // The following if is particularly complex as it ensures that the
  // position of the multifile is correct regardless of how I left it in Qt Designer
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

  resizeEvent(nullptr);
  // Per la spiegazione della seguente riga vedere il commento sotto FRAMEGEOMETRY_COMMENT
  // For the explanation of the following line see the comment below FRAMEGEOMETRY_COMMENT
  // *** Starting from Qt 5.9, the following row, for unknown reasons, stopped working!
  //     This is not fixed yet; however it creates minor problems to users
  if(!GV.PO.rememberWinPosSize)
    on_arrTBtn_clicked();
}

QPoint CDataSelWin::toInPrimaryScreen(QPoint inPoint, int pixelMargin){
    /* serve ad assicurarsi che un punto sia all'interno della zona visibile dello schermo. Anzi, esso viene posizionato all'interno di tale zona di una quantità di pixel pari a pixelMargin. Se ad esempio inPoint è pos() di una finestra esso verrà posizionato un po' dentro lo schermo in modo che l'utente possa vedere almeno un angolo della finestra */

    /* is used to make sure that a point is within the visible area of ​​the screen.
     * On the contrary, it is positioned within this zone by a quantity of pixels
     * equal to pixelMargin. For example, if inPoint is pos () of a window it will
     * be placed a little bit inside the screen so that the user can see at least
     * one corner of the window */
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
  /* This function is a slot that is automatically executed
   * after the varTable has changed.
   * Manages the activation of the buttons at the bottom of the varTable itself.
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
  /*Seleziona il file presente nella riga passata row */
  QString str;
  //Se non c'è alcun file nella riga selezionata non faccio nulla:
  // If there is no file in the selected row I do nothing:
  str=ui->fileTable->item(row,1)->text();
  if(str=="")return;
  ui->fileTable->item(selectedFileRow,0)->setText("");
  if(selectedFileIdx<0){
    QMessageBox::critical(this,"CDataSelWin","critical error 1");
    QCoreApplication::exit(0);
  }
  //memorizzo l'indice della prima variabile visualizzata del file finora visualizzato:
  // I save the index of the first displayed variable of the file viewed so far:
  topIndex[selectedFileIdx]=ui->varMenuTable->verticalScrollBar()->value();

  selectedFileIdx=ui->fileTable->item(row,1)->text().toInt()-1;
  str=ui->fileTable->item(row,1)->text();
  ui->fileTable->item(row,0)->setText("x");

  selectedFileRow=row;

  //Quando si commuta il file i nomi delle variabili nelle funzioni di variabile di tutte le schede di varMenuTable devono essere convertiti in nomi completi per maggior chiarezza

  // When you switch the file the variable names in the variable functions of
  // all the varMenuTable cards must be converted to full names for greater clarity
  for (int tab=0; tab<MAXPLOTWINS; tab++)
    varTable[tab]->fillFunNames();

  //Aggiorno varMenuTable
  // Update varMenuTable
  fillVarMenuTable(selectedFileIdx);

/* Scopo della seguente riga è ripristinare la posizione della varMenuTable.
E' stato verificato in un progetto-test a sé stante che questo comando ha l'effetto desiderato. (cartella "ProgramScrollTable").
E' stato inoltre verificato qui che riceve l'input giusto, ma per una ragione non chiarita è come se non venisse emesso! La lista infatti parte sempre dal primo valore (indice 0).*/

/* The purpose of the following line is to reset the position of the varMenuTable.
It has been verified in a separate test-project that this command has the desired effect.
("ProgramScrollTable" folder). It has also been verified here that it receives the
right input, but for an unexplained reason it is as if it were not issued!
The list always starts from the first value (index 0). */
  ui->varMenuTable->verticalScrollBar()->setValue(topIndex[selectedFileIdx]);


  if (mySO[selectedFileIdx]->fileType==MAT_Modelica){
    ui->showParTBtn->setVisible(true);
    ui->showParTBtn->setChecked(false);
    ParamInfo parInfo=mySO[selectedFileIdx]->giveParamInfo();
    myParamWin->getData(parInfo.names, parInfo.values, parInfo.units, parInfo.description);
  }else{
    ui->showParTBtn->setVisible(false);
    myParamWin->hide();
  }
}


void CDataSelWin::on_tabWidget_currentChanged(int index){
  /* The following if is needed since when I construct DataSelWin I remove from the
   * tabWidget all the tabs which are then put back. When I remove the last one here
   * one enters with index equal to -1!
*/
    if (index<0)
      return;
    currentTableIndex=index;

    myVarTable=varTable[index];
    myPlotWin=plotWin[index];
    myFourWin=fourWin[index];

    myPlotWin->raise();
    myPlotWin->setFocus();
    myVarTable->setMultiFile(GV.multiFileMode);
    myVarTable->setCurrFile(selectedFileIdx);
    if(!fileLoaded)
      return;

    // Select time variable:
    if(myVarTable->xInfo.idx==-1 && GV.multiFileMode){
       myVarTable->setCommonX(computeCommonX());
    }else{
      if(ui->varMenuTable->rowCount()>0 && myVarTable->isEmpty())
//        if(myVarTable->xInfo.idx==-1)
         varMenuTable_cellClicked(0,1,false);
    }
    // manage the activation of the various buttons:
    varTableChanged();
}


void CDataSelWin::on_resetTBtn_clicked()
{
  myVarTable->myReset();
  if(GV.multiFileMode && fileLoaded)
    myVarTable->setCommonX(computeCommonX());
  else
    varMenuTable_cellClicked(0,1,false);
   ui->moreLbl->setVisible(false);
}


void CDataSelWin::varMenuTable_cellClicked(int row, int column, bool rightBtn)
{
  /* slot di cellClicked di varMenuTable
Quando ho cliccato su varMenuTable, in DataSelWin processo il comando e mando i dati a myVarTable
Da quando (lug 2015) la varMenu è stata realizzata in due colonne, per consentire un agevole sort, il click deve avvenire sempre sulla colonna di indice 1, e quindi se invece la colonna è 0 non faccio niente.
L'indice da passare a setVar è il numero presente nella prima colonna in corrispondenza della riga nella quale si è cliccato.
*/
  /* varMenuTable cellClicked slot
When I clicked on varMenuTable, in the DataSelWin process the command and send the data to myVarTable
Since (July 2015) the varMenu has been realized in two columns, to allow an easy sort, the click must
always take place on the index column 1, and therefore if instead the column is 0 I do nothing.
The index to pass to setVar is the number in the first column corresponding to the row in which it was clicked.
*/
   if(column==0)return;
   //se sono in multifile e si è selezionata la var. "t" (di riga 0) non faccio nulla:

   // if they are in multiFileMode and the var has been selected. "t" (in line 0) I do nothing:
   // if(row==0 && GV.multiFileMode)return;
  QString varName=ui->varMenuTable->item(row,column)->text();
  QString fileName=ui->fileTable->item(selectedFileRow,2)->text();

  bool bVar=ui->varMenuTable->monotonic[row];
  int varNum=ui->varMenuTable->item(row,0)->text().toInt();
  if(mySO[selectedFileIdx]->fileType==MAT_Modelica){
    // In questo caso l'unità di misura si trova nella lista sVars di strutture SVar del file mySO corrente.
    //L'elemento giusto di sVars si trova seguendo l'ordine sequenziale: le variabili selezionate e visualizzate in fileMenuTable seguendo l'ordine delle variabili presenti in mySO[selectedFileIdx].

    // In this case, the unit of measure is in the sVars list of SVar structures
    // of the current mySO file.
    // The right element of sVars is found following the sequential order: the
    // variables selected and displayed in fileMenuTable following the order of
    // the variables in mySO [selectedFileIdx].
    QString myUnit=mySO[selectedFileIdx]->sVars[row].unit;
    myVarTable->setVar(varName,varNum,selectedFileIdx+1,rightBtn, bVar,myUnit);
  }else
    myVarTable->setVar(varName,varNum,selectedFileIdx+1,rightBtn, bVar,"");
  //La seguente chiamata serve per l'aggiornamento di moreLbl:
  // The following call is needed to update moreLbl:
  resizeEvent(nullptr);

}


void CDataSelWin::on_plotTBtn_clicked() {
    /* Funzione per l'esecuzione del plot.
- fase 0: analizzo fileTable e compilo timeShift
- fase 1: analizzo varTable
- fase 2: creo le matrici x1 e y1 (la loro struttura è descritta in developer.ods)
- fase 3: calcolo gli elementi di y1 connessi alle funzioni di variabili
*/
   /* Function for executing the plot.
- phase 0: analyze fileTable and compile timeShift
- phase 1: analyze varTable
- phase 2: create the matrices x1 and y1 (their structure is described in developer.ods)
- phase 3: calculate the elements of y1 connected to the functions of variables
*/

  int  iFile, iVar, iFileNew;
//  int lastIFile; //l'ultimo valore di iFile su cui si è scritto (utile per debug)
// int lastIFile; // the last value of iFile on which it was written (useful for debugging)
  float **x1, ***y1; //puntatori a vettori e matrici delle delle variabili selezionate.
                     // pointers to vectors and matrices of the selected variables.
  SFileInfo myFileInfo;
  QList <SFileInfo> filesInfo; //informazioni relative ai files di cui sono richiesti plot. Contiene sia le informazioni relative ai plot diretti da dati di input (uno per file) che a funzioni di variabili (uno per funzione). Pertanto il numero di elementi che contiene è pari a numOfTotPlotFiles

        // information related to the files which requested plot; it contains

        // information related to thet plots generated from input data (one per file)
        // and to variable functions (one per function).
        // Therefore the number of elements it contains is equal to numOfTotPlotFiles

  QList <SCurveParam> y1Info[MAXFILES+MAXFUNPLOTS];
  QList <SXYNameData> funInfoLst; //una  voce della lista per ogni funzione di variabile
                               // a list item for each variable function
  CLineCalc myLineCalc;
  QString ret;
  float timeShift[MAXFILES];

 // fase 0: analizzo fileTable e compilo timeShift
  //Associo il tShift al numero di file:

  // step 0: I analyze fileTable and compile timeShift
  // Associate the tShift to the file number:
  for (int i=0; i<MAXFILES; i++)
      timeShift[i]=0;
  for (int iRow=1; iRow<ui->fileTable->rowCount(); iRow++){
    bool ok;  //diventa true se la conversione in int del testo della cella fatta qui sotto ha successo
              // becomes true if the conversion to int of the text of the cell below is successful
    int myIFile=-1;
    myIFile=ui->fileTable->item(iRow,1)->text().toInt(&ok)-1;
    QString str6=ui->fileTable->item(iRow,6)->text();
    if(ok)
      timeShift[myIFile]=ui->fileTable->item(iRow,6)->text().toFloat();
  }

 // - fase 1: analizzo varTable
 // - phase 1: analyze varTable
  myVarTable->analyse();
  if(myVarTable->numOfPlotFiles>MAXFILES){
    QMessageBox::critical(this,"CDataSelWin", "Internal critical error\ncontact program maintenance",QMessageBox::Ok);
    return;
  }
  /* - fase 2: creo le matrici x1 e y1 (la loro struttura è descritta in developer.ods)   *
   * A questo punto la table è in grado di fornirmi i dati da inviare alla scheda del plot
   *  per il grafico.
   * Devo ora creare le matrici delle variabili x1[] e y1[] (il digit '1' è lasciato solo
   * per compatibilità terminologica con la versione BCB).
   * Ogni elemento di y1 è una matrice. I primi numOfPlotFiles sono dedicati ai dati di
   * variabili direttamente prelevate dai files di input; gli ultimi funinfo.count() sono
   * dedicati alle variabili funzione.
   * Cosa analoga vale per le righe di x1[].
*/
  /* - phase 2: create arrays x1 and y1 (their structure is described in developer.ods) *
   * At this point the table is able to provide the data to be sent to the plot sheet
   * for the chart.
   * Now I have to create the arrays of variables x1 [] and y1 [] (digit '1' is left alone
   * for terminological compatibility with the BCB version).
   * Each element of y1 is a matrix. The first numOfPlotFiles are dedicated to data
   * variables directly taken from the input files; the last funinfo.count () are
   * dedicated to function variables.
   * The same applies to the lines of x1 [].
*/
  funInfoLst=myVarTable->giveFunInfo();
  x1=new float*[myVarTable->numOfPlotFiles+funInfoLst.count()];
  y1=new float**[myVarTable->numOfPlotFiles+funInfoLst.count()];

  /* Attribuzione alle matrici dei rispettivi valori. */
  //Nella copiatura su Y devo anche rendere contigui gli indici di files che possono essere sparpagliati:

  /* Attribution to the matrices of the respective values. */
  // When copying to Y I also have to make the indexes of files that can be scattered contiguous:
  filesInfo.clear();
  iFileNew=-1;
  for(iFile=0; iFile<MAXFILES; iFile++){
    if(myVarTable->yInfo[iFile].count()==0)
        continue;
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

      for(int iCurve=0; iCurve<y1Info[iFileNew].count(); iCurve++){
        QString thisUnit=mySO[iFile]->sVars[y1Info[iFileNew][iCurve].idx].unit;
//        int sVarIndex=y1Info[iFileNew][iCurve].idx;
        y1Info[iFileNew][iCurve].unitS=mySO[iFile]->sVars[y1Info[iFileNew][iCurve].idx].unit;
      }
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

  /* STEP 3 **** Now I add the calculation of the elements of y1 connected to the variable functions. ****/
  // In the case where myVarTable.xInfo.isFunction = true one of the fun of the
  // following loop will be put as a vector of the x instead of a single row in y.
  for(int iFun=0; iFun<funInfoLst.count(); iFun++){
    SXYNameData funInfo=funInfoLst[iFun];
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
    /* The management of variable functions is particularly complex if you do not have one
     * the guarantee that all the files involved have data in correspondence
     * of the same values ​​as the time variable.
     * This type of information is provided by the PlotXY user through
     * the checkbox in the CFunStrInput dialog.
     * Currently (September 2015), this checkbox is disabled and the functions of
     * variables with data from different files is always performed without
     * "cross-interpolation". It is therefore assumed that the files involved have
     * samples all on the same instant, and the only verification that is done is that the
     * number of values ​​present in the various files is identical, which constitutes
     * "without which it could not be" the functions of variables without cross-interpolation
     * can be calculated.
*/
  //  Come prima cosa devo verificare che tutti i file coinvolti nella funzione di variabili possiedono il medesimo numero di punti; altrimenti emetto un messaggio di errore

    // First of all I have to verify that all the files involved in the variable
    // function have the same number of points; otherwise, I issue an error message
    int points=mySO[funInfo.fileNums[0]-1]->numOfPoints;
    for (int fileInFun=1; fileInFun<funInfo.fileNums.count(); fileInFun++){
      if(mySO[funInfo.fileNums[fileInFun]-1]->numOfPoints!=points){
          QMessageBox::warning(this,"Invalid function",
             "Unable to plot this function:\n"
             "currently only functions operating on variables\n"
             "coming from files having the same number of points are allowed.");
            return;
        }
      }

    QVector<int> funFileIdx;
    for (int var=0; var<funInfo.varNumsLst.count(); var++ )
      funFileIdx.append(funInfo.varNumsLst[var].fileNum-1);

    /* Le matrici y1[i] e i vettori x[i] con i a partire da plotFiles, vanno allocati (dettagli in Developer.odt):*/
    /* Arrays y1 [i] and vectors x [i] with i starting from plotFiles must be allocated (details in Developer.odt): */
    y1[plotFiles+iFun]=CreateFMatrix(1,points);
    x1[plotFiles+iFun]=new float[points];
    //Passo la linea a myLineCalc():
    myLineCalc.getFileInfo(fileNumsLst, fileNamesLst, varMaxNumsLst);
    ret=myLineCalc.getLine(funInfo.lineInt,selectedFileIdx+1);

    /*  In LineCalc devo avere dei puntatori ai vettori dei valori fra loro adiacenti.
     * Creo pertanto varMatrix che è un puntatore che punta ad un array di puntatori
     * contigui ai dati da utilizzare per il calcolo della funzione.
     * In namesMatrix invece metto i corrispondenti nomi delle variabili.
    */
    /* In LineCalc I must have pointers to the vectors of adjacent values.
     * I therefore create varMatrix which is a pointer pointing to an array of pointers
     * contiguous to the data to be used for the calculation of the function.
     * In namesMatrix instead I put the corresponding variable names.
    */
    float**varMatrix= new float*[funInfo.varNumsLst.count()];
    QList <QString *> namesFullList;
    int size=int(sizeof(float))*points;
    //Creo i valori di x1, cioè sull'asse x delle variabili funzione, nel caso in cui proprio sull'asse x non vi sia una funzione:

    // I create the values ​​of x1, that is, on the x axis of the function variables,
    // in the case where there is no function on the x axis:
    if(!myVarTable->xInfo.isFunction){
      memcpy(x1[plotFiles+iFun],mySO[funFileIdx[0]]->y[myVarTable->xInfo.idx], size);
      // Qui va messa la gestione del timeshift nel caso di funzione di variabili. Occorrerà comporre la x1 considerando differenti time shifts per i vari files che compongono la funzione. Pertanto il seguente for non è sufficiente in quanto traslerebbe uniformemente la funzione e non selettivamente gli assi x dei vari files presenti nella funzione:

      // Here the timeshift management must be set in the case of a variable function.
      // It will be necessary to compose the x1 considering different time shifts for
      // the various files that make up the function. Therefore the following for is
      // not sufficient because it would uniformly translate the function and not
      // selectively the x-axis of the various files present in the function:

//      for(int iPoint=0; iPoint<points; iPoint++)
//          x1[plotFiles+i][iPoint]+=timeShift[funFileIdx];
    }

    for(int j=0; j<funInfo.varNumsLst.count(); j++){
      int soIndex=funInfo.varNumsLst[j].fileNum-1;
      int yIndex=funInfo.varNumsLst[j].varNum-1;
       varMatrix[j]=mySO[funInfo.varNumsLst[j].fileNum-1]->y[funInfo.varNumsLst[j].varNum-1];

       funInfo.varUnits.append(mySO[soIndex]->sVars[yIndex].unit);
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
   /* I pass the names and the pointer to the array containing the vectors in the various lines
    * of the variables to be used for the calculation of the function
    * The values ​​of pointers to function variables remain allocated in the program
    * caller.
    * lineCalc analyzes the line by replacing the explicit constants with corresponding ones
    * pointers and references to variable names with the corresponding pointers
    * passed as a second parameter
    * The vector of explicit names mySO [] -> varNames serves only to compile
    * complete information on the meaning of the function string to be displayed
    * then to the user. The function line with explicit names is called
    * "fullLine"
   */

    // La seguente riga manda in esecuzione indirettamente variablesToPointers.
    // Se il plot avviene dopo un cambiamento di file, la stringa della funzione è stata arricchita delle indicazioni dei files, ma la line interna è rimasta senza i nomi di file e quindi variablesToPointers viene eseguita con la stringa sbagliata: ad esempio con v2+v3 invece della corretta f2v2+f2v3.

    // The following line indirectly runs variablesToPointers.
    // If the plot occurs after a file change, the function string has been
    // enriched with the file indications, but the internal line has been left
    // without the file names and therefore variablesToPointers is executed
    // with the wrong string: for example with v2 + v3 instead of the correct f2v2 + f2v3.

    // ############
    // NOTA IMPORTANTE
    // Ora che le funzioni di variabile ammettono di mescolare dati da differenti files, occorre cambiare la seguente chiamata a funzione. Invece di passare solo i varNames di un unico mySO, e il solo selectedFileIdx, devo passare un array di puntatori ad arrays di stringhe, ed un vettore di indici di file. Questo è un prerequisito per evitare il crash di f1v2+f2v3 nel TODO.
    // ############

    // ############
    // IMPORTANT NOTE
    // Now that the variable functions admit to mixing data from different files,
    // the following function call must be changed. Instead of passing only the
    // varNames of a single mySO, and the only selectedFileIdx, I have to pass
    // an array of pointers to string arrays, and a file index vector. This is
    // a prerequisite to avoiding the crash of f1v2 + f2v3 in the TODO.
    // ############

     ret=myLineCalc.getNamesAndMatrix(funInfo.varNames, funInfo.varUnits, varMatrix,
                                                   namesFullList, selectedFileIdx);
    if(ret!=""){
        QMessageBox::critical(this, "PlotXY",ret);
        return;
    }
    //Per soli scopi di visualizzazione per l'utente finale passo, tramite doppia indirezione per ragioni di efficienza, l'array di array dei nomi espliciti di tutte le variabili presenti nei files caricati:

    // For visualization purposes only for the end user, by means of a double
    // direction for efficiency reasons, the array of arrays of the explicit
    // names of all the variables present in the loaded files:

    //Ora effettuo il calcolo della funzione. Il file è data.fileNums[0], ma con indice a base 1.
    //Se una funzione deve divenire la variabile x, essa la calcolo nel vettore ausiliario funXVar che devo allocare.

    // Now I calculate the function. The file is data.fileNums [0], but with a base-based index of 1.
    // If a function is to become the variable x, it is the calculation in the auxiliary vector funXVar that I have to allocate.
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

                     // Also the var. on the x axis it can be a function.
                     // If I have already discussed the possible function on the x axis I put funXDone to true.
    if(myVarTable->xInfo.isFunction && iFun>myVarTable->xInfo.idx)
      funXDone=1;
    for(int k=0; k<points; k++){
      float xxx;
        xxx=myLineCalc.compute(k);
      if(myLineCalc.divisionByZero|| myLineCalc.domainError){
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
          if(myLineCalc.divisionByZero)
            msg= "Division by zero in function of variable; plot impossible.\n"
               "Offending operation at "+sampleIndex+ " sample.\n"
               "The horizontal variable (possibly time) value is " + timeValue+ "\n";
          if(myLineCalc.domainError)
              msg= "Domain error in sqrt() in function plot; plot impossible.\n"
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
       * 2) L'integrale è inteso come integrale del tempo. Nel caso di multifile
       *    l'integrale è fatto rispetto al tempo relativo alla variabile considerata;
       *    nel caso di plotXY devo chiarire che il tempo è la variabile di indice 0
       *    del file corrente.
      */
    /* To make the integral correctly, the following considerations are valid:
           * 1) if the variable x is a function for it, integration is not allowed
           * 2) Integral is intended as an integral of time. In the case of multifile,
           *    the integral is made with respect to the time relating to the variable
           *    considered; in the case of plotXY I have to clarify that the time is
           *    the index variable 0 of the current file.
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
                            // Remember that y1Info for functions has a single element for function "i"
    param.isMonotonic=false;
    param.name=funInfo.name;
    param.midName=funInfo.line;
    param.fullName=myLineCalc.giveLine("lineFullNames");
    param.color=funInfo.color;
    param.style=funInfo.style;

    param.rightScale=funInfo.rightScale;
//    param.unitS=myLineCalc.computeUnits();
    param.unitS=myLineCalc.unitOfMeasuref();
    if(myLineCalc.integralRequest)
       param.unitS=integrateUnits(param.unitS);
    y1Info[plotFiles+iFun].append(param);
    myFileInfo.name=myLineCalc.giveLine("funText");
    myFileInfo.fileNum=plotFiles+iFun;
    myFileInfo.numOfPoints=points;
    myFileInfo.variableStep=true;
    // Nel caso di funzioni di variabili faccio per default diagrammi di linea e quindi metto falso a frequency scan:
    // In the case of variable functions, I do line diagrams by default and then put a false frequency scan:
    myFileInfo.frequencyScan=false;
    /*  Comment on 4 December 2019. Timeshift of function plots has not been yet programmed.
     * for the time being it is always set to zero:
     *
    */
    myFileInfo.timeShift=0.0f;
    filesInfo.append(myFileInfo);
  }
  //Ora, nel caso in cui myVarTable.xInfo.isFunction==true devo ricopiare funXVar in x1
  // Now, in case myVarTable.xInfo.isFunction == true I have to copy funXVar to x1
  if(myVarTable->xInfo.isFunction){
    for(int i=0; i<1+funInfoLst.count()-1; i++){
    //Il primo 1 nel target è numOfPlotFiles che deve essere 1 (quando facciamo plot X-Y tutti i grafici devono provenire dal medesimo file); il secondo, preceduto da segno '-', è necessario perché dobbiamo diminuire di 1 funInfo.count() in quanto funInfo contiene anche dati della funzione che deve andare sull'asse x

    // The first 1 in the target is numOfPlotFiles must be 1 (when we make an X-Y plot all plots must refer to data coming from the same file); the second, preceded  by a '-' sign, is necessary because we have to decrease by 1 funInfo.count () because funInfo also contains data of the function that must go on the x axis
        x1[i]=funXVar;
//      memcpy(x1[i],funXVar, sizeof(float)*mySO[0]->numOfPoints);
    }
  }
/*********** Software start for debugging **************
    float x_dbg0[3], y_dbg0[3][3];
    int
        var_dbg=min(3,myVarTable->yInfo[lastIFile].count()),
        pt_dbg=min(3,mySO[lastIFile]->numOfPoints);
    for(int iPt=0; iPt<pt_dbg; iPt++)
       for(iVar=0; iVar<var_dbg; iVar++){
           x_dbg0[iPt]=x1[iFileNew][iPt];
           y_dbg0[iVar][iPt]=y1[iFileNew][iVar][iPt];
       }
***********  fine Software per debug *************** */


    /*Trasmetto le informazioni alla finestra di plot, che farà una copia locale delle intere matrici x1 e y1.
    */
    /* I pass the information to the plot window, which will make a local copy of the entire arrays x1 and y1.
    */

    //Si ricordi che lo show comporta un resize della finestra e quindi, attraverso la ui, anche del lineChart gestito dal designer all'interno della finestra. Pertanto il plot() deve seguire lo show().

  // Remember that the show involves a resize of the window and therefore,
  // through the ui, also of the lineChart managed by the designer inside
  // the window. Therefore the plot () must follow the show ().
  myPlotWin->getData(x1, y1, myVarTable->xInfo, y1Info, filesInfo);

  /* Le matrici y1[i] e i vettori x[i], con i a partire da plotFiles, erano stati allocati più sopra e quindi qui disalloco, ora che myPlotWin ne ha fatto copia locale:*/

  /* The matrices y1 [i] and the vectors x [i], with i starting from plotFiles,
   * had been allocated above and therefore here disallocated, now that
   * myPlotWin has made a local copy of it: */
  for(int i=0; i<funInfoLst.count(); i++){
    DeleteFMatrix(y1[plotFiles+i]);
    delete[] x1[plotFiles+i];
  }

  //Il seguente close serve per  fare un reset del bottone "data". Altrimenti può accadere che se il data cursor è visibile, al nuovo plot i numeri sono visualizzati nell'opzione vecchia: ad esempio se ho ridotto il numero di variabili da n a 1 rimane la finestra esterna e viceversa.

  // The following close is used to reset the "date" button. Otherwise it can
  // happen that if the data cursor is visible, to the new plot the numbers
  // are displayed in the old option: for example if I have reduced the number
  // of variables from n to 1 the external window remains and vice versa.
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

  // The value of checked is that already after the effect of the click.
  // For example, if I was in multifile and clicked, here I will find checked = false.
  if(!checked){
    //Vado in singleFile
    // I go in singleFile
    GV.multiFileMode=false;
    goneToSingleFile=true;
    // Il tooltip del nome deve essere quello del file selezionato, non quello della riga dove c'era, in multifile, la x. Faccio quindi lo swap dei tooltip:

    // The tooltip of the name must be that of the selected file, not that of the line where
    // there was, in multifile, the x. I then swap the tooltips:
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
    /*  IMPORTANT NOTE
     * Since the appearance of Windows 10 the table header line has become
     * too light and I could not find a system to make it gray.
     * I therefore decided to use the first useful table row as a row of
     * heading.
     * The data of the files are therefore starting from line 1. In addition, the passage in
     * Multifile I did it by reducing the height of the table and making the text swap.
     * Was not it better to use a "setRowHidden" command?
    */

    int tableHeight=ui->fileTable->rowHeight(0)+ui->fileTable->rowHeight(1)+2;
    ui->fileTable->setMinimumHeight(tableHeight);
    ui->fileTable->setMaximumHeight(tableHeight);
    ui->multifTBtn->setToolTip(tr("Go to multi-file mode"));
    /* devo salvare il contenuto della riga di indice 1 e copiare il contenuto della riga del file selezionato in riga di indice 1:*/
    /* I have to save the contents of the index line 1 and copy the contents of the row of the selected file to index line 1: */
    if(selectedFileIdx>-1 && selectedFileRow!=1)
      for(int icol=0; icol<ui->fileTable->columnCount(); icol++){
        saveStrings[icol]= ui->fileTable->item(1,icol)->text();
        ui->fileTable->item(1,icol)->setText(ui->fileTable->item(selectedFileRow,icol)->text());
      }
    // rendo hide tutte le righe eccetto le prime due
    // I render hide all lines except the first two
    for(int i=2; i<ui->fileTable->rowCount(); i++){
       ui->fileTable->hideRow(i);
    }
  }else{
    //Vado in multiFile
    // I go to multiFile
    GV.multiFileMode=true;
    if(goneToSingleFile&&selectedFileRow>-1){
      // Il tooltip del nome deve essere quello del file selezionato, non quello della riga dove c'era, in multifile, la x. Può esserci stato infatti un aggiornamento in singlefile del file caricato. Faccio quindi lo swap dei tooltip:

      // The tooltip of the name must be that of the selected file, not that of the line where
      // there was, in multifile, the x. In fact there may have been a singlefile update of the
      // uploaded file. I then swap the tooltips:
      QString strDummy=ui->fileTable->item(selectedFileRow,2)->toolTip();
      ui->fileTable->item(selectedFileRow,2)->setToolTip(ui->fileTable->item(1,2)->toolTip());
      ui->fileTable->item(1,2)->setToolTip(strDummy);
      //Se in singlefile è stato aggiornato un file, il suo numero è stato correttamente messo in selectedfileIdx, ma non nella tabella. Lo metto ora:

      // If a file has been updated in singlefile, its number has been correctly put in
      // selectedfileIdx, but not in the table. I put it now:
      strDummy.setNum(selectedFileIdx+1);
      ui->fileTable->item(selectedFileRow,0)->setText(strDummy);
    }
    ui->fileTable->setColumnHidden(0,false);
    ui->fileTable->setColumnHidden(1,false);
#ifndef Q_OS_MAC
    ui->fileTable->resizeColumnsToContents();
    ui->fileTable->setColumnWidth(0,int(0.8*ui->fileTable->columnWidth(0)));
    ui->fileTable->setColumnWidth(1,int(0.8*ui->fileTable->columnWidth(1)));
#endif
    if(numOfLoadedFiles==1){
      ui->fileTable->item(1,0)->setText("x");
      ui->fileTable->item(1,1)->setText("1");
    }

    // Visualizzo tutte le righe dotate di file, con un minimo di 3 più l'intestazione:
    // I see all rows with files, with a minimum of 3 plus the header:
    visibleFileRows=max(3,numOfLoadedFiles);
    int tableHeight=(visibleFileRows+1)*ui->fileTable->rowHeight(0)+2;
    ui->fileTable->setMaximumHeight(tableHeight);
    ui->fileTable->setMinimumHeight(tableHeight);
    ui->multifTBtn->setToolTip(tr("Go to single-file mode"));
    // Rimetto nella riga di indice selectedFileRow (l'inidice 0 indica la riga di intestazione della tabella) il contenuto salvato precedentemente (o stringhe vuote se non era stato salvato nulla):

    // Return to the selectedFileRow index row (the inidic 0 indicates the table header row)
    // the previously saved content (or empty strings if nothing had been saved):
    if(goneToSingleFile && selectedFileRow!=1){
      for(int icol=0; icol<ui->fileTable->columnCount(); icol++){
        if(selectedFileIdx<0)
            break;
        ui->fileTable->item(selectedFileRow,icol)->setText(ui->fileTable->item(1,icol)->text());
        ui->fileTable->item(1,icol)->setText(saveStrings[icol]);
      }
    }
    // tolgo hide a tutte le righe che contengono files, con un minimo di 3
    // I remove hide to all lines containing files, with a minimum of 3
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

  // When I switch between multifile and singlefile, I resize the file table by hiding
  // or showing columns. So I have to enable custom resizing with the following resizeEvent:
  QResizeEvent * ev=nullptr;
  resizeEvent(ev);

  myVarTable->setMultiFile(GV.multiFileMode);
  if(GV.multiFileMode && fileLoaded){
   //Il commonX non lo devo solo passare alla tabella corrente ma a tutte. Infatti se  parto da single file,  carico un file, e passo in multifile, resterebbe nelle altre tabellle "1! nelle colonna "f" invece di "a".

   // The commonX should not just pass it to the current table but to all.
   // In fact, if I start from single file, load a file, and pass in multifile,
   // it would remain in the other tabels "1! In the column" f "instead of" a ".
  QString str=computeCommonX();
  for (int tab=0; tab<MAXPLOTWINS; tab++){
    varTable[tab]->setMultiFile(true);
    varTable[tab]->setCommonX(str);
  }
  }else{
    /* qui occorre valutare quando cliccare sulla variabile tempo per selezionarla
     * Non basta verificare che il menù delle variabili non sia vuoto. Occorre
     * anche che il tempo non sia già stato selezionato. Se ad es. è stato
     * fatto un loadFile il tempo è già seleizionato Se invece arrivo qui perché
     * sto commutando da multifile esso non  selezionato. Maglio quindi chiedere
     * direttamene a myVarTable.
    */
    /* here it is necessary to evaluate when to click on the time variable to select it
     * It is not enough to verify that the variable menu is not empty. It must
     * also that the time has not already been selected. If for example has been
     * made a loadFile the time is already selected If instead I get here why
     * I am switching from multifile it not selected. Mallet then ask
     * directly to myVarTable.
    */
    if(ui->varMenuTable->rowCount()>0 && myVarTable->isEmpty())
      varMenuTable_cellClicked(0,1,false);
  }
  ui->plotTBtn->setEnabled(false);
}

void CDataSelWin::on_fileTable_clicked(const QModelIndex &index)
{
  /* Nel caso di click semplice su riga non di intestazione si seleziona il file di cui
   * visualizzare le variabili.
   * Se il click semplice è sull'ultima colonna si fa il toggle fra Tmax e Tshift
   * Occorre inoltre considerare l'eventualità che il click semplice sia semplicemente
   * il primo click di un doppio click.
  */
    /* In the case of a simple click on a non-header line, select the file referred to
     * view the variables.
     * If the simple click is on the last column the toggle is made between Tmax and Tshift
     * It is also necessary to consider the possibility that the simple click is simply
     * the first click of a double click.
    */

  int i=index.row(),j=index.column();

  //se il click è fatto nella colonna Tmax o Tshift, non in prima riga esco: altrimenti farei la commutazione o la deselezione del file corrente

  // if the click is done in the Tmax or Tshift column, not in the first line I go out:
  // otherwise I would switch or deselect the current file
  if(i>0 && j>4)
        return;
  if(doubleClicking){
    doubleClicking=false;
    return;
  }

  if(i==0){  //ora gestisco la commutazione Tshift-Tmax
             // now I manage Tshift-Tmax switching
     if (j==5){  //passaggio da Tmax a Tshift
                 // transition from Tmax to Tshift
        ui->fileTable->hideColumn(5);
        ui->fileTable->showColumn(6);
        ui->fileTable->resizeColumnsToContents();
     }else if (j==6){
        ui->fileTable->hideColumn(6);
        ui->fileTable->showColumn(5);
        ui->fileTable->resizeColumnsToContents();
      }  else
        return; //se click su prima riga ma non ultima colonna esco
                // if you click on the first line but not the last column I go out
  }
  if(i==0){
    resizeEvent(nullptr);
    return;
  }
  selectFile(index.row());
  resizeEvent(nullptr);
  //    ui->varMenuTable->resizeColumnsToContents();
}

void CDataSelWin::on_fileTable_doubleClicked(const QModelIndex &index){
    /*  Quando c'è un double click il file viene rimosso
     *
   */
    /* When there is a double click, the file is removed
     *
    */

    int i=index.row(),j=index.column();

    //se il doppio click è fatto nella colonna Tmax o Tshift, non in prima riga esco: altrimenti farei la deselezione del file corrente

    // if the double click is done in the Tmax or Tshift column, not in the
    // first line I go out: otherwise I would deselect the current file
    if(i>0 && j>4)
          return;
    //Alla prima riga il double-click non deve vare nulla:
    // On the first line the double-click must not vanish:
    if(i==0)
        return;
  //devo comandare la rimozione solo se nella riga dove ho cliccato esiste realmente un file. Lo verifico attraverso la presenza di un qualsiasi contenuto nella FILENUMCOL

  // I have to command the removal only if a file really exists in the line where I clicked.
  // I verify it through the presence of any content in the FILENUMCOL
  if (ui->fileTable->item(i,FILENUMCOL)->text()!=""){
    removeFile(i);
    myVarTable->myReset();
    if(numOfLoadedFiles>0)
      myVarTable->setCommonX(computeCommonX());
    else{
      ui->refrTBtn->setEnabled(false);
      // The following button can always stay enabled: it won't harm. If I disalble it YI must find somewhere to re-enable!
//      ui->refrUpdTBtn->setEnabled(false);
	}
  }

}


void CDataSelWin::giveFileInfo(QList <int> &fileNums, QList <QString> &fileNames, QList <int> &varNums) {
    /* Questa funzione è una callback da CVarTable che serve per vedere quali numeri di files sono utilizzabili nelle funzioni di variabili.*/
    /* This function is a callback from CVarTable which is used to see which file numbers can be used in variable functions. */
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
/* Function for "downloading" a file from the program.
   * I download only if we are in multifile.
   * Actually the names and values ​​of the variables are not deleted in MySO
   * corresponding to the file to be downloaded, nor the relative variables are deallocated
   * This is because MySO at the time of loading as a first step disallocates memory
   * possibly allocated previously to the vectors of the names and values ​​of the
   * variables.
   * The memory is then freed only a moment before relocation for
   * allow the new file to be uploaded.
  */
  int row, col, fileIndex;
  if(!GV.multiFileMode)return;
  //Il file da rimuovere è sempre quello corrente, in quanto il doppio click è sempre interpretato anche come singolo click che ha selezionato il file.
  //Rendo corrente il primo file della tabella:

  // The file to be removed is always the current one, as the double click is
  // always interpreted as a single click that has selected the file.
  // I make the first file in the table current:
  for (row=1; row<=MAXFILES; row++)
    if(ui->fileTable->item(row_,1)->text()!="") break;
  selectFile(row);

  //rendo "free" l'indice del file che sto per far scomparire dalla tabella:
  // I render "free" the index of the file that I am going to make disappear from the table:
  fileIndex=ui->fileTable->item(row_,1)->text().toInt()-1;
  freeFileIndex<<fileIndex;
  GV.varNumsLst[fileIndex]=0;
  numOfLoadedFiles--;
  setAcceptDrops(true);
  ui->loadTBtn->setEnabled(true);
  if(numOfLoadedFiles==0){
    ui->saveStateTBtn->setEnabled(false);
    ui->varMenuTable->clear();
    ui->varMenuTable->setRowCount(0);
  }
  //Adesso a partire dalla riga su cui si è fatto click sposto più in alto tutti gli items, e i relativi tooltip
  // Now starting from the line on which we have clicked, move all the items upwards, and the relative tooltips
  if(row_<MAXFILES )
    for(row=row_; row<MAXFILES; row++){
      for(col=0; col<ui->fileTable->columnCount(); col++)
        ui->fileTable->item(row,col)->setText(ui->fileTable->item(row+1,col)->text());
      ui->fileTable->item(row,2)->setToolTip(ui->fileTable->item(row+1,2)->toolTip());
   }

  //ora devo "sbiancare" l'ultima riga. Non è visualizzata ma è bene cha abbia contenuto vuoto:
  // now I have to "whiten" the last line. It is not displayed but it is good to have empty content:
  for(col=0; col<ui->fileTable->columnCount(); col++)
     ui->fileTable->item(numOfLoadedFiles+1,col)->setText("");
  if(numOfLoadedFiles>2)
     ui->fileTable->hideRow(numOfLoadedFiles+1);

  // Se il numero di file visualizzati era superiore a 3 rendo invisibile l'ultima riga:
  // If the number of files displayed was greater than 3, make the last line invisible:
  if(visibleFileRows>3){
    visibleFileRows--;
    int newVSize=(visibleFileRows+1)*ui->fileTable->rowHeight(0)+2;
    ui->fileTable->setMaximumHeight(newVSize);
    ui->fileTable->setMinimumHeight(newVSize);
  }

  if(freeFileIndex.count()==MAXFILES)
    fileLoaded=false;

  //se il file che ho scaricato è l'ultimo devo fare una cancellazione completa del contenuto delle 4 tabelle
  // if the file I downloaded is the last one I have to make a complete deletion of the contents of the 4 tables
  if(numOfLoadedFiles==0){
    for (int tab=0; tab<MAXPLOTWINS; tab++)
      varTable[tab]->myReset(true);
    selectedFileIdx=-1;
  }else{
    //devo deselezionare eventuali variabili selezionate relative al file che ho eliminato, in tutte le pagine.
    // I have to deselect any selected variables related to the file I deleted, on all pages.
    for (int tab=0; tab<MAXPLOTWINS; tab++)
      varTable[tab]->unselectFileVars(fileIndex);
    fileNumsLst[fileIndex]=0;
    fileNamesLst[fileIndex]=" ";
    varMaxNumsLst[fileIndex]=0;
  }

  //Se la tabella variabili corrente è vuota devo disattivare tutti i bottoni:
  // If the current variable table is empty, I have to deactivate all the buttons:
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
 /* This private function does operations related to filling the VarMenuTable
  * At the time of its birth (Nov 2016) it is called both by loadFile and by
  * selectFile ().
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

   /* I remember that (Norobro on the Qt forum, Sept. 2012):
  QTableWidget :: clearContents () calls delete (link) on each QTableWidgetItem.
  Stacked objects that will yield unpredictable results. You need to allocate your
  QTableWidgetItem (s) on the heap.
  So you can not pass to cells, for example, elements of a vector
      QTableWidgetItem varItems [100];
  Remember that:
  - the stack is allocated with LIFO technique, while the allocation in the heap
    can take place in any area and therefore requires more calculation resources.
  - heap contains the objects allocated with the new operator
  - stack contains all the other variables, then the static ones that are built
    before the run-time, and the automatic variables (for which the LIFO access
    is particularly convenient).
  */
    ui->varMenuTable->setRowCount(mySO[fileIndex]->numOfVariables);
    for(int i=0; i<mySO[fileIndex]->numOfVariables; i++){
      int j=i+1;
      QString str;
      str.setNum(j);
     //Se il numero massimo supera 9 premetto un digit 0:
     // If the maximum number exceeds 9, press a digit 0:
      if(mySO[fileIndex]->numOfVariables>9 &&j<10)
      str="0"+str;
      //Se il numero massimo supera 99 premetto un ulteriore digit 0:
      // If the maximum number exceeds 99 I press an additional digit 0:
       if(mySO[fileIndex]->numOfVariables>99 &&j<100)
       str="0"+str;
       //Se il numero massimo supera 999 premetto un ulteriore digit 0:
       // If the maximum number exceeds 999 I press an additional digit 0:
       if(mySO[fileIndex]->numOfVariables>999 &&j<1000)
       str="0"+str;
       QTableWidgetItem *item0 = new QTableWidgetItem(str);
       ui->varMenuTable->mySetItem(i,0,item0,true);
       item0->setFont(myFont);
       item0->setToolTip(str);
   /* In item 1 metto il numero della variabile. Il numero di digit con cui è scritto deve essere pari al numero di digit del più alto numero di variabile, per consentire un successivo sort efficace.*/

   /* In item 1 I put the number of the variable. The number of digits with which it
    * is written must be equal to the digit number of the highest variable number,
    * to allow a subsequent effective sort. */
       item0->setBackgroundColor(neCellBkColor);

       QTableWidgetItem *item = new QTableWidgetItem(mySO[fileIndex]->varNames[i]);
       ui->varMenuTable->mySetItem(i,1,item,mySO[fileIndex]->timeVarIndex==i);
       item->setFont(myFont);
       if(mySO[fileIndex]->fileType==MAT_Modelica){
           QString toolTip= mySO[fileIndex]->sVars[i].name+"\n"+
                   mySO[fileIndex]->sVars[i].description;
           item->setToolTip(toolTip);
       }else{
         item->setToolTip(mySO[fileIndex]->varNames[i]);
       }

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
    /* La seguente riga serve per evitare che il plot della finestra Plot
     * faccia riferimento, al momento della creazione della finestra Four,
     * a dati diversi da quelli di quest'ultima: si ricordi che entrambe le
     * finestre non mantengono una copia privata dei dati ma puntano a vettori
     *  esterni.
     * Con questa tecnica anche se in un secondo momento si rifà un plot
     * con dati diversi da quelli con cui si è costruita la finestra Fourier
     * non ci sono problemi.
  */
    /* The following line is used to prevent the Plot window from plotting
     * refer to, when creating the Four window,
     * to data different from those of the latter: remember that both
     * windows do not maintain a private copy of the data but point to vectors
     * external.
     * With this technique even if at a later time a plot is redone
     * with data different from those with which the Fourier window was built
     * there are no problems.
  */
    on_plotTBtn_clicked();
    /* Con la riga precedente si sono passati i dati a myPlotWin, anche facendo
     *  calcoli nel casi di funzioni di variabili. Pertanto la cosa più
     * semplice per effettuare Fourier è di passare a myFourWin puntatori
     * dati asse x e y presi da myPlotWin. Con l'occasione faccio transitare
     * da myPloWin a myFourWin anche dati accessori quali il nome del file,
     * della variabile, ecc.
   */
    /* With the previous line the data has been passed to myPlotWin, also doing
     * calculations in the case of variable functions. Therefore the thing more
     * Simple to make Fourier is to switch to myFourWin pointers
     * x and y axis data taken from myPlotWin. On this occasion I move
     * from myPloWin to myFourWin also accessory data such as the file name,
     * of the variable, etc.
   */
    fourData=myPlotWin->giveFourData();
    /* Gli istanti iniziale e finale sono quelli calcolati da myPlotWin che corrispondono alla frequenza default, solo per una volta per sessione, e sono stati appena caricati in fourData.opt. Se non sono nella prima esecuzione di un Fourier, valuto i dati salvati sul registro, dentro la chiave fourWin. Se sono accettabili uso quelli, altrimenti il full-time range, emettendo contemporaneamente un warning*/
    if(!firstFourPerSession){
      QSettings settings;
      float storedIniTime, storedFinTime;
      settings.beginGroup("fourWin");
      // Verifico l'accettabilità dell'intervallo di tempi salvato con il file correntemente caricato:
      bool timesAreValid=true;
      float epsilon; //one thousandth of constant step, needed for diagnostics.

      epsilon=(mySO[selectedFileIdx]->y[mySO[selectedFileIdx]->timeVarIndex][1]-mySO[selectedFileIdx]->y[mySO[selectedFileIdx]->timeVarIndex][0])/1000.0f;

      storedIniTime=settings.value("initialTime",mySO[selectedFileIdx]->y[mySO[selectedFileIdx]->timeVarIndex][0]).toFloat();
      storedFinTime=settings.value("finalTime",mySO[selectedFileIdx]->y[mySO[selectedFileIdx]->timeVarIndex][mySO[selectedFileIdx] -> numOfPoints-1]).toFloat();

      if(mySO[selectedFileIdx]->y[mySO[selectedFileIdx]->timeVarIndex][0]-epsilon>storedIniTime)
          timesAreValid=false;
      if(mySO[selectedFileIdx]->y[mySO[selectedFileIdx]->timeVarIndex][mySO[selectedFileIdx] -> numOfPoints-1]<storedFinTime-epsilon)
          timesAreValid=false;
      // Se l'intervallo salvato è accettabile lo uso, altrimenti uso l'intero intervallo del file:

      if(timesAreValid){
        fourData.opt.initialTime=settings.value("initialTime",fourData.opt.initialTime).toFloat();
        fourData.opt.finalTime=settings.value("finalTime",fourData.opt.finalTime).toFloat();
      }else{
        QString msg="Unable to determine \"Fourier\" time-range based on saved data.\n"
                "Selected Fourier analysis on the full time-range.\n\n"
                "Please select wished range manually, and possibly save it\n"
                "using \"Save settings\" button"    ;
        QMessageBox::information(this,"CDataSelWin", msg,"");
        fourData.opt.initialTime=mySO[selectedFileIdx]->y[mySO[selectedFileIdx]->timeVarIndex][0];
        fourData.opt.finalTime=mySO[selectedFileIdx]->y[mySO[selectedFileIdx]->timeVarIndex][mySO[selectedFileIdx] -> numOfPoints-1];
      }
    }
    firstFourPerSession=false;
    myFourWin->getData(fourData);
    if(fourData.ret!=""){
      QMessageBox::information(this,"CDataSelWin", fourData.ret,"");
      qDebug()<<"Sent fourier Information";
    }
    myFourWin->close();
    myFourWin->show();
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
    for (int tab=0; tab<MAXPLOTWINS; tab++)
      varTable[tab]->getColorScheme(GV.PO.useOldColors);
    on_resetTBtn_clicked();
  }

    //Qui non è necessario fare un giveData() perché all'interno di myProgOptions le opzioni sono salvate in GV.PO
    // Here it is not necessary to make a giveData () because inside myProgOptions the options are saved in GV.PO
}


CDataSelWin::~CDataSelWin()
{
    delete ui;
}

void CDataSelWin::on_loadTBtn_clicked(){
  QFileDialog dialog(this);
  dialog.setFileMode(QFileDialog::ExistingFiles);

  static QStringList filters;
  static bool filterSet=false;
  QString selectedNameFilter;
  if(!filterSet){
#ifdef EXCLUDEATPCODE
    filters
       << "All compatible files (*adf *.csv *.lvm *.cfg *.mat)"
       << "ADF and CSV (*.adf *.csv)"
       << "LAB-VIEW (*.lvm)"
       << "COMTRADE (*.cfg)"
       << "MATLAB(*.mat)";
#else
   filters
      << "All compatible files (*adf *.csv *.lvm *.pl4 *.cfg *.mat)"
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
     // In MAC at the moment (Qt 5.2 Mar 2014) the multiple filters do not work and therefore do not enable them:
    dialog.setNameFilters(filters);
  }

  #if defined(Q_OS_MAC)
     #ifdef EXCLUDEATPCODE
       dialog.setNameFilter("ADF, CSV, CFG, LVM, MAT (*adf *.csv *.cfg *.lvm *.mat)");
     #else
       dialog.setNameFilter("ADF, ATP, CSV, CFG, LVM, MAT (*adf *.pl4 *.csv *.cfg *.lvm *.mat)");
    #endif
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

  /* I want the filters to remain open the first time the next time you open them again.
   * If for example I am working on CSV, I most likely will want to reopen csv again.
   *
   * But this I do only for PC, because for MAC the multiple filters do not work
   *
   * */
#if defined(Q_OS_MAC)
;
#else
  selectedNameFilter=dialog.selectedNameFilter();
  if(selectedNameFilter!=filters[0]){
    //allora l'utente ha scelto un filtro diverso: faccio lo swap fra il primo e quello scelto.
    // then the user has chosen a different filter: I swap between the first and the chosen one.
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
  resizeEvent(nullptr);
  ui->varMenuTable->resizeColumnsToContents();
  // La stringa ret non va trattata in un messageBox perché è già stato fatto all'interno di loadFile().
  // The string ret should not be treated in a messageBox because it has already been done inside loadFile ().
}

void CDataSelWin::on_refrTBtn_clicked(){
  /*Per trovare il nome del file completo del percorso parto dal tooltip, che lo contiene.
   * Il tooltip è fatto così:
   *   fileTooltip="<p><B>Full name:</B> "+ fullName+"</p>";
   *  cui è aggiunta poi altra roba.
   * Pertanto prima taglio i primi 21 caratteri, poi taglio quello che c'è a partire
   * dalla prima parentesi angolare aperta
  */
  /* To find the complete file name of the path, I start the tooltip, which contains it.
   * The tooltip is like this:
   *   fileTooltip="<p><B>Full name:</B> "+ fullName+"</p>";
   * Then other stuff is added.
   * Therefore I first cut the first 21 characters, then what is there starting from
   * the first open angular bracket
  */
  QString name=ui->fileTable->item(selectedFileRow,2)->toolTip();
  name.remove(0,21);
  name.chop(name.length()-name.indexOf('<',0));

  qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
  enum ESortType oldSortType=sortType;

  loadFile(selectedFileIdx,name,true,refreshUpdate);
  if(oldSortType==ascending)
    on_sortTBtn_clicked(); //modifica il sortType e vado in ascending!
                           // change the sortType and go into ascending!
  else if(oldSortType==descending){
    on_sortTBtn_clicked(); //modifica il sortType e vado in ascending
                           // change the sortType and go into ascending
    on_sortTBtn_clicked(); //modifica il sortType e vado in descending
                           // change the sortType and go into descending
  }
  qApp->restoreOverrideCursor();
  updatingPlot=refreshUpdate;
  
/*
    Qui devo:
 1) selezionare in sequenza i vari sheet (come se facessi un click, e non direttamente da  ui->tabWidget->setCurrentIndex(0); Questo è importante perché se uno sheet non ha variabili in questo modo il pulsante plot non è attivo
 2) se il pulsante plot è attivo lo clicco
 3) ripristino currentShIndex

*/

/*
    Here I have to:
 1) select in sequence the various sheets (as I were making a click, and not
    directly from ui-> tabWidget-> setCurrentIndex (0)). This is important because
    if a sheet has no variables in this way the plot button is not active
 2) if the plot button is active, I click it
 3) restore currentShIndex

*/
 int currentTabIndex=ui->tabWidget->currentIndex();
    for(int iTab=0; iTab<actualPlotWins; iTab++){
      on_tabWidget_currentChanged(iTab);
      if(ui->plotTBtn->isEnabled())
          on_plotTBtn_clicked();
      // Può capitare che si faccia il refresh da un file nel quale non sono presenti variabili precedentemente visualizzate. In questo caso non eseguo il plot e neanche il Four (il quale al suo interno comanderebbe comunque un plot causando un segfault)
      if(ui->plotTBtn->isEnabled() && fourWin[iTab]->isVisible())
          on_fourTBtn_clicked();
    }
    /* La seguente riga commentata fa un sempice switch della table. Non va bene perché non aggiusta lo stato enabled dei vari bottoni.*/
    on_tabWidget_currentChanged(currentTabIndex);
    //ui->tabWidget->setCurrentIndex(currentTabIndex);
    myVarTable=varTable[currentTabIndex];

    //Aggiorno la finestra dei parametri, se visibile
    // Update the parameter window, if visible
    if(myParamWin->isVisible())
        myParamWin->fillTable();
    updatingPlot=false;
    if(myFourWin->isVisible() && ui->fourTBtn->isEnabled()){
        on_fourTBtn_clicked();
    }
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
  /* This function is used to save some variables on files, coming from a single input file.
Its typical use is to extract a few variables from a file that can contain many.
In the future, perhaps it will be extended to allow saving of variables coming from multiple files,
but in any case they will have to have the same number of points
*/
  int i,
      myFileIdx=-1, //indice del file da cui salvare variabili
                    // index of the file from which to save variables
      *stoVarIdx, //vettore degli indici di variabili da prelevare dal file di indice
                  // myFileIdx e salvare
                  // vector of the variable indexes to be taken from the index file
                  // myFileIdx and save
      nStoVars,  //numero di variabili da salvare
                 // number of variables to be saved
      nBkpVars;  //numero di variabili nel file da cui salvare a disco (e di cui
                 //backup-are i nomi)
                 // number of variables in the file to be saved to disk (and whose names
                 // are to be back-upped)
  SCurveParam info;
  QString msg, fullName, ext,
          *bkpVarNames;  //Vettore temporaneo di nomi in cui copiare i nomi del file
                         //da cui salvare su disco
                         // Temporary vector of names in which to copy the names of
                         //the file to save to disk
  QString comment;

  myVarTable->analyse();
  //Se ci sono funzioni emetto un messaggio ed esco:
  // If there are any functions, issue a message and exit:
  if(myVarTable->giveFunInfo().count()>0){
      qDebug()<<"warning 7";
      QMessageBox::warning(this, "PlotXY",
        "you can save only file vars:\nfunctions not allowed when saving");
      return;
  }
  //Se ci sono variabili provenienti da più files emetto un messaggio ed esco (non dovrebbe accadere in quanto in questo caso il bottone SaveVars dovrebbe essere disattivato):
  // If there are variables coming from several files, issue a message and exit:
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
  // The following "+1" is due to the fact that in addition to the selected variable we always save the time
  nStoVars=myVarTable->yInfo[myFileIdx].count()+1;
  nBkpVars=mySO[myFileIdx]->numOfVariables;
  stoVarIdx=new int [nStoVars];
  bkpVarNames=new QString[nBkpVars];

  // riempo gli stoVarIdx con gli indici delle variabili da salvare (la prima è il tempo):
  // fill the stoVarIdx with the indices of the variables to be saved (the first is the time):
  stoVarIdx[0]=0;
  for(i=0; i<nStoVars-1; i++)
    stoVarIdx[i+1]=myVarTable->yInfo[myFileIdx][i].idx;

 //A questo punto ho solo variabili dal file di indice myFileIdx, e posso fare il salvataggio.
 // At this point I only have variables from the myFileIdx index file, and I can save.

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
  /* The saveToAdfFile routine, originally designed for the Converter.exe program,
   * which after the conversion was discarded by the input program, changes all
   * variable names to make them compatible with the output format. Therefore it
   * is necessary to provide to save the original names and then restore them.
  */
  for(i=0; i<nBkpVars; i++)
    bkpVarNames[i]=mySO[myFileIdx]->varNames[i];
  // ora devo verificare che l'estensione sia fra quelle consentite:
  // now I have to verify that the extension is between those allowed:
  ext=fullName.right(fullName.length()-fullName.lastIndexOf('.')-1);
  ext=ext.toLower();

  mySO[myFileIdx]->commasAreSeparators=GV.PO.commasAreSeparators;

  if(ext=="adf"){
    comment="ADF file created by \"";
    comment+="MC's PlotXY";
    comment+="\" program";
    msg=mySO[myFileIdx]->saveToAdfFile(fullName, comment, nStoVars, stoVarIdx);
  }
  else if(ext=="cfg"){
      comment="CFG file created by \"";
      comment+="MC's PlotXY";
      comment+="\" program";
    msg=mySO[myFileIdx]->saveToComtradeFile(fullName, comment, nStoVars, stoVarIdx);
  }
  else if(ext=="mat")
    msg=mySO[myFileIdx]->saveToMatFile(fullName, nStoVars, stoVarIdx);
  else if(ext=="pl4"){
    if(mySO[myFileIdx]->allowsPl4Out)
        msg=mySO[myFileIdx]->saveToPl4File(fullName, nStoVars, stoVarIdx);
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
    mySO[myFileIdx]->varNames[i]=bkpVarNames[i];
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


void CDataSelWin::on_eqTBtn_clicked() {
    /* Questa funzione serve per equalizzare le dimensioni delle finestre di plot, o a un numero prefissato, o alle dimensioni della finestra 1. In quest'ultimo caso l'utente vede visivamente, agendo sulla finestra 1, come verranno le dimensioni delle varie finestre.
     *Le finestre mantengono l'angolo quperiore sinistro originale. Per risistemarle nello spazio a disposizione occorre cliccare sul bottone "arrange"
*/
    /* This function is used to equalize the dimensions of the plot windows,
     * or to a predetermined number, or to the dimensions of the window 1.
     * In this last case, the user visually sees, by acting on window 1,
     * how the various windows will look.
     * The windows maintain the original left quarter angle. To rearrange
     * them in the available space, click on the "arrange" button
*/
    int w,h;
    QScreen *screen=QGuiApplication::primaryScreen();
    QRect avGeom=screen->availableGeometry();
//    QMargins m(10,10,10,10);
    avGeom=avGeom.marginsRemoved(QMargins(10,10,10,10));

    if(ui->allToBtn->isChecked()){
        w=ui->winWEdit->text().toInt();
        h=ui->winHEdit->text().toInt();
        //Come unica verifica sui dati evito che larghezza e altezza superino le dimensioni dello schermo. Presumo infatti che le finestre siano in partenza con la sbarra superiore all'interno della parte utile dello schermo e pertanto se per casu una parte della finestra sforasse fuori schermo l'utente potrebbbe agevolmente risistemarle.

        // As the only verification of the data I avoid the width and height
        // exceeding the screen size. In fact, I assume that the windows are
        // starting with the upper bar inside the useful part of the screen
        // and therefore if for a part of the window casu out of screen the
        // user could easily rearrange them.
        w=min(w, avGeom.right()-avGeom.left());
        h=min(h, avGeom.bottom()-avGeom.top());
        for (int win=0; win<MAXPLOTWINS; win++)
          plotWin[win]->resize(w,h);
    } else if(ui->toWin1Btn->isChecked()){
        w=plotWin[0]->width();
        h=plotWin[0]->height();
        for (int win=1; win<MAXPLOTWINS; win++)
          plotWin[win]->resize(w,h);
        QString s;
        ui->winWEdit->setText(s.setNum(w));
        ui->winHEdit->setText(s.setNum(h));
      }

}

void CDataSelWin::on_arrTBtn_clicked(){
/* this function rearranges plot windows.
 * The windows are positioned as a 4x4 matrix, following the natural direction of western
 * writing.
 * If I have more than 4 windows, only the first 4 are automatically arranged.
 *
*/

    bool sizeIsEnough=true;
/*
    QScreen *screen=QGuiApplication::primaryScreen();
    QRect avGeom=screen->availableGeometry();
    avGeom=avGeom.marginsRemoved(QMargins(10,10,10,10));
*/
    QRect win1Rect, win2Rect;
    if(plotWin[0]->isVisible())
      win1Rect=plotWin[0]->frameGeometry();
    if(plotWin[1]->isVisible())
      win2Rect=plotWin[1]->frameGeometry();
    QRect thisFrameGeom=this->frameGeometry();
    int screenCount=QGuiApplication::screens().count();
    //the space globally available is right() of availableGeometry of the last screen
    QScreen *lastScreen=QGuiApplication::screens()[screenCount-1];
    int totAvailableWidth=lastScreen->availableGeometry().right();  //Total availableWidth of all screens!!
    QScreen * myScreen=nullptr;
    for(int i=0; i<screenCount; i++){
      myScreen=QGuiApplication::screens()[i];
      int rightPix=myScreen->availableGeometry().right();
      if(this->x()<rightPix){
        break;
      }
    }
    int availableHeight=myScreen->availableGeometry().height();

    //but if I have a single window, the available space is reduced, since we do not
    // have two plot windows aside each other.:
    if(!plotWin[1]->isVisible() && GV.PO.rememberWinPosSize){
      if(totAvailableWidth-thisFrameGeom.width()-thisFrameGeom.x()< win1Rect.width())
        sizeIsEnough=false;
      if(availableHeight-thisFrameGeom.y() <win1Rect.height()) //We must have enough height for plot1 window
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

    if(plotWin[1]->isVisible()&& GV.PO.rememberWinPosSize){
      if(totAvailableWidth-thisFrameGeom.width()-thisFrameGeom.x()  <
                                                  win1Rect.width()+win2Rect.width())
        sizeIsEnough=false;
      if(availableHeight-thisFrameGeom.y() <win1Rect.height()+win2Rect.height())
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
    plotWin[0]->move(r.topLeft());

    r=plotWin[0]->frameGeometry();
    r.moveLeft(r.x()+win1Rect.width()+1);
    plotWin[1]->move(r.topLeft());

    r=plotWin[0]->frameGeometry();
    r.moveTop(r.y()+win1Rect.height()+1);
    plotWin[2]->move(r.topLeft());

    r=plotWin[1]->frameGeometry();
    r.moveTop(r.y()+win1Rect.height()+1);
    plotWin[3]->move(r.topLeft());

    //I put MyFourWin in the same position of Plot3; if plot3 is visible, it will be below DataSelection window.
    if(plotWin[2]->isVisible()){
      r=thisFrameGeom;
      r.moveTop(r.x()+r.height()+1);
    }else{
      r=plotWin[0]->frameGeometry();
      r.moveTop(r.y()+win1Rect.height()+1);
    }
    myFourWin->move(r.topLeft());
}

void CDataSelWin::on_saveStateTBtn_clicked()
{
    /* Pressing the saveStateTBtn key causes the "status" of the program to be saved.

    Saving phases:
    1) saving the multiFileMode and number of plots
    2) saving the name, accompanied by information, date and time, files currently loaded in memory, fourWin visibility
    3) saving the contents of the varTable# tables
    4) saving the currently displayed table-plot index
    5) saving the index of the current file (important also for theinterpretation of the strings definign function plots, when they are written as v# (not f#v#).
    */
    QSettings settings;
    int iSheet, i,j, r, filesSaved;
    QString keyName, fileName, pathName, tShift;

    settings.beginGroup("programState");
    // Delete the previously saved state:
    settings.remove("");

    // Fase 1: salvataggio del multifileMode. Occorre notare che il multifileMode viene salvato anche i GV.PO.multifilemode, ma in un momemto diverso. Non si può quindi fare affidamento a quel valore in quanto può essere diverso da quello che si ha al momento del salvataggio dello stato.

    // Phase 1: saving the multifileMode. It should be noted that the
    // multifileMode is also saved in the GV.PO.multifilemode, but in
    // a different moment. You can not therefore rely on that value as
    // it can be different from what you have at the time of saving the state.
    settings.setValue("multifileMode",GV.multiFileMode);
    settings.setValue("numOfPlotWins",actualPlotWins);

    // Phase 2: saving the name of the files, and info on the date and time; fourWin visibility
    if(GV.multiFileMode)
      filesSaved=numOfLoadedFiles;
    else
      filesSaved=1;
    settings.setValue("Number Of Files",filesSaved);
    j=0;
    for(i=1; i<MAXFILES+1; i++){
      // Looking for a file row containing i + 1 as file number:
      for (r=1; r<=MAXFILES; r++){
//        int kk=ui->fileTable->item(r,FILENUMCOL)->text().toInt();
        if(ui->fileTable->item(r,FILENUMCOL)->text().toInt() == i)
            break;
      }
      //Se non ho trovato nessun file di indice i passo al file successivo:
      // If I have not found any index files, I move to the next file:
      if(r==MAXFILES+1)
          continue;
      //A questo punto i è un indice di file presente in memoria, posizionato nella tabella fileTable alla riga r (r=1 per il primo dei files visualizzati)
      // At this point i is a file index in memory, located in the fileTable table on line r
      fileName=ui->fileTable->item(r,FILENAMECOL)->text();
      pathName=ui->fileTable->item(r,FILENAMECOL)->toolTip();
      tShift=ui->fileTable->item(r,TSHIFTCOL)->text();

      //Per trovare il nome del file completo del percorso parto dal tooltip, che lo contiene.
      //Il tooltip è fatto così:
      //  fileTooltip="<p><B>Full name:</B> "+ fullName+"</p>";
      // cui è aggiunta poi altra roba.
      //Pertanto prima taglio i primi 21 caratteri,poi taglio quello che c'è a partire dalla prima parentesi angolare aperta

      // To find the complete file name of the path, I start the tooltip, which contains it.
      // The tooltip is like this:
      //   fileTooltip="<p><B>Full name:</B> "+ fullName+"</p>";
      // Then another stuff is added.
      // Therefore first cut the first 21 characters, then cut what is there starting from the first open angle bracket
      pathName.remove(0,21);
      pathName.chop(pathName.length()-pathName.indexOf('<',0));

      //Se non sono in multifile, anche se in memoria sono presenti più files salvo solo quello correntemente attivo:
      // If they are not in multifile, even if there are more files in the memory, except for the one currently active:
      if(!GV.multiFileMode && i!=selectedFileIdx+1)
        continue;
      keyName="File_"+QString::number(++j);
      //Oltre al nome del file salvo anche l'informazione su data e ora dell'ultima modifica. In tal modo se al ricaricamento non corrispondono posso accorgermi che i due files sono in realtà diversi e quindi non mi devo attenderere lo stesso contenuto in termini di variabili.

      // In addition to the file name, this also includes information on the
      // date and time of the last modification. In this way if the reloads
      // do not match I can realize that the two files are actually different
      // and therefore I do not have to wait for the same content in terms of variables.
      QFileInfo info1(pathName);
      QDateTime dateTime=info1.lastModified();
      settings.setValue(keyName+".name",pathName);
      settings.setValue(keyName+".date",dateTime);
//Salvo anche il numero attribuito al file, che può non essere sequenziale. Questo in quanto tale numero è poi usato dai dati salvati nelle tabelle delle variabili, e altrimenti si avrebbe un disallineamento fra la numerazione del file nella tabella file e nelle tabelle variabili

      // Except for the number assigned to the file, which may not be sequential.
      // This is because this number is then used by the data saved in the variable
      // tables, and otherwise there would be a misalignment between the numbering
      // of the file in the file table and in the variable tables
      settings.setValue(keyName+".num",i);
      //Salvo il tShift
      // Saving tShift
      settings.setValue(keyName+".shift",tShift);
      for(int iTab=0; iTab<actualPlotWins; iTab++){
        QString str="fourWin"+QString::number(iTab+1)+"Visible";
        settings.setValue(str,  fourWin[iTab]->isVisible());
      }
      if(!GV.multiFileMode)
        break;
    }

    // Fase 3: Salvataggio dati delle varTables
    // Phase 3: Saving data of the varTables

    settings.beginGroup("VarTables");
    for (iSheet=0; iSheet<MAXPLOTWINS; iSheet++){
      CVarTableComp * myTable;
      SVarTableState  tableState;
      myTable=varTable[iSheet];
      tableState=myTable->giveState();
      keyName="VarTable_"+QString::number(iSheet+1)+".names";
      settings.setValue(keyName, tableState.allNames);
//      keyName="VarTable_"+QString::number(iSheet+1)+".colors";
      settings.beginGroup("Colors");
      for (int iRow=0; iRow<TOTROWS-1; iRow++){  // Qui iRow è il numero di riga al netto dell'intestazione!
        keyName="VarTable_"+QString::number(iSheet+1)+"_"+ QString::number(iRow+1)+".colors";
        settings.setValue(keyName, tableState.varColors[iRow]);
      }
      keyName="VarTable_"+QString::number(iSheet+1)+".colors";
      settings.endGroup();  //Colors; entering varTables
      // Per lo stile creo un int per ogni tabella. I 16 bit meno significativi conterranno 1 se lo stile è dashed 0 in caso contrario.
      int styleData=0;
      for (int iRow=0; iRow<TOTROWS-1; iRow++){
          int dashed=int(tableState.styles[iRow]==Qt::DashLine);
          styleData|=dashed<<iRow;
      }
      keyName="VarTable_"+QString::number(iSheet+1)+".styleData";
      settings.setValue(keyName, styleData);

//      keyName="VarTable_"+QString::number(iSheet+1)+".xIsFunction";
      keyName="VarTable_"+QString::number(iSheet+1)+".xInfoIdx";
      settings.setValue(keyName, tableState.xInfoIdx);

      ui->loadStateTBtn->setEnabled(true);
    }

    // Fase 4: salvataggio dell'indice della tabella-plot correntemente visualizzata
    // Phase 4: saving the currently displayed table-plot index
    int tableIndex=ui->tabWidget->currentIndex();
    settings.setValue("tableIndex", tableIndex);


    //Fase 5: salvataggio dell'indice del file corrente
    //Phase 5: saving the current (selected) file index
    settings.endGroup(); //exiting VarTables, entering programState
    settings.setValue("selectedFileIndex", selectedFileIdx);


    // Indicazione del salvataggio avvenuto per 0.7 s (passato il quale il timer rimetterà le cose a posto)
    // Indication of the saving occurred for 0.7 s (after which the timer will set things right)
    QRect rect0=saveStateLbl->geometry(), rect=rect0;
    QPoint center=ui->saveStateTBtn->geometry().topRight();
    rect.moveCenter(center);
    rect.moveTop(rect0.top());
    saveStateLbl->setGeometry(rect);
    saveStateLbl->setText("State Saved");

    ui->saveStateTBtn->setVisible(false);
    ui->loadStateTBtn->setVisible(false);
    saveStateLbl->setVisible(true);

    settings.endGroup(); //exiting programState, entering MC's PlotXY
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
  3) recupero i dati di fourWin
  4) Recupero il testo delle celle delle tableComp, e le invio loro.
     Quando tableComp riceve le stringhe ricostruisce gli altri dati interni che ne
     completano lo stato. Faccio i grafici
  5) operazioni finali
*/
/*
    Stages of the status loading operations:
  1) retrieve the multifileMode and its correct activation
  2) recovery of the name, accompanied by information, date and time, files
     saved, and uploading them by simultaneously deleting those already in memory
  3) Recover the text of the tableComp cells, and send them to them.
     When tableComp receives the strings it reconstructs the other internal data that it contains
     complete the state. Make plots
  4) final operations
*/

  int filesStored;
  QDateTime dateTime;
  QFileInfo fileInfo;
  QSettings settings;
  QString keyName, pathName, tShift;
  settings.beginGroup("programState");

  //1) setto il multifileMode al valore salvato:
  // 1) sets the multifileMode to the saved value:
  bool multifileMode=settings.value("multifileMode").toBool();
  if(GV.multiFileMode!=multifileMode){
    on_multifTBtn_clicked(multifileMode);
    ui->multifTBtn->setChecked(!ui->multifTBtn->isChecked());
  }
  int wins=settings.value("numOfPlotWins",4).toInt();
  //The following row internally sets variable "actualPlotWins"
  setActualPlotWins(wins);

  //Fase 2: recupero nomi files e gestione fileTable
  //Per la fase 1 seguo la seguente procedura:
  // 2.1) leggo le registrazioni dei files e faccio una verifica di validità
  // 2.2) se è tutto OK scarto i files eventualmente già presenti in memoria
  //      e carico quelli nuovi, altrimenti lascio tutto com'è ed emetto
  //      un messaggio di errore.

  // Phase 2: recovery of file names and file management
  // For step 1, I follow the following procedure:
  // 2.1) I read the recordings of the files and make a validity check
  // 2.2) if everything is OK, I reject the files already present in the memory
  //      and load the new ones, otherwise I leave everything as it is and emit
  //      an error message.

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
    // I check if the file exists and if time and date coincide with those stored
    fileInfo.setFile(pathName);
    bool valid=fileInfo.isFile();
    if(!valid || fileInfo.lastModified()!=dateTime){

      QMessageBox msgBox;
        msgBox.setText(
          "Warning:\n file \""+pathName+"\"\n"+
          "has time and date stamps that do not correspond to those stored when saving state\n");
        msgBox.setInformativeText(
          "Do you want to proceed with reloading it?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No );
        msgBox.setDefaultButton(QMessageBox::No);
        int ret = msgBox.exec();

        if(ret==QMessageBox::No)
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

  /*2.2) If I have arrived here all the files are valid, and I can proceed to
  delete those currently loaded and reload the new ones.
  The maneuver is a bit 'delicate as it may be the case that I am in singleFile
  but there are more files in memory that are visible only after switching in multiFile.
  Therefore I adopt the following technique:
  - If numOfSelFiles> 1, I start in multiFile
  - simulate a double click of the mouse on the various rows of files.
  - I start in multiFile or singleFile depending on the number of uploaded files.
  */
  int i;
  if(filesStored>1 && ! ui->multifTBtn->isChecked()){
    ui->multifTBtn->setChecked(true);
    on_multifTBtn_clicked(true);
  }

  //Eliminazione dei files (simulo ripetuti doppi click sulle varie righe):
  // Deleting files (simulate repeated double clicks on the various lines):
  for(i=1;  i<ui->fileTable->rowCount(); i++) {
     if(ui->fileTable->item(1,2)->text()!="")
       removeFile(1);  //l'indice di riga è sempre 1 perché dopo ogni rimozione i nomi sono spostati tutti verso l'alto
                       // the row index is always 1 because after each removal the names are all moved upwards
  }

  // Caricamento nuovi files.
  // Si ricorda che è necessario che i numeri di files siano quelli usati durante il salvataggio, altrimenti vi sarebbe disallineamento con i dati della tabella delle variabili. Tali numeri sono stati segnati assieme ai nomi dei files e qui li ripristino

  // Loading new files.
  // Remember that it is necessary that the file numbers are those used during
  // the saving, otherwise there would be a misalignment with the data of the
  // variable table. These numbers have been marked along with the names of the
  // files and here they are restored

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
  QResizeEvent *event=nullptr;
  resizeEvent(event);

  // Phase 3: Fourwin data
  keyName="fourWin/tableIndex";
  bool fourWinVisible[MAXPLOTWINS];
  for(int iSheet=0; iSheet<MAXPLOTWINS; iSheet++){
    QString str="fourWin"+QString::number(iSheet+1)+"Visible";
    fourWinVisible[iSheet]=settings.value(str,false).toBool();
  }



  // Phase 4: Retrieve text, color palette and cell styles of tableComp's, and send to them.
  // When tableComp receives the strings it reconstructs the other internal data that complete its status.
  QStringList list;
  bool xIsFunction;
  int xInfoIdx;
  settings.beginGroup("VarTables");
  for (int iSheet=0; iSheet<actualPlotWins; iSheet++){
    keyName="VarTable_"+QString::number(iSheet+1)+".names";
    list.clear();
    list=settings.value(keyName,"").value<QStringList>();

    QVector <QRgb> colorVect;
    settings.beginGroup("Colors");
    for (int iRow=0; iRow<TOTROWS-1; iRow++){
      keyName="VarTable_"+QString::number(iSheet+1)+"_"+ QString::number(iRow+1)+".colors";
      QRgb color=settings.value(keyName).value<QRgb>();
      if(color==0){
       QMessageBox::information(this, "PlotXY",
                 "The stored data do not appear to be created using this PlotXY version\n"
                 "Save state operation before restoring.\n"
                 "No data restored.");
          return;
      }
      colorVect.append(color);
    }
    settings.endGroup();  //Colors
    int styleData=0;
    keyName="VarTable_"+QString::number(iSheet+1)+".styleData";
    styleData=settings.value(keyName,"").value<int>();

    keyName="VarTable_"+QString::number(iSheet+1)+".xIsFunction";
    xIsFunction=settings.value(keyName,"").value<bool>();

    keyName="VarTable_"+QString::number(iSheet+1)+".xInfoIdx";
    xInfoIdx=settings.value(keyName,"").value<int>();

  // It seems that the getState of a varTable can not be done when it is not displayed. Therefore we to switch before issuing getState().
  // Note that the following line implicitly runs an on_tabWidget_currentChanged (), which in turn runs a setCommon() if necessary.

    myVarTable=varTable[iSheet];
    myVarTable->getState(list, colorVect, styleData, xIsFunction, xInfoIdx, multifileMode);
    myVarTable->getFileNums(fileNumsLst, varMaxNumsLst);
    if(myVarTable->numOfTotVars>1){
      ui->tabWidget->setCurrentIndex(iSheet);
      myPlotWin=plotWin[iSheet];
      on_plotTBtn_clicked();
      if(fourWinVisible[iSheet])
        on_fourTBtn_clicked();
    }
  }
  settings.endGroup();  //VarTables


  //Fase 5: operazioni finali:
  // Phase 5: final operations:
  // 5.1: seleziono il file correntemente selezionato all'atto del salvataggio
  selectedFileIdx=settings.value("selectedFileIndex",0).value<int>();
  //i file sono stati caricati in maniera sequenziale, con i numeri in prima colonna che vanno da 1 al massimo. Quindi per selezionare il file salvato basta selezionare la riga della tabella che valuto sulla base del valore sequenziale atteso del numero di file:
  selectFile(selectedFileIdx+1);
  resizeEvent(nullptr);


  //5.2: Seleziono la scheda di plot giusta e faccio le operazioni relative:
  //5.2  I select the right plotting card and do the related operations:
  int savedIndex=settings.value("tableIndex",0).value<int>();
  ui->tabWidget->setCurrentIndex(savedIndex);
  on_tabWidget_currentChanged(savedIndex);

  // Indicazione del salvataggio avvenuto per 0.7 s (passato il quale il timer rimetterà le cose a posto)
  // Indication of the saving occurred for 0.7 s (after which the timer will set things right)

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
//  on_resetTBtn_clicked();

  QTimer::singleShot(700, this, SLOT(resetStateBtns()));

}

void CDataSelWin::enterEvent(QEvent *){
  for (int win=0; win<MAXPLOTWINS; win++){
  if(plotWin[win]->isVisible())
     plotWin[win]->raise();
  }
  if(myFourWin->isVisible())
     myFourWin->raise();
  //Ora che ho alzato tutte le finestre che potevano essere sotto altre, devo alsare la dinestra DataSelWin, che deve superare quelle di plot:
  raise();
}


void CDataSelWin::moveEvent(QMoveEvent *){

  /*  Questa funzione serve per identificare lo schermo attuale, per vedere quando cambia
   * e attuare al cambiamento una personalizzazione delle finestre del programma in funzione
   * dell'effettivo DPI.
   *
   * Notare che c'è un problema in questa funzione, credo ascrivibile a Qt e/o a Windows: durante
   * il passaggio da uno schermo all'altro non tutti i font definiti in punti vengono
   * aggiornati. Vengono in particolare aggiornati i font dei bottoni e delle intestazioni
   * delle schede, ma non quelli degli items passati alle tabelle.
   * Andrebbe fatto un lavoro di aggiornamento manuale di tali font, ma questo sembra più
   * qualcosa da discutere nel forum di Qt.
   *
  */

  /* This function is used to identify the current screen, to see when it changes
   * and implement a customization of the windows of the running program to change
   * of the actual DPI.
   *
   * Note that there is a problem with this function, which I believe can be attributed to Qt and / or Windows: during
   * switching from one screen to another not all the fonts defined in points come
   * updated. In particular, the fonts of the buttons and the headings are updated
   * of the cards, but not those of the items passed to the tables.
   * Manual updating of these fonts should be done, but this seems more
   * something to discuss in the Qt forum.
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
  int DPI=int(screen->logicalDotsPerInch());
  if (DPI!=int(currentDPI)){
    //  Voglio che la massima altezza della parte utile sia pari alla massima utilizzabile sul desktop. L'altezza che passo a adaptToDPI è invece l'altezza della parte utile della finestra.
    //pertanto qui tengo conto di questo e riduco maxHeight di conseguenza:

    // I want the maximum height of the useful part to be equal to the maximum
    // usable on the desktop. The height I pass to adaptToDPI is instead the
    // height of the useful part of the window.
    // So here I take this into account and reduce maxHeight accordingly:
    int maxHeight=int(FRACTIONOOFSCREEN*screen->availableGeometry().height())
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

/* Pressing the button controls the sorting of the variables. For ascending or
 * descending sorting, we order on the basis of the contents of the column of
 * variables, column N. 1. For the inversion of the sort, we order on the basis
 * of the content of the first column, number "0".
 * In order for this to work, the numbers in the first column must all be written
 * with the same digits number. For example if the numbers are more than 9 and
 * less than 100 the first and the second must be "01" and "02 and not" 1 "and" 2
*/
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

void CDataSelWin::on_showParTBtn_clicked(bool checked) {
    //Se dopo il caricamento del file non ho ancora richiesto mai la visualizzazione della tabella dei parametri, la devo costruire. Altrimenti la devo solo visualizzare

    // If I have not yet requested the display of the parameter table after
    // loading the file, I have to build it. Otherwise I just have to view it
    if(!paramWinTableBuilt){
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      myParamWin->fillTable();
      QApplication::restoreOverrideCursor();
      paramWinTableBuilt=true;
    }

    if(checked)
        myParamWin->show();
    else
        myParamWin->hide();
}


void CDataSelWin::on_tool468_clicked()
{
    if(actualPlotWins==4)
      setActualPlotWins(6);
    else if(actualPlotWins==6)
      setActualPlotWins(8);
    else if(actualPlotWins==8)
      setActualPlotWins(4);
}
