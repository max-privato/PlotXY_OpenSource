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

#ifndef CLINECALC_H
#define CLINECALC_H
#include <QByteArray>
#include <QColor>
#include <QList>
#include <QVector>
#include <QRegExp>
#include <math.h>
#define MAXBINARYOPS 5 //massimo numero di operatori binari
#define MAXFUNCTIONS 13 //massimo numero di funzioni matematiche


/* questa classe realizza un calcolatore di linea che interpreta la sintassi ed effettua calcoli.
  La stringa da interpretare può contenere i segni delle quattro operazioni, le parentesi, il segno unario davanti ad un numero (ad inizio stringa o inizio contenuto di coppia di parentesi).
  Ovviamente è mantenuta la priorità standard:
  - prima i contenuti all'interno delle parentesi; all'interno di esse:
    . prima * e / da sinistra a destra
    . poi + e - da sinistra a destra
  Gli operatori operano su due tipi di dati:
- costanti  Sono costanti esplicite, ad es. 1.0, -1e10, 1e-10
- variabili. Sono dei nomi simbolici che non possono iniziare con un digit.

La classe possiede anche una predisposizione per l'effettuazione del successivo integrale della funzione calcolata. Un integrale viene richiesto passando una linea del tipo:
int(...)
CLineCalc in questo caso si accorge che è richiesto un successivo integrale del contenuto della parentesi, attiva la variabile booleana integralRequest, e poi elimina "int(" e ")", e opera sul contenuto delle parentesi.

*****************
 UTILIZZO:
 1) si passa la stringa (QString) da analizzare attraverso "getLine(QString line)" (in PlotXY quando inserisco la stringa)
 2) si passa l'elenco dei nomi di variabili possibili nameList e della matrice y_ a cui corrispondono i valori, attraverso la funzione getAndPrepare(QList <QString> nameList, float ** y_) (in PlotXY quando comando il plot()). Non è necessario che tutti i nomi di "nameList" siano presenti in "line", ma deve sussistere corrispondenza ordinata fra i nomi di nameList e le righe di y_
 3) OPZIONALE Se non voglio allocare inutilmente righe in y per variabili che poi non sono presenti in "line", posso farmi dare le informazioni necessarie su tali variabili mediante la funzione SXYNameData CLineCalc::giveXYNameData(). Se prima di richiamarla si fa xyNaming=true, verranno accettati solo nomi conformi alla  codifica XY (f#v# e v#); altrimenti i nomi basta che comincino per lettera e contengano lettere o numeri.
 Durante tale chiamata viene anche creata la versione "line1" che rispetto a "line0" non ha le parti dei nomi "f#" se il numero di file è quello di default
 4) OPZIONALE Nel caso di utilizzo all'interno di XY sono separate la fase di accettazione del QString (quando lo si mette nella XVarTable) da quella di calcolo (Quando si effettua il comando plot(). Di conseguenza è stata aggiunta la funzione checkLine()che simula un calcolo per verificare la congruenza sintattica del QString.
 5) si fa result=compute(i); essendo i l'indice del valore da prelevare dalle variabili-funzione.
 N.B. Eventuali messaggi di errore si trovano nella QString, interna ma pubblica, di nome "err"

 **********  Significato delle varie versioni della stringa "line"
 Si hanno varie versioni della stringa passata che rispondono a varie esigenze.
 1) line0 è la riga così come viene passata la prima volta e non viene mai modificata. In realtà viene usata solo nei primissimi istanti per generare line1 e poi non va più usata.
 2) line1 è una versione compattata di line0: si sopprimono le parti f# dei nomi nei casi in cui il numero coincida con quello del file predefinito
 3) prepLine è la versione di line1 preparata per il calcolo: al posto delle costanti sono presenti i caratteri '#', al posto delle variabili i caratteri 'ò'
 4) line è una stringa di lavoro che viene continuamente modificata durante l'elaborazione. Alla fine del calcolo conterrà il solo carattere '#', che è il valore che la stringa assume
 5) fullLine è una versione non usata per le elaborazioni, ma solo per fornire all'utente finale un'informazione chiara: contiene al posto dei nomi codificati tipo f#v# o v# in nomi completi delle variabili dei files originali, ma privi dei nomi dei files.
 NOTA sfruttando la funzione giveLine1 si può mettere nella tabella dei nomi delle variabili di PlotXY la versione già alleggerita di line0


 *********************
    FUNZIONAMENTO INTERNO

    Internamente prima si fanno le seguenti due fasi:
   1) si manda in esecuzione constantsToPointers(). Questa funzione analizza la stringa ed identifica tutti i numeri espliciti. Essi vengono sostituiti con il carattere '#'. La posizione di tale carattere costituisce l'indice di un puntatore al numero sostituito
   2) si manda in esecuzione variablesToPointers(). Questa funzione identifica tutti i nomi letterali, che a loro volta identificano le variabili-funzione. Essi vengono sostituiti con il carattere '@'. La posizione di tale carattere costituisce l'indice di un puntatore al umero sostituito. Tale puntatore viene richiesto all'esterno attraverso un signal.
   3) poi si analizza la stringa andando a cercare gli operatori. Ogni volta che è trovato un operatore si identifica il numero alla sua sinistra (attraverso il meccanismo '#' - puntatore) e quello alla sua destra e si effettua il calcolo. Il risultato è sostituito con un nuovo carattere '#' in corrispondenza di dove era posizionato l'operatore, e questo carattere punta al risultato medesimo
   4) ovviamente il percorso di cui al punto 2 viene eseguito in due passate, la prima con gli operatori prioritari '* e '/' e nella seconda con '+' e '-'
   5) ovviamente la presenza di parentesi viene onorata. Viene definita una sottostringa con il contenuto della parentesi, e con tale contenuto viene effettuata una chiamata ricorsiva alla funzione di calcolo compute()
   6) alla fine del processo la stringa conterrà un unico carattere '#', la cui posizione è l'indice di un puntatore al risultato

*/

