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

#include <stdio.h>
#include <QFileInfo>
#include <QString>
//#include "Globals.h"
//---------------------------------------------------------------------------
enum EFileType {ADF, COMTRADE, CSV, MAT_V4, MAT, MAT_Modelica, PL4_1, PL4_2, LVM};
enum ERunType {rtTimeRun, rtFreqScan, rtHFreqScan, rtUndefined};
enum EModHFS {mhMagnitude, mhPolar, mhRealIm, mhPolRealIm};
struct SReturn{int code; QString msg;}; //valore di ritorno per talune funzioni, quale ad es. loadFromPl4File: contiene un codice di severità: 0 warning, 1 errore che non fa uscire dal programma, 2 errore che fa uscire dal programma

//La seguente struttura potrà essere usata forse in futuro per tutti i formati di files, con conseguente eliminazione dell'array di stringhe varNames (sostituendolo con un array di SVar). Per ora, per evitare di fare una modifica drammatica tutta insieme con conseguenze potenzialmente drammatiche, la introduco solo per i files di Modelica, che ne fanno un uso pieno.
struct SVar{
    QString name; //nome della variabile
    QString description; //descrizione della variabile in alcun formati (es.Modelica)
    QString unit; //unità di misura in alcuni formati (ad es. Modelica)
    int infoIndex; //indice di dataInfo che caratterizza la variabile nel file di input
};

struct DataFromModelicaFile{
  //Struttura compilata da inputMaatModelicaFile
  QStringList descriptionLst;
  QStringList namesLst;
  QStringList unitLst;
  QString retString;
  int dataInfoRows;
  int numOfData2Rows, numOfData2Cols;  //Numero di righe e colonne di data_2
  int numOfAllVars;  //Numero di tutte le variabili incluso le alias (esclusi i parametri)
  int **dataInfo;
  float ** data_1;
  float ** data_2;
};

struct ParamInfo{
    //Struttura contenente tutte le informazioni necessarie per visualizzare i parametri (e le altre variabili che non variano durante la simulazione). Essa è disponibile solo se il file in questione è un file MatModelica, e in particolare esso è letto con l'opzione allVars=false. E' compilata all'interno di loadFromModelicaMatFile(File **, bool).
    QStringList names, description, units;
    QList <float> values;
    QList <int> code; //1 se ho messo un valore nelle liste; 2 se ho messo anche *** ALIASES ***
};

class CSimOut{
    /* Classe definente gli oggetti di tipo SimOut, contenenti i dati di output di una
    simulazione, ed i relativi nomi delle variabili corrispondenti.
    Essa  pensata per agevolare al massimo la lettura dei dati da file, il quale può essere
    di tipo:
    - ADF, così come definito da apposito documento di specificazione
    ovvero:
    - pl4, file di uscita binario di simulazioni ATP
    - mat, file binario di matlab:
           . formato 4.0 completamente supportato
           . formato 5.0 completamente supportato se nel PC Windows è presente MATLAB
           . formato 5.0 supportato in assenza di matlab sul PC solo se non compresso
   - lvm  file di testo di uscita di LabView, supportato solo per i casi più comuni
          attualmente (Nov 2012) testati
   - cfg  file comtrade Supportati ascii e binario)

    ***
    Per quanto riguarda i nomi dei files di tipo pl4, riportati nel vettore varNames, sono
    composti come segue:
    1) la variabile tempo ha il nome "t";
    2) le variabili di ramo (tensioni e correnti di ramo) hanno nomi secondo la seguente
            struttura:
            <codice1 (%2d)> + ": " + <codice2 (%6d)> + "-" + <codice3 (%6d)>
    3) le altre variabili hanno nomi secondo la seguente struttura:
            <codice1 (%2d)> + ": " + <codice2 (%6d)>
    Si riporta nel seguito l'ordine di immissione variabili nella matrice y di uscita e i
    relativi campi di varNames:
    #    Tipo                              Codice1        Codice2              Codice3
    - Variabile tempo
    0 tensioni di nodo ("Node Voltage")     "v"        <nome nodo>                -
    1 tensioni di ramo ("Branch Voltage")   "v"        <nome nodo1>         <nome nodo2>
    2 correnti/potenze/energie di ramo      "c"        <nome nodo1>         <nome nodo2>
                                ("Branch current")
    3 variabili TACS ("TACS Variable")      "t"     <nome variabile TACS>         -
    4 variabili MODELS ("MODELS Variable")  "m"     <nome variabile MODELS>       -
    5 variabili SM (macchine sincrone)      "s"+#    <nome variabile SM>          -
    6 variabili UM (macchine universali)    "u"+#    <nome variabile UM>          -
dove con # si  indicato un numero di una cifra indicativo della macchina (sincrona o
universale) considerata.
*/
  private:
    class CStrTok{
      /* La presente classe serve per gestire i campi dei file Comtrade.
         La routine strtok() non può essere utilizzata in quanto se essa trova due
         separatori adiacenti li considera un unico separatore e questo non è ammissibile
         per i files Comtrade.
       */
       public:
         int commas;
         void getStr(char * Str_);
           char * giveTok(void);
           CStrTok(void);
           ~CStrTok();
       private:
          bool lastTok, endStr;
          char separ, *string, *lastBeg, *lastEnd; //Last Beginning
    } StrTok;

    int fileLength;
    /* Il seguente vettore serve solo per il trattamento dei files Comtrade in ingresso:
    se una variabile ha unità di misura del tipo kV o mA, deve essere convertita
    rispettivamente in V e A. Al momento della lettura dell'unità di misura, pertanto,
    viene settato il corrispondente elemento di questo vettore in modo che dopo la lettura
    del file CFG si possa effettuare agevolmente la correzione.
    */
    float *factorUnit;
    struct Fmatrix {
      long type,nRows,nCols,imagf,namlen;
    } header;
    ParamInfo paramInfo;

