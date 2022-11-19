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

#ifndef CLINECHART_H
#define CLINECHART_H
#include <QRect>
#include <QWidget>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPrinter>
#include <QStack>
#include <QTimer>
#include <QToolTip>

#define MAXFILES 9  //numero massimo di files (le linee inizialmente visualizzate sono invece 3)
#define MAXVARS 15  //Numero massimo di variabili nella lista (escluso il tempo)

//#include "SuppFunctions.h"  //commentata in quanto sia le funzioni di allocazione delle matrici, che la smartSetNum sono state definite come funzioni statiche all'interno dellala classe
#define RATIOLIM 0.799f
#define MAXAUTOMARKS 4
#define MAXMANUMARKS 8
#define MAXLOGTICS 26  //Cinque tacche per decade, cinque decadi più una
#define EXPFRACTION 0.75f  //dimensioni dell'esponente in frazione delle dimensioni della base


struct SFileInfo {
    bool frequencyScan; // CURRENTLY NOT USED in CLineChart
    bool variableStep; //Tells if points are constantly spaced or not. Normally CLineChart is used with variableStep files. If it is known that the step is constant (i.e. points equally spaced) some faster algorithms are used in some contingencies
    int fileNum, // CURRENTLY NOT USED in CLineChart
    numOfPoints; //The total number of points contained in the considered file (every point will have an x-axis value, e.g. a time for simulations)
    float timeShift; // CURRENTLY NOT USED in CLineChart
    QString name; //A file name used by CLineChart legends
};

struct SXVarParam{
    bool isMonotonic, //Tells whether the x-values are monotonically increasing. When this is not true, some functions are disabled. In fase oc parametric curves, for instance the horizontal-axis is not monotonic and the vertical rectangle to see numerical values is disabled.
         isVariableStep;
    QString unitS; //le unità di misura, ad es. "s" "kA", etc.
    QString name;
};
struct SCurveParam {
    bool isFunction; //true se ho una variabile-funzione
    bool isMonotonic; //Nel caso essa sia true, sarà possibile effettuare le zoomate in maniera molto più veloce in quanto si sfrutta il fatto di sapere che X è monotona crescente.
//    int fileIndex; //numero del file in base 0
    int idx; //numero della variabile all'interno del file considerato: indice di mySO[ifile]->y[num];'
    int timeConversion; //vale 0 se non faccio conversioni, 1: s->h, 2: s->h
    QString name; //il nome della variabile; per le funzioni f1, f2, ecc.
    QString midName;  //per le funzioni la loro stringa, tipo f1v1+2*v3
    QString fullName; // per le funzioni il nome completo, tipo voltage1-voltage2/2.0
    QColor color; // il colore (anche se non definito per la var. x sarà sempre black)
    Qt::PenStyle style;
    bool rightScale; //valore non definito per la variabile x
    QString unitS;
};

struct SUserLabel {
  QString B,E; //Stanno per base, esponente: l'esponente scritto pi piccolo e in alto
             // per consentire una buona visualizzazione delle potenze di 10.
};

struct SFloatRect2{float Left,Right,LTop,LBottom,RTop,RBottom;};

class QCursor;

enum EPlotType{ptLine,ptBar, ptSwarm};
enum EPlotPenWidth {pwThin, pwThick, pwAuto};
enum EScaleType {stLin, stDB, stLog};
enum ESwarmPointSize {ssPixel, ssSquare};
enum EDrawType {dtMC, //Filtraggio con gli algoritmi di MC, come sviluppati
                      //nell'implementazione storica BCB
       dtMcD, //come dtMC ma con uso di double invece di float
       dtQtF, //assenza di filtraggio e uso della funzione painter.lineTo fra numeri float
       dtQtI,//assenza di filtraggio e uso della funzione painter.lineTo fra numeri int.
       dtPoly}; //assenza di filtraggio e uso di Polygon invece di drawPath.

struct SXYValues{
     float *X,  //Pointer to an array containing the x-values
     **Y; //Pointer to an array  the y-values  y[3][4] is the the value of the 4th plot
          //belonging to the 3rd file
};

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