struct SVarNums{
    int fileNum, //contiene il numero del file per la variabile considerata; vale -1 per variabili di tipo 'v#'
        varNum; // numero della variabile; è il valore dell'ultimo '#' in f#v# o v#. Vale -1 in caso di nome passato incorretto.
    bool operator== (const SVarNums & x);
};

//Predisposizione per il passaggio all'uso del nuovo CLineCalc (v. CLinecalc\developer.docx):
struct SInputData {QList <QByteArray> nameList; float ** y_;};

struct SXYNameData{
    bool allLegalNames; //true se tutte le variabili contengono un nome legale Se allLegalNames=false il contenuto delle altre variabili della struttura è indeterminato in quanto alla prima variabile non valida l'analisi della stringa di input è interrotta
    bool rightScale;  //dice se la variabile va plottata verso l'asse verticale destro o no.
    bool integralRequest;  //dice se si sta richiedendo l'integrale di una stringa
    QColor color;
    Qt::PenStyle style;
    QList <int> fileNums; //contiene la lista dei numeri dei files da cui è necessario prelevare le variabili della stringa (un item in lista per ogni file differente)
    QList <SVarNums> varNumsLst; //Numeri delle variabili. Un item in lista per ogni variabile differente
    QList <QString> varNames; //i nomi delle variabili della stringa. Forniscono un'informazione meno sofisticata di varNumsLst ma utile quando non serve la scomposizione di dettaglio ma solo i nomi.
    QList <QString> varUnits; //i nomi delle variabili unità di misura delle variabili della stringa. L'ordine è il medesimo di varNames.
    QString line; //v. CLineCalcDevel
    QString lineInt; //v. CLineCalcDevel
    QString name; //nome attribuito alla funzione di variabile descritta.
    QString ret; //messaggio di errore compilato se allLegalNames=false;
};

QString fillNames(QString inpStr, int defaultFileNum);

class CLineCalc{
  public:
    bool allowMathFunctions; // se accetta funzioni tipo sin(), cos(), abs(),ecc.
    bool xyNaming; //se true accetta solo nomi di variabili f#v# o v#
    bool divisionByZero; //se c'è stata una divisione per 0 diviene true
    bool domainError; //domain error in sqrt
    bool integralRequest; //Per la spiegazione vedere il commento introduttivo all'inizio della descrizione della classe
    QString ret;
    CLineCalc(bool allowMathFunctions_=true);
    SXYNameData checkAndFindNames();
    QString checkBSharp(QList<QString> varNames);
    QString checkLine();
    float compute(int iVal);
    void getExplicitNames(QList<QList <QString> >  names_);
    void getFileInfo(QList <int> fileNumsLst_, QList<QString> fileNamesLst_, QList <int> varMaxNumsLst_);
    QString getLine(QString line_, int defaultfileNum_);
    QString getNamesAndMatrix(QList <QString> nameList, float ** y_);
    QString getNamesAndMatrix(QList <QString> nameList, QList<QString> unitsList, float ** y_, QList<QString *> namesFullList, int selectedFileIdx);
    QString giveLine(QString);
    QString unitOfMeasuref();

