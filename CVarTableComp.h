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

#ifndef CTABLE_H
#define CTABLE_H
#include <QDataStream>
#include<QTableWidget>
#include<QHeaderView>
#include <QColor>
#include <QList>
#include <QInputDialog>
#include "Globals.h"
#include "CLineChart.h"
//#include "CLineChart.h"
#include "Dialogs/CCustomiseCol.h"

#define COLORCOL 0 //colonna del numero del file
#define FILENUMCOL 1 //colonna del numero del file
#define XVARCOL 4 //colonna dell'indicazione di quale è la variabile x
#define VARNUMCOL 2 //colonna del numero della variabile
#define VARCOL 3 //colonna del nome della variabile

#define TOTCOLS 5 //numero totale di colonne
#define TOTROWS 17 //numero di righe (incluso l'header)
#define MAXVARSPERFUN 10  //massimo numero di variabili accettate in una medesima funzione di variabile

#include "CLineCalc.h"
#include "Dialogs/CFunStrInput.h"
//#include "C:/Qt5Source/calc3/CLineCalc.h"

// Le colonne saranno: colore, N. file, funzione (bool), nome variabile, Xvar

/*
Il nome di questa classe, "CVarTableComp" ricorda quello di CSelVarTable, con l'aggiunta dell'inizio della parola "compatta" in quanto si sopprimono le colonne Factors e Offsets, sostituite da un comportamento più smart del resto delle colonne.

Le innovazioni rispetto alla tabella di selezione variabili di XWin sono:
  1) metto la possibilità  di definire variabili a partire da altre variabili caricate  non limitandomi a somma e sottrazione ma definendo una semplice struttura liguistica che consente  di manipolare grafici usando le quattro operazioni in maniera pressoché arbitraria, incluso l'uso di costanti esplicite, operatori unari e parentesi.
E' inoltre possibile richiedere l'integrale di una variabile.
Sarà a breve possibile anche mescolare grafici provenienti da file diversi a condizione che essi abbiano gli stessi istanti di campionamento.
    2) di conseguenza i Factors e Offsets perdono significato e vengono soppressi
    3) ad ogni riga della tabella corrisponde un colore di visualizzazione. Quando si cancella una variabile si lascia libera la corrispondente riga, ed il colore associato. Di conseguenza attiverò in seguito la possibilità  di fare drag&drop da una riga all'altra di una variabile, e da una riga della ListVar alla SelTable

Questa classe definisce molti elementi del comportamento di una tabella di selezione variabili di programma XWin2012, estendendo per eredità  le funzioni della tabella QTableWidget.
Una cosa particolarmente interessante  che la funzione interpreta il click su una cella contenente il nome di variabile per deselezionare la variabile interessata. Per fare questo si aggancia il segnale clicked() della QTableWidget da cui si eredita, e lo si convoglia al uno slot appositamente creato. Il ::Connect, quindi opera su due indirizzi indicati come "this"; quello del segnale  la QTableWIdget di partenza, quello dello slot la mia estensione.

La gestione della dimensione della classe  anch'essa speciale: si passa al costruttore la larghezza disponibile, e a partire da essa si determinano le dimensioni delle colonne. In tal modo si spera di attivare un meccanismo che vada bene su diversi sistemi target, con differenti sistemi operativi, font, risoluzioni.

Le celle della tabella non sono editabili: le operazioni vengono effettuate tramite click e (forse, in futuro) drag&drop.
*/

struct SVarTableState{
    QStringList allNames;
    QList <int> fileList;
    QVector <QRgb> varColors;
    QVector <Qt::PenStyle> styles;
    QSet <int> funSet;
    bool xIsFunction;
    int xInfoIdx;
};

class CVarTableComp:public QTableWidget
{
    Q_OBJECT
private:
    bool multiFile, commonXSet, monotonic[TOTROWS],
        timeVarReset; //if true, the user has clickes on fhe "X" column to select s horizontal variable a variable different than default's
    int currFileIdx;
    int highestUsedRowIdx; // indice dell'ultima riga con contenuto
    int iniVarNumColWidth; // valore iniziale di varNumCol
    int singleFileNum; //numero del file se in SingleFile
    int styleData;
    int tabNameNum; //number (between 1 and 8) of the tab containining the object: for debug purposes
    int xVarRow;
    double myDPI; //valore del numero di DPI logici del PC: di base in win (non 4k): 96 dpi.
    float **yLine;
    // I vettori yLine[i] contengono valori fittizi che simulano dei puntatori a vettori y. Essi sono usati per effettuare un check sintattico su line. Per semplicità quindi sono tutti composti da un unico elemento il cui valore è pari al suo indice i; il numero di elementi di yLine è stato fissato in MAXVARSPERFUN.