class CLineChart:public QLabel
{
    /* La classe è molto articolata, e i suoi contenuti sono elencati secondo il seguente ordine:
      0) macro
      1) definizione di enum, strutture e classi (compresa CFilterClip)
      2) variabili implementate come property
      3) variabili private
      4) funzioni private
      5) variabili pubbliche
      6) funzioni pubbliche
      7) funzioni di accesso properties
      8) signals
      9) private slots
*/

// *************  0) MACRO
    Q_OBJECT
    Q_PROPERTY(int activeDataCursor  READ giveActiveDataCurs WRITE setActiveDataCurs)

// *************  1) DEFINIZIONE DI ENUM E STRUTTURE
    enum EadjustType{atLeft, atRight, atCenter};
    enum ELegendFontSizeType {EqualToAxis, LAuto};
public:
    enum EAxisType {atX, atYL, atYR};
private:
    struct SDigits{
        int i1,i2,i3,i4;
      int ie;
        float Value, roundValue;
        char Sign;
    };
    struct SAxis{
      /* Tutti i dati sono relativi a scale lineari eccetto eMin ed eMax*/
      bool
      addZeroLine, //Se attiva aggiungo una riga "del colore degli assi"  che segna la
                   // coordinata "0" Se è true sull'asse y sarà tracciata la retta y=0
                   // parallela all'asse x.
      cutsLimits, //true se l'estensione della scala arrodondata taglia un pezzo
                  // di quella completa
      halfTicNum; //se  attiva, dimezzo i numeri sulle tacche quando metto
                  //un elevato numero di tacche per far comparire anche la tacca sullo 0
      enum EScaleType scaleType; //dice se ho scala lineare, DB o logaritmica pura
      int eMin,   //potenza di 10 relativa all'estremo inferiore di scala logaritmica
          eMax,   //potenza di 10 relativa all'estremo superiore di scala logaritmica
          done,  //se è 0 i due estremi, arrotondati a quattro cifre significative, sono uguali,
                 //in tal caso di usano due sole tacche, in posizioni convenzionali;
                 //se è 1, sono riuscito a trovare due estremi distinti ma non "rotondi",
                 //è 2 se il risultato è OK, cioè se ho eseguito la scalatura in modo
                //"exactMatch", oppure ho trovato i due numeri rotondi.
          widthPix, //ampiezza dell'asse in pixel: X1-X0, o Y1-Y0
                 //(in realtà sull'asse X c'è una piccola correzione nel solo caso delle bar).
          maxTextWidth, //massima ampiezza in pixel dei testi sugli assi, comprendendo
                    // i numeri sulle tacche e le eventuali label
          ticPixWidth, //ampiezza delle tacche in pixel
          ticDecimals, //Numero di decimali sulle label numeriche
          scaleExponent; //la scala si intende moltiplicata per 10^ScaleExponent
      float scaleFactor, //Fattore di scala per evitare numeri troppo grossi o piccoli
                          //sulle tacche degli assi  = 10^ScaleExponent
            ticInterval,    //distanza numerica fra le tacche (unità della grandezza)
            ticIntervalPix,
            pixPerValue; //rapporto fra widthPix e scaleFactor*(scaleMax-scaleMin) se in scala lineare
                         //rapporto fra widthPix e scaleFactor*(eMax-eMin) se in scala log
      float minF,  //=minVal*scaleFactor
            maxF;  //=maxVal*scaleFactor
      float minVal, // minimo effettivo dell'asse senza arrotondamento
            maxVal;  // massimo effettivo dell'asse senza arrotondamento
      float scaleMin,  //valore numerico visualizzato sulla label all'estremo SX del grafico
            scaleMax;  //valore numerico visualizzato sulla label all'estremo DX del grafico
            /*NOTA: Nel caso di grafico logaritmico ScaleMin e ScaleMax sono i valori
                   effettivi prima dell'estrazione del logaritmo  */
         enum EAxisType type;
    };
    struct SMarkPts{
        int indexSX[MAXFILES][MAXMANUMARKS], //indice delle matrici dei punti immediatamente a sinistra del punto su cui si mettono i marcatori
            lastMark; //Indice dell'ultimo marcatore manuale inserito
      float fraction[MAXFILES][MAXMANUMARKS]; /*Questo numero precisa l'indicazione fornita da IndexSX: dà la frazione dell'intervallo fra il punto di indice indexSX e indexSX+1; in tal modo ho indicazione precisa della posizione del marcatore, anche quando i punti visualizzati sono pochi. La memorizzazione è anche indipendente        dalla risoluzione del Painter su cui vado a scrivere, e quindi può essere anche utilizzata per la stampa.
        */
    };
    struct SMinMax {float Min, Max;}; //contiene i valori minimi e massimi ad es. di un array.


/*$$$*********************************************************************************/
/***********Inizio definizione della classe FilterClip*******************************/
/*************************************************************************************/
class CFilterClip{
/* Questa classe implementa le funzioni necessarie per
  1. filtrare da grafici punti ridondanti
  2. fare una ClipRegion manuale in quanto wuella standard di Windows non ha effetto
     sulle metafile una volta che queste ultime siano state aperte dal programma di
     destinazione.
  Per quanto riguarda la funzione 1 viene definita una striscia intorno ad una retta:
  se un dato punto dsi trova entro la striscia definita dai due punti precedenti
  può essere filtrato via.
    OCT '98. L'algoritmo della striscia semplice funziona erroneamente in alcuni casi:
      se ad es. ho un overshoot di un solo pixel e poi si torna indietro restando nella
      striscia se non adotto correttivi il punto di picco dell'overshoot viene escluso dal
      grafico. Pertanto devo includere nel grafico oltre che i punti fuori della striscia
      anche quelli che "tornano indietro". Questo risultato lo ottengo considerando il
      segno del prodotto scalare di un vettore rappresentativo, in direzione e verso,
      della retta (LineVector), e un vettore congiungente penultimo e ultimo punto
      considerato.
  Per quanto riguarda la funzione 2 essa  implementata nel metodo "GiveRectIntersect"
  che dà l'intersezione di una retta orientata con un rettangolo.
*/
  public:
    bool strongFilter; //Se  true vuol dire che sto facendo un Copy e il LineChart
                       //di cui FC fa parte ha StrongFilter=true.
    struct FloatPoint{float X,Y;};
    float maxErr; //SemiLarghezza della striscia
    CFilterClip(void);
    bool getLine(float X1, float Y1, float X2, float Y2); //si passano le coordinate
        //di due punti per cui passa la retta, e si definisce in tal modo la retta di
        //riferimento. Se i due punti passati erano coincidenti, la retta non  definibile
        //e la funzione ritorna false;
    void getRect(int X0, int Y0, int X1, int Y1);
    int giveRectIntersect(FloatPoint & I1, FloatPoint &I2);
    bool isRedundant(float X, float Y); //ritorna true se il punto passato non  all'interno della striscia
    bool isInRect(float X, float Y); //ritorna true se il punto passato  all'interno del rettangolo
  private:
    struct FloatRect{	float Top, Bottom, Left, Right;};
    float X1,Y1,X2,Y2;
    FloatRect R; //Rettangolo del grafico su cui effettuare il taglio delle curve
    FloatPoint Vector;  //vettore che mi dà direz. e verso della StraightLine
    bool lineDefined;
    float A, B, C,  //coefficienti dell'equazione della retta e sqrt(A^2+B^2)
          aux, lastX, lastY;
    float inline giveX(float Y), giveY(float X);
    bool giveX(float Y, float &X), giveY(float X, float &Y);
};
/*$$$***********************************************************************/
/*************Fine definizione della classe FilterClip**********************/
/***************************************************************************/


/*$$$*********************************************************************************/
/***********Inizio definizione della classe FilterClipD*******************************/
/*************************************************************************************/
class CFilterClipD{
/* Classe variante di CFilterClip, creata nel Dicembre 2018.
 * L'obiettivo per la quale è stata realizzata era verificare se l'uso di numeri double
 * velocizzasse o rallentasse l'esecuzione.
 * La domanda era legittima in quanto il lineTo() di painter richiede dei double, e l'uso
 * di float comporta continue conversioni di formato. L'utilizzo estensivo di double,
 * però comporta di lavorare con uin numero maggiore di bytes per numero, e comunque di
 * fare conversione dai dati interni delle matrici contenenti i risultati delle simulazioni
 * da fload a double.
 *
 * Il risultato è stato sorprendente (e riproducibile attraverso test TestLineChart) i tempi
 * di calcolo sono paragonabili nei due casi se uso linee di spessore di un pixel, mentre
 * sono 50 volte più lenti se uso linee a due pixel. In sostanza si ottiene la seguente
 * situazione:
 * - i grafici con drawCurves() e CFilterClip e numeri sempre float sono paragonabili nei
 *   tempi a quelli ottenibili con tutte le atre funzioni: drawCurvesD(),
 *   drawCurvedPoly(), drawCurvesQtF(), drawCurvesQtI()
 * - i grafici con drawCurves() e CFilterClip e numeri sempre float sono enormemente più
 *   veloci (50 volte) di quelli ottenibili con tutte le atre funzioni: drawCurvesD(),
 *   drawCurvedPoly(), drawCurvesQtF(), drawCurvesQtI()
 * La ragione di queste forti differenze è per me oscura. Comunque la soluzione da
 * scegliere è semmplice:
 * - usare sempre drawCurves()
 * - lasciare nel codice le altre funzioni, per futuri test.
 *   */
    public:
      bool strongFilter; //Se  true vuol dire che sto facendo un Copy e il LineChart
                         //di cui FC fa parte ha StrongFilter=true.
      struct DoublePoint{double X,Y;};
      double maxErr; //SemiLarghezza della striscia
      CFilterClipD(void);
      bool getLine(double X1, double Y1, double X2, double Y2); //si passano le coordinate
                //di due punti per cui passa la retta, e si definisce in tal modo la retta di
                //riferimento. Se i due punti passati erano coincidenti, la retta non  definibile
                //e la funzione ritorna false;
      void getRect(int X0, int Y0, int X1, int Y1);
      int giveRectIntersect(DoublePoint & I1, DoublePoint &I2);
      bool isRedundant(double X, double Y); //ritorna true se il punto passato non  all'interno della striscia
      bool isInRect(double X, double Y); //ritorna true se il punto passato  all'interno del rettangolo
    private:
      struct DoubleRect{double Top, Bottom, Left, Right;};
      double X1,Y1,X2,Y2;
      DoubleRect R; //Rettangolo del grafico su cui effettuare il taglio delle curve
      DoublePoint Vector;  //vettore che mi dà direz. e verso della StraightLine
      bool lineDefined;
      double A, B, C,  //coefficienti dell'equazione della retta e sqrt(A^2+B^2)
             aux, lastX, lastY;
      double inline giveX(double Y), giveY(double X);
      bool giveX(double Y, double &X), giveY(double X, double &Y);
  };
/*$$$***********************************************************************/
/*************Fine definizione della classe FilterClipD**********************/
/***************************************************************************/







// *************  2) VARIABILI PRIVATE IMPLEMENTATE COME PROPERTY
  int activeDataCurs; //Il cursore dati attivo di massimo numero (se  2 o 3  attivo anche il cursore 1!)
  EPlotPenWidth PPlotPenWidth;


// *************  3) VARIABILI PRIVATE nell'ordine: bool, int, mie strutture, strutture Qt
  bool
    adjustingTitleSize, //Se è true è in atto un eventuale adattamento del grafico all'altezza del titolo
    autoMark,
    copying, //true se è in atto un'operazione di "Copy"
    dataCursDragging, //se è true sto trascinando il rettangolo per la visualizzazione dati
    dataCurs2Dragging,
    dataCursVisible, dataCurs2Visible,
    dataGot, // se  true i dati sono stati acquisiti
    defaultFO[MAXFILES+1], //L'elemento di indice nFiles fa riferimento alla variabile
      //sull'asse x; gli altri alle variabili dei vari files. Se un valore  true
      //la variabile x, ovvero le variabili provenienti da un certo files hanno tutti
      //Factors unitari e Offsets nulli
    lastAutoScale, //ultimo valore di autoScale (argomento di plot() )usato
    makingSVG,
    plotDone,  // falso solo all'inizio quando nessun grafico è visualizzato;
    printing, //true se sono in fase di stampa
    rectTTVisible, //se il rettangolo del tooltip è visibile
    strongFilter, //Se è true durante il Copy faccio un filtraggio dei dati "strong"
    variableStep1, // se è true il file  a passo variabile
    writeTitle1,  //se true visualizzerò il titolo
    zoomed, //se true il diagramma è soomato
    zoomSelecting; //sto facendo il drag per individuare il rettangolo del grafico

