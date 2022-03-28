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

#ifndef GLOBALS_H
#define GLOBALS_H
#include<QColor>
#include <QDateTime>
#include <QString>
#include <QVector>

// *** Valori limite ad allocazione statica POI METTERE 8 A MAXSHEETS!):
#define MAXPLOTWINS 8
// *** Notare che la seguente MAXVARS è definita canche dentro CLineChart.h. I due valori DEVONO  coincidere!
#define MAXVARS 15  //Numero massimo di variabili nella lista (escluso il tempo)
#define MAXFUNPLOTS MAXVARS  //Numero massimo di funzioni di variabili plottabili simultaneamente
#define VARTABLEPANELS 4


// *** Valori default delle opzioni di programma:
#define AUTOLABELXY true
#define BARCHARTFORFS false
#define BARCHARTFORHFS false
#define COMMASARESEPARATORS true
#define COMPACTMMVARMENU true
#define DEFAULTFREQ 50.
#define ENABLEAUTOREFRESH false
#define LARGERFONTS false
#define MAXSPEED false
#define ONLYPOINTSINPLOTS false
#define USECOPIEDDIALOG true
#define USEOLDCOLORS false
//La seguente variabile contiene il valore di
//enum EPlotPenWidth {pwThin, pwThick, pwAuto};
#define PLOTPENWIDTH 2
//#define USETHINLINES false
// Ricordarsi che il programma richiede modifiche se la seguente variabile è posta in true. Infatti accadrebbe che quando l'utente resetta le opzioni, il registro viene cancellato, ma poi è nuovamente riscritto all'uscita, mentre vogliamo che il reset pulisca anche il registro.
#define REMEMBERWINPOSANDSIZE false
#define SHOWTIME false
#define TRIMQUOTES true
#define USEBRACKETS true
#define USEGRIDS false


// *** Strutture globali:
//struct SReturn{int code; QString msg;}; //valore di ritorno per talune funzioni, quale ad es. loadFromPl4File: contiene un codice di severità: 0 warning, 1 errore che non fa uscire dal programma, 2 errore che fa uscire dal programma

struct SfileRecord{
  QString fullName;
  QDateTime dateTime;
};

struct SOptions{
    bool autoLabelXY, //Mette etichette automatiche sugli assi
    barChartForFS, //il default per frequency scan è diagramma a barre
    commasAreSeparators, //accetta come separatori di campi anch ele virgole; utile per leggere i CSV
    compactMMvarMenu, //compatta il varMenu di file mat-Modelica; utile per leggere i CSV
    largerFonts, //Font di un punto più larghi in DataSel Win: utile su PC con grandi schermi 4k
    onlyPoints, //per default traccia i diagrammmi con i soli punti, senza le linee di collegamento
    rememberWinPosSize, //chiede di memorizzare posizioni e dimensioni delle finestre da un'esecuzione all'altra
    showElapsTime, //chiede di visualizzare il tempo di esecuzione nella sbarra del titolo
    showFullFilelist, // in multifile mostra l'intera lista di 8 file nella fileTable
    trimQuotes, //elimina caratteri virgolette a inizio e fine dei nomi (utile per CSV di Modelica)
    useCopiedDialog, // ussa un dialog per indicare che il grafico è stato copiato nella clipboard
    useGrids, //per default aggiunge le gridlines ai plot
    useMatLib, //usa la libreria matlab., se presente nella lettura dei files mat
    useOldColors, //usa i primi tre colori con logica RGB
    useBrackets;  //usa parentesi intorno alle unità di misura
    int drawType; // corrisponde al valore della variabile enumerata
    int firstFileIndex;
    int plotPenWidth;
    /* AGO 2017 Da oggi uso la seguente frequenza in double anzichhé in float La ragione
       di questo sta nel fatto che con il sistema MAC attivo ad AGO 2017 (Qt 5.7 e Mac OS
       10.12 - Sierra) se uso float lo scambio dell'informazione di frequenza con il
       registro dati non funziona! Funzionava invece correttamente in qnd.
       La conversione in double rende il sistema funzionante in entrambi i casi.
    */
    double defaultFreq; //frequenza di riferimento per il calcolo della DFT
};


/* Variabili globali contenenti informazioni generali che si debbano rendere facilmente accessibili ai vari moduli del programma:
*/


struct SGlobalVars {
  bool multiFileMode;
  int shiftWin, //Variabile globale rappresentante lo spostamento verticale, in pixel, delle finestre principali del programma
      instNum, //Variabile globale rappresentante il numero dell'istanza del programma
      firstNameIndex, //indice fra i parametri passati del primo fileName.
      fileNameNum;  //numero di nomi di file passati fra i parametri
  short int WinPosAndSize[3+10*MAXPLOTWINS];
  QVector <int> varNumsLst;
  struct SOptions PO;
};


enum EAmplUnit {peak, rms, puOf0, puOf1};
enum EAmplSize {fifty, seventy, hundred};

struct SFourOptions{
    enum EAmplUnit amplUnit;
    enum EAmplSize amplSize;
    int harm1, harm2;
    float initialTime, finalTime;
};

struct SFourData{
    bool variableStep;
    int numOfPoints;
    float *x,*y;
    QString fileName, varName, ret;
    SFourOptions opt;
};

#endif //GLOBALS_H