    void addPrefix(QString &VarName, QString Unit, QString CCBM, int Var);
    struct DataFromModelicaFile  inputMatModelicaData(FILE * pFile);
    QString giveAutoUnits(QChar c);


public:
    bool ObjectNames,
         allowsPl4Out, // true solo se l'input è un run-time di ATP in file PL4
         commasAreSeparators, //accetta virgole come separatori nei files ADF
         trimQuotes, //solo in cvs, se è true toglie le quotes a inizio e fine
         useMatLib, //  usa librerie Matlab per file V5 invece del mio file di interpretazione
         variableStep; //se  true si tratta di un tipo di file sul quale  consentito
         //di avere un passo di integrazione costante
    int lastFileSize;
    int numOfVariables, //Numero di variabili dell'oggetto
        blockNumOfVariables[7], //Per ora utilizzato per i soli files pl4
           //danno i numeri delle variabili dei vari gruppi sopra specificati (da 0 a 6)
        numOfPoints,  //Numero di punti per ciascuna variabile
        timeVarIndex; //indice della variabile tempo (unica supposta monotona) è -1 se nessuna variabile è supposta monotona
    EFileType fileType;
    ERunType runType;
    //Le seguenti due variabili servono solo per fare salvataggi corretti di variabili
    //prelevate da files Pl4 (attualmente, GIU 04, sono inutilizzate):
//    int IHSPl4;
//    EModHFS ModHFS;
    float **y; //Conterrà la matrice dei dati (per righe)
    QWidget * parent;
    QString Date, Time;
    QString  *varNames;  //array dei numOfVariables nomi delle variabili
    SVar *sVars; //array di strutture SVar
    QFileInfo fileInfo;  //nome, data, attributi, ecc. del file
    CSimOut(QWidget *);
    ~CSimOut();

    struct ParamInfo  giveParamInfo();

    QString loadFromAdfFile(QString fullName, bool csv=false);
    /* Function per la lettura delle informazioni da un file avente la semplice struttura
    che ho definito per l'estensione ADF (Ascii Data File):
    - una prima riga contenente i nomi delle variabili che si trovano nelle suggessive colonne,
      separate da spazi (' ' o tab)
    - i valori delle variabili, in ascii, strutturati a matrice (una colonna per variabile)
    Codici di ritorno:
    - "" per OK
     -"Unable to open file (does it exist?)";  in caso di problemi
    */
    QString loadFromComtradeFile(QString cfgFileName);
    QString loadFromLvmFile(QString FileName);
    QString loadFromMatFile(QString fileName, bool allVars_, bool addAlias_);
    QString loadFromMatFile4(QString fileName, bool allVars_, bool addAlias_);
    QString loadFromMatFile5(QString fileName);
    QString loadFromMatFileLib(QString fileName);
    QString loadFromModelicaMatFile(FILE * pFile);
    QString loadFromModelicaMatFile(FILE * pFile, bool addAlias_);
    SReturn loadFromPl4File(QString fileName);
    QString loadFromPl4Type1(FILE * fpIn);
    QString loadFromPl4Type2(FILE * fpIn);
    QString namesAdfToMat();
    /* Questa routine converte i nomi delle variabili originariamente lette da un file ADF allo standard Matlab.
       Le modalità di conversione sono chiarite nel documento Conversion.doc
    */
    QString namesComtradeToMat(bool mat);
    /* Questa routine converte i nomi delle variabili originariamente lette da un file
        COMTRADE dallo standard Simout, che è quello utilizzato in PlotXY allo standard Matlab, che è quello utilizzato in Pl42mat.
        Le modalità di conversione sono chiarite nel documento Conversion.doc
    */
    QString namesPl4ToAdf(int addDigit);
    /* Questa routine converte i nomi delle variabili originariamente lette da un file Pl4
    allo standard Adf, che è quello utilizzato in PlotXY.
    Le modalità di conversione sono chiarite nel documento Conversion.doc
    Inoltre se AddDigit è >0 viene appeso al nome il digit corrispondente al valore
    numerico di Converted.
    */
    QString saveToAdfFile(QString fileName, QString comment);
    /* Function per la scrittura delle informazioni su un file di tipo ADF.
    */
    QString saveToAdfFile(QString fileName, QString comment, int nVars, int vars[]);
    /* Function per la scrittura delle informazioni su un file di tipo ADF, selezionando
       le variabili da salvare.
    */
    QString saveToComtradeFile(QString cfgFileName, QString stationName);
    /* Function per la scrittura delle informazioni su un file di tipo ADF.
    */

    QString saveToComtradeFile(QString cfgFileName, QString stationName, int nVars, int vars[]);
    /* Function per la scrittura delle informazioni su un file di tipo ADF, selezionando
       le variabili da salvare.
     */

    QString saveToMatFile(QString fileName);
    /* Function per la scrittura delle informazioni su un file di tipo Matlab 4
     */
    QString saveToMatFile(QString fileName, int nVars,int vars[]);
    /* Function per la scrittura delle informazioni su un file di tipo Matlab 4,
       selezionando le variabili da salvare
    */
    QString saveToPl4File(QString fileName);
    /* Function per la scrittura delle informazioni su un file di tipo Pl4.
    */
    QString saveToPl4File(QString fileName, int nVars, int vars[]);
    /* Function per la scrittura delle informazioni su un file di tipo Pl4, selezionando
    le variabili da salvare.
    */

};