  int dataCursSelecting, //indica su quale cursore dati ero quando ho messo il cursore mouse a freccia:
                         // serve quando faccio click in prossimità di un cursore dati
      generalFontPx,
      lastDataCurs, //indice numerico dell'ultimo visualizzatore dati considerato
      legendFontSize,
      legendHeight, titleHeight,titleRows,
      /*La seguente variabile  non nulla soltanto per i diagrammi a barre. E' un margine
      che va aggiunto a SX a X0 e tolto a DX a X1 per fare in modo che la prima e l'ultima
      barra non debbano avere ampiezza dimezzata per non sforare fuori il rettangolo
      X0Y0-X1Y1.
      */
      margin,
      maxAutolineWPoints, // Numero massimo con cui si fa il grafico con spessore auto. Infatti con Qt, nel caso di MorkSwarm, a spessore aumentato il tempo del grafico è altissimo (4s contro 50ms di BCB)! Questo problema non si può risolvere con path.simplified() che da una parte non migliora la velocità di esecuzione, ma in compenso traccia anche una linea erronea (che rende il path un percorso chiuso).
      minXTic, minYTic, //Numero minimo di tacche consentite dall'attuale dimensione
                        // del grafico rispettivamente sull'asse X e Y
      myDPI, //numero effettivo di logical dots per inch
      nFiles, //Numero di files delle variabili da plottare
      nPlots1, //cella contenente l'informazione "nPlots" per l'uso di funzioni relative al caso di file singoli
      numOfTotPlots, //Numero di grafici visualizzati
      numOfVSFiles, //Numero di files a passo variabile aventi variabili visualizzate
      smallHSpace, //circa mezzo carattere di spaziatura con GeneralFontSize (orizzontale)
      svgOffset, //Serve per correggere un errore non chiarito del tracciamento SVG
      swarmPointWidth,
      textHeight, //Altezza testo valori numerici degli assi
      X0,Y0,X1,Y1, //ascisse e ordinate del rettanglolo del grafico;
      xVarIndex, //Indice che nel vettore delle variabili  selectedVarNames indicizza il nome della variabile dell'asse x
      **pixelToIndexDX; //matrice che punta, per ogni pixel, all'indice del punto immediatamente alla sua destra (solo per le variabili variableStep). I pixel si incominnciano a contare dal rettangolo del grafico, cioè da X0. Quindi quanto l'indice di pixel è 0 mi riferisco al segmento verticale di sinistra.
  int *startIndex, *stopIndex, //Indici di inizio e fine di grafico in caso di zoom
      xStartIndex[MAXFILES], //ascissa in pixel del primo punto visualizzato (che corrisponde a startIndex[0]. Originariamente è stato messo per individuare quando il cursore si trova fuori range nel caso di più grafici da di versi files con diversi range asse x.
  // A partire da Apr 2018 si cercherà di usarlo anche nel caso in cui con scala logaritmica il grafico non parte proprio dall'origine dell'asse x, quindi xStartIndex non è X0. Questo accade per evitare, in un caso molto particolare, che vanga valutato il logaritmo di 0.
      xStopIndex[MAXFILES]; //ascissa in pixel dell'ultimo punto visualizzato (che corrisponde a stopIndex[0])
  float aspectRatio;  //altezza/larghezza
  float markHalfWidth; //metà della dimensione orizzontale dei Mark