  private:
    bool constantsAreSharps; //=ture quando le costanti sono state sostituite con '#'
    bool constantsConverted; //true se questa routine sia stata chiamata a valle di substPointersToConsts():
    bool gotExplicitNames; //se true, sono stati passati i nomi espliciti delle variabili di tutti i files, e quindi si può compilare il campo "fullName. Se essa è true,  quando giveXYVarNames viene mandato in esecuzione, nel valore di ritorno il campo "fullName" è compilato; altrimenti resta indeterminato.
    bool lineIsSimplified; //=true quando è stata eseguita simplifyAndFindNames()
    bool lineReceived; // true se una line è stata già ricevuta attraverso "getLine"
    bool pointersPrepared; //true se i puntatori sono stati preparati dopo la ricezione della linea tramite chiamata a getAndPrepare().
    bool unitCharLstFilled; //true se unitCharLst è stato compilato
    bool variablesAreSimplified; //se true i nomi sono stati rielaborati ad esempio convertendo f#v# in v#
    bool varListsReceived; //è true se è sono state inviate le liste dei nomi e valori di variabili per la riga corrente
    bool yReceived; //è true se è sono state inviate le liste dei nomi di variabili per la riga corrente e relativa matrice dei valori y
    int defaultFileNum;
    float result;
    QString funText; // Contiene il nome del file di funzione se unico, altrimenti "*"
    QString lineUser; //la linea originaria per il calcolo della funzione di variabili
    QString lineNoInt; //la linea originaria per il calcolo della funzione di variabili privata dell'eventuale  "int(.)"
    QString line; //lineNoInt con i nomi delle variabili semplificati, privati dei f# superflui, con applicazione di simplified(), e sostituzione di ',' con '.'
    QString lineInt; //come Line, ma contenente, se presente in lineUser,  "int(.)"
    QString intLine;// Stringa interna, continuamente alterata durante l’elaborazione di compute(). Non è locale di compute() perché deve consentire un’esecuzione ricorsiva.
    QString lineFullNames;//su richiesta nella funzione ### è preparata questa stringa speciale, che contiene invece dei nomi convenzionali i nomi completi delle variabili originali, ma privi dei nomi dei files. Serviranno per visualizzare meglio i nomi delle variabili nelle finestre di plot


    QList <QString> myNameList; //Lista dei nomi delle variabili
    QList <QString> myUnitList; //Lista delle unità di misura delle variabili. Questa lista è ordinata come myNameList
    QList <QChar> unitCharLst; //Lista dei primi caratteri dei nomi reali (servono per le unità di misura. Questa lista è ordinata come myNameList
    QString unitOfMeasure; //contiene l'unità di misura risultante dall'elaborazione delle unità presenti nella formula
    QList <int> allowedFunIndexes; //
    QList <int> fileNumsLst;  //Lista dei numeri di files correntemente visualizzati nella fileTable
    QList <QString> fileNamesLst;  //Lista dei nomi di files correntemente visualizzati nella fileTable (senza path)
    QString computeUnits();

    //pure function tetermins uunit of measure from partial units
    QList <int> varMaxNumsLst; //Lista dei numeri di variabili dei files correntemente visualizzati nella fileTable
    //La sintassi  di CLineCalc prevede solo rxn e rxnn:
    QVector <int> varNumVect; //contiene l'elenco del numero di variabili per i MAXFILES files; serve per il check sintattico per le funzioni di variabile.
    // Array dei nomi espliciti di tutte le variabili di tutti i files. Si tratta di un array di array, con dimensioni dinamiche. As esempio explicitNames[2][3] dà la stringa che descrive la terza variabile del secondo file
    QList<QList <QString> >  explicitNames;

    QRegExp rxNum, //number
         rxOper, //operator
         rxNotDigit, //not a digit
         rxNotNum, //not a character allowable in a number
         rxNumSepar, //carattere ammissibile fra un numero e il successivo
         rxLetter, //a letter: a variable name must begin with a letter
         rxLetterDigit, //a letter or a digit: a non-first variable name character must be a letter or a digit
         rxNotLetterDigit, //the first character after a variable name must be such
         rxNotLetterDigitBracket,
         rxDatumPtr, //il carattere '#' o '@'. Entrambi puntano ad un dato il primo di costante, il secondo di variabile.
         rxAlphabet; //l'intero alfabeto, unione delle seguenti due stringhe

    bool * pUnaryMinus; //mi dice se una variabie deve essere peceduta da un '-' unario
    float * pConst; //vettore di puntatori alle costanti in line
    float (*(*pFun))(float x); //vettore di puntatori alle funzioni
    float ** pVar; //vettore di puntatori ai primi valori di ogni variabile-funzione
    QString *pUnit; //vettore dei puntatori a unità di misura, contenuti nella stringlist mUnits
    SXYNameData nameData;
    //puntatori alle funzioni-operatore:
    QString funStr[MAXFUNCTIONS];
    float (*fun1[MAXFUNCTIONS])(float x1); // Il nome fun1 fa riferimento al fatto che sono funzioni a un argomento (come sin, cos, ecc.)
    QString (*fun2[MAXBINARYOPS])(float x1, float x2,  float  &y); // Il nome fun2 fa riferimento al fatto che sono funzioni a due argomenti (come somma, prodotto, ecc.)
    static QString  sum(float x1, float x2, float & y), subtr(float x1, float x2, float & y),
               prod(float x1, float x2, float & y),   div(float x1, float x2, float & y),
               power(float x1, float x2, float & y);
//    void computeXYNameData();
    QString computeFun1(int start, int iVal);
    QString substConstsWithPointers();
    QString substFunsWithPointers();
    SVarNums readVarXYNums(QString varStr);
    QString substVarsWithPointers(float **y_);

};

#endif // CLINECALC_H