    // Now structured Qt Objects in class alphabetic order:
    QColor headerGray; //Colore grigio della prima riga di intestazione setRgb(230,230,230);
    QFont cellFont;
    QList <int> tabFileNums; //contiene i numeri nella colonna file, eccetto riga var. x. Serve per la gestione dell'abilitazione di saveVars
    QList <int> allFileNums;  //Lista di tutti i numeri di files disponibili, anche non visualizzati nella varTable, utilizzabili nelle funzioni di variabili
    QList <QString> allFileNames;  //Lista dei nomi di files usati nella tabella (eccetto in funzioni di variabili);
    //i nomi sono messi in indici corrispondenti all'indice di riga della tabella in cui il file è usato
    QList <int> varMaxNumsLst; //Lista dei numeri di variabili dei files correntemente visualizzati

    /* La seguente variabile funInfoLst contiene una lista di informazioni delle varie variabili, così come introdotte dall'utente. In questa copia privata di CVarTableComp, varNames equivalenti quali ad es. f1v3 e v3 se f1 è il file corrente vengono mantenute distinte.
Quando però il programma chiamente chiede giveFuninfo, i nomi equivalenti vengono unificati. Infatti vi sarebbe altrienti un problema con la memoria perché i vettori numerici passati per il calcolo sono in numero pari alle variabili effettivamente distinte, mentre e se esistono nomi duplicati si crea un disallineamento pernicioso.*/
    QList <SXYNameData> funInfoLst;
    QSet <int> funSet; //contiene gli indici delle funzioni definite.

    //Now provate functions:
    void resizeEvent(QResizeEvent *);
    QColor colors[TOTROWS], bkgroundColor;
    QBrush xVarBrush;
    QString hdrs[TOTCOLS];
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    void mouseReleaseEvent(QMouseEvent * event);
    void showEvent(QShowEvent *);
    CCustomiseCol *customiseCol;
    CFunStrInput *funStrInput;

public:
    bool allowSaving; //Se è true si possono salvare le variabili: ad oggi questo implica che le variabili devono essere tutte dallo stesso file e non devono esserci funzioni di variabili
    bool isEmpty();
    int numOfPlotFiles; //numero di files di cui è plottata almeno una variabile (esclusi i files fittizi delle funzioni di variabile
    int numOfFuns; // Numero di funzioni di variabile
    int numOfTotVars; //Numero di variabili selezionate (inclusa la x)
    QColor neCellBkColor; //colore delle celle non editabili
    SCurveParam xInfo;  //contiene anche, in "idx", l'indice sequenziale della variabile marcata x (0 se siamo in multifile con variabili da plottare provenienti da più files vale -1 se nessuna var è stata ancora selezionata, quindi anche nessuna variable x).
    QList <SCurveParam> yInfo[MAXFILES];

    CVarTableComp(QWidget *parent);
    ~CVarTableComp();

    //Funzioni pubbliche in ordine alfabetico:
    QString analyse();
    void fillFunNames(void);
    void filterOutVars(QList <QString> varList);
    void getFileNums(QList<int> fileNums, QList<int> varNums);
    void getFont(QFont font_);
    void getVarNumVect(QVector<int> list); //serve per consentire la diagnostica delle funzioni di variabile
    void getColorScheme(bool useOldColors_);

    void getState(QStringList &list, QVector <QRgb> varColRgb, int styleData_, bool xIsFunction_, int xInfoIdx_, bool multiFileMode_);
    int giveFileNum(int row);
    int givehighestUsedRowIdx();
    SVarTableState giveState();
    QList <SXYNameData> giveFunInfo();
    void myReset(bool deep=false);
    int setCommonX(QString str);
    void setNameNumber(int number_); //setst the current index of the container tab
    void setMultiFile(bool multifile_);
    void setPosition(int x, int y); //Riposiziona la tabella senza alterare larghezza e altezza.
signals:
    void numOfTotVarsChanged(int numOfTotVars);
    void queryFileInfo(QList <int> &fileNums, QList <QString> &fileNames, QList <int> &varNums);
//    void checkCalcData(SXYNameData,QString &);
    void contentChanged();

public slots:
    void blankCell();
    void leftClicked(int r, int c); //Implementa la deselezione di variabile come SLOT di clicked()
    void setCurrFile(int fileIdx);
    int setVar(QString varName, int varNum, int giveFileNum, bool rightScale, bool monotonic_, QString unit_);
    int unselectFileVars(int fileIndex);
    int varColumn();
};

#endif // CTABLE_H