  float lastXCurs[3]; //serve per riposizionare i cursori durante il dimensionamento
  float onePixDPI; // DPI/96: fattore di conversione per convertire un pixel nel suo equivalente a DPI aumentati
  float **px, ***py; //Matrici dei valori asse x e asse y;
  float *cursorXValues,	//Valori da utilizzare per il cursore numerico
               **cursorYValues; //Valori da utilizzare per il cursore numerico
  float *cursorXValBkp,	//Valori di salvataggio del cursore numerico
               **cursorYValBkp; //Valori di salvataggio del cursore numerico
  ELegendFontSizeType legendFontSizeType;
//  SCurveParam *curveParam;
  QList <SCurveParam> curveParamLst;
  QVector <int>   nPlots; //Vettore dei numeri di grafici per i vari files

  SXVarParam xVarParam;
  SAxis xAxis, yAxis, ryAxis;
  SMarkPts manuMarks; //Posizioni dei marcatori manualmente fissati
//  SMinMax xmM, lYmM, rYmM;
  SFloatRect2 dispRect; //Rettangolo di visualizzazione attuale
  QString baseFontFamily;
  CFilterClip FC;
  CFilterClipD FCd;
  struct SUserUnits {QString x, y, ry;} userUnits;
  QCursor myCursor;
  QFont numFont, baseFont, expFont, lgdFont; //Loro utilizzo in Developer.odtQuesti tre font vengono definiti solo nel design plot. Essi sono funzione della dimensione del grafico. se é fontSizeType==fsFixed, allora il baseFont assumerà come pixelSize fixedFontPx
  QList <SFileInfo> filesInfo;
  QRect hovVarRect; //contiene la posizione del rettangolo in cui è posizionato il nome della variabile su cui si sta facendo Hovering. Serve per facilitare il debug del tooltip con nme completo per funzioni di variabili. Non è escluso che sia utile anche dopo che questa fuznione di debug si sia esaurita.
  QRect tooltipRect; //rettangolino rosso del punto del tooltip
  struct SHoveringData {int iTotPlot; QRect rect;};
  QList <SHoveringData> hovDataLst; //informazioni sulle  aree in cui sono posizionati i nomi delle variabili, per consentire informazioni aggiuntive quando si fa l'hovering.
  QImage * myImage;
  QPainter *myPainter; //painter su cui opera il comando plot()
/* Nota: lo stesso myPainter viene usato sia per le scritture su myImage (quindi a schermo) che su Svg e sulla stampante: semplicemente si associa questo painter a tre differenti paint devices! L'unico globale di CLineChart  myImage; l'svg generator e la stampante vengono definiti e usati all'interno di makeSvg() e di print().
*/
  QPen framePen, gridPen, plotPen, ticPen, txtPen;
  QPointF stZoomRectPos, endZoomRectPos;
  QPointF markPositions[MAXVARS]; //Posizioni dei marcatori sui nomi delle variabili
/* In debugRect copierò sempre dataCurs in quanto a seguito di un bug non chiarito capita che all'evento paint vengano spesso inviati dei dataCurs con valore x  errato!*/
  QRect debugCurs;
  QRect dataCurs, dataCurs2;
  QRect plotRect; //Normalmente contiene geometry(); Ma nel caso della stampa contiene printer->pageRect();
  QStack <SFloatRect2>  plStack;
  QString baseFontName, symbFontName, titleText;
  QTimer * myTimer;
  //QString sPlotTime;
  QRectF titleRectF;

// *************  4) FUNZIONI PRIVATE in ordine alfabetico

  char **CreateCMatrix(long NumRows, long NumCols);
  float **CreateFMatrix(long NumRows, long NumCols);
  int **CreateIMatrix(long NumRows, long NumCols);
  void DeleteCMatrix(char **Matrix);
  int DeleteFMatrix(float **Matrix);
  void DeleteIMatrix(int  **Matrix);
  void drawAllLabelsAndGrid(SAxis axis);
  void drawAllLabelsAndGridDB(SAxis axis);
  void drawAllLabelsAndGridLog(SAxis axis);
  bool allowsZero(float MinRoundValue, float MaxRoundValue,int ntic);
  void beginRounding(SDigits &Min, SDigits &Max, float MinVal, float MaxVal,unsigned Include0);
  int computeTic(float minRoundValue_, float maxRoundValue_, int halfTicNum_),
      computeDecimals(float scaleMin, float ticInterval, bool halfTicNum);
  void designPlot(void);
  void drawBars(void);
  int drawCurves(bool NoCurves);
  int drawCurvesD(bool NoCurves);
  void drawCurvesQtF(bool NoCurves);
  void drawCurvesQtI(bool NoCurves);
  void drawCurvesPoly(bool NoCurves);
  void drawMark(float X, float Y, int mark, bool markName);
  void drawSwarm(void);
  int writeText2(QPainter *myPainter, int X, int Y, EadjustType hAdjust, EadjustType vAdjust, QString msg1, QString msg2, bool addBrackets, bool Virtual);
  int smartWriteUnit(QPainter * myPainter, QFont baseFont, int X, int Y, EadjustType hAdjust, EadjustType vAdjust, QString text,  bool addBrackets, bool Virtual );
  bool fillPixelToIndex(int **pixelToIndexDX);
  bool fillPixelToIndexLog(int **pixelToIndexDX);
  //Funzione che dà i valori delle X e delle Y in corrispondenza di una data posizione
  //in pixel del cursore. I bool dicono se devo dare solo le differenze rispetto ai
  //valori in corrispondenza del cursore primario:
  SXYValues giveValues(int cursorX, bool interpolation, int &nearX, bool xDiff, bool yDiff);
  SXYValues giveValues(int cursorX, bool interpolation, bool xDdiff, bool yDdiff);
  SFloatRect2 giveZoomRect(int startSelX, int startSelY, int X, int Y);
  QString goPlot(bool Virtual, bool includeFO);
  struct SMinMax findMinMax(float * vect, int dimens);
  void keyPressEvent(QKeyEvent * event) override;
  void mark(bool store);
  void markAll();
  //Funzione per mettere un singolo marcatore in corrispondenza della posizione
  //orizzontale del cursore e del file e del grafico specificati attraverso i primi due
  //parametri passati:
  void markSingle(int iFile, int iVSFile, int iPlot, int iTotPlot, bool store);
  static float minus(struct SDigits d, int icifra, int ifrac);
  static float plus(struct SDigits d, int icifra, int ifrac);
  void resizeEvent(QResizeEvent *) override;
  void selectUnzoom(QMouseEvent *event);
  SFloatRect2 setFullDispRect();
  int scaleAxis(SAxis &Axis, float minVal, float maxVal, int minTic, unsigned include0, bool exactMatch);
  int scaleXY(SFloatRect2 &dispRect, const bool justTic);
  void setRect(QRect r);
  QTimer * tooltipTimer;
  int writeAxisLabel(int X, int Y, SAxis &Axis, bool Virtual);
  void  writeLegend(bool _virtual);

protected:
//  bool event(QEvent *event)Q_DECL_OVERRIDE;
  bool event(QEvent *event) override;
  void leaveEvent(QEvent *) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *ev) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

public:
// *************  5) VARIABILI PUBBLICHE
/*** Variabili pubbliche implementate in BCB come property: ***/
  bool
  addLegend, //se è true scrivo la leggenda
    blackWhite, //se  true si tracciano tutte le curve in nero
    cutsLimits, //true se l'estensione di una delle scale è inferiore alla piena estensione
    forceYZero,//forza lo 0 sull'asse y
    linearInterpolate, //Se è true con cursore faccio l'interpolazione lineare
    twinScale, //se è true il grafico ha anche la scala verticale destra
    useBrackets, //se è true vengono usate parentesi tonde intorno all'unità di misura
    useSmartUnits, //se è true le unità di misura posssono contenere lettere greche, esponenti, e prodotti col puntino
    xGrid, //se è true si mette la griglia sull'asse X
    yGrid; //se è true si mette la griglia sull'asse Y

  EDrawType drawType;
    enum EFontSizeType {fsFixed, fsAuto};
    EFontSizeType fontSizeType;
    EPlotType plotType;
    ESwarmPointSize swarmPointSize;

// Altre variabili pubbliche
bool  exactMatch, //se true DispRect  correntemente visualizzato in modalità "ExactMatch".
     useUserUnits, //se l'utente vuole scegliere le unità di misura a prescindere da quello che è passato mediante xVarParam e curveParam
      autoLabelXY, //se  true vengono utilizzate le unità di misura automatiche, sulla base dei dati reperibili dai files di  input. "V", "A", ecc e le potenze di 10 vengono convertite in prefissi (tipo m per 10^-3 o k per 10^3).
      enableTooltip; //Se true viene visualizzato il tooltip dei valori dove indicato dal mouse
      //Se  true vengono messe sul grafico label definite dall'utente
int drawTimeMs; //drawing time in milliseconds
int drawTimeUs; //drawing time in microseconds
int fixedFontPx; //default pixel size of text font, when "fsFixed" is selected by the user
int pointsDrawn; //Numero di punti utilizzati per il tracciamento
int tooltipMargin; //distanza  in pixel dal punto per visualizzare il tootip dei valori
bool showPlotCopiedDlg; //mostra il dialog "plot copied as an image into the system clipboard"

// *************  6) FUNZIONI PUBBLICHE (in ordine alfabetico)
  CLineChart(QWidget * parent);
  void copy();
  void disableTitle();
  void enableTitle();
  //funzione per l'utilizzo elementare di lineChart: associabile naturalmente a plot() (senza parametri passati).
  void getData(float *px_,float **py, int nPoints_, int nPlots_=1);
  //Funzione per il passaggio dati all'oggetto relativa al caso di file singoli:
  void getData(SFileInfo FI, int nPlots_,  SXVarParam xVarParam_, QList<SCurveParam> &lCurveParam_, float *px_, float **py_);
  //Funzione per il passaggio dati all'oggetto relativa al caso di file multipli:
  void getData(QList <SFileInfo> FIlist, const QVector <int> &nPlots,  SXVarParam xVarParam, QList<SCurveParam> curveParam, float **px, float ***py);
  void getUserUnits(QString xUnit, QString yUnit, QString ryUnit);
  SFloatRect2 giveDispRect();
  QImage *  giveImage();
  int giveNearValue(QPoint mouseP , QPoint &nearP, QPointF &valueP);
  QString givexUnit();

  SFloatRect2 giveFullLimits();

  bool isZoomed();
  QString makePng(QString fullName, bool issueMsg);
  QString makeSvg(QString fullName, bool issueMsg=true);
  void mark(void), markAuto(void);
  QString plot(bool autoScale=true);
  QString print(QPrinter * printer, bool thinLines);
  void resetMarkData();
  void setDispRect(SFloatRect2 rect);
  void setPlotPenWidth(EPlotPenWidth myWidth);
  void setXScaleType(enum EScaleType xScaleType_);
  void setYScaleType(enum EScaleType yScaleType_);
  void setXZeroLine(bool zeroLine_);
  void setYZeroLine(bool yZeroLine_);

// *************  7) FUNZIONI DI ACCESSO PROPERTY
  EPlotPenWidth givePlotPenWidth() const;
  int giveActiveDataCurs();
  void setActiveDataCurs(int activeCurs);

  // *************  8)   SIGNALS
  signals:
  void chartClickedOn(void);;
  void chartResizeStopped(void);
  void valuesChanged(SXYValues values, bool hDifference, bool vDifference);

  // *************  9)   PRIVATE SLOTS
  private slots:
  void checkTooltip();
  void resizeStopped();

};

#endif // CLINECHART_H
