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

#include "CLineCalc.h"
#include <QDebug>
#define max(a, b)  (((a) > (b)) ? (a) : (b))

// La seguente funzione statica, usata qui e in CVarTable, è identica a quella dentro CSimOut. /denominata giveAutoUnits()) Ad un certo punto antrà soppressa fuori di CSimOut in quanto le unità vengono lì associate direttamente alle variabili lette e non c'è nessun bisogno di valutarle nuovamente.

QString giveUnits(QChar c){
    int ic=c.toLatin1();
    switch (ic){
    case 't': return "s";
    case 'f': return "s";
    case 'v': return "V";
    case 'c': return "A";
    case 'i': return "A";
    case 'p': return "W";
    case 'e': return "J";
  }
  return "";
}


CLineCalc::CLineCalc(bool allowMathFunctions_){
    allowMathFunctions=allowMathFunctions_;
    xyNaming=true;
    gotExplicitNames=false;
    lineReceived=false;
    pointersPrepared=false;
    unitCharLstFilled=false;
    varListsReceived=false;
    funStr[0]="sin";   fun1[0]=sinf;
    funStr[1]="cos";   fun1[1]=cosf;
    funStr[2]="tan";   fun1[2]=tanf;
    funStr[3]="sinh";  fun1[3]=sinhf;
    funStr[4]="cosh";  fun1[4]=coshf;
    funStr[5]="tanh";  fun1[5]=tanhf;
    funStr[6]="exp";   fun1[6]=expf;
    funStr[7]="sqrt";  fun1[7]=sqrtf;  //unica funzione accettata in PlotXY che necessita verifica di dominio
    funStr[8]="asin";  fun1[8]=asinf;
    funStr[9]="acos";  fun1[9]=acosf;
    funStr[10]="atan"; fun1[10]=atanf;
    funStr[11]="log";  fun1[11]=logf;
    funStr[12]="abs";  fun1[12]=fabsf;

    // In PlotXY ammetto solo alcune funzioni:
    allowedFunIndexes<<0<<1<<6<<7<<12;

    fun2[0]=power;
    fun2[1]=prod;
    fun2[2]=div;
    fun2[3]=sum;
    fun2[4]=subtr;
    rxAlphabet=QRegExp("[0-9a-zA-Z .-+*/()]");
    rxDatumPtr=QRegExp("[#@]");
    rxLetter=QRegExp("[a-zA-Z]"); //initial character of a variable
    rxLetterDigit=QRegExp("[a-zA-Z0-9]");
    rxNotDigit=QRegExp("[^0-9]");  //not a digit
    rxNotNum=QRegExp("[^0-9.]"); //not a character allowable in a number. I caratteri 'E' e 'e' sono considerati non-allowable perché sono trattati a parte per gestire l'eventuale segno sull'esponente.
    rxNum=QRegExp("[0-9.]"); //initial character of a number
    rxNumSepar=QRegExp("[-+*/() ]"); //carattere ammissibile fra un numero ed il successivo
    rxOper=QRegExp("[-+*/]"); //operator
    rxNotLetterDigit=QRegExp("[^a-zA-Z0-9]");
    rxNotLetterDigitBracket=QRegExp("[^a-zA-Z0-9)]");
    defaultFileNum=-1;
    pConst=nullptr;
    pUnaryMinus=nullptr;
    pFun=nullptr;
    pVar=nullptr;
    pUnit=nullptr;
    unitOfMeasure="";

}


QString CLineCalc::checkLine(){
    /* Questa funzione fa un'analisi della validità della stringa descrittiva, in quanto in PlotXY l'accettazione della stringa è precedente al momento in cui essa viene valutata: infatti essa è accettata al momento dell'inserzione in VarList, mentre è eseguita iterativamente dopo che è stato cliccato il pulsante di plot().
*/
    if(!lineReceived)return "No line already received.";
    return "";
}


float CLineCalc::compute(int iVal){
    /* Quando questa funzione è chiamata line contiene già puntatori a costanti e variabili.
      Ne fa l'interpretazione. iVal è l'indice dell'array delle variabili. Se ad esempio devo sommare due sinusoidi di 100 elementi, questa funzione verrà richiamata con iVal che va da 0 a 99.
*/
   bool  unary=false, unaryMinus=false;
   static bool recursiveCall=false;
   int d1, d2, //indici dei dati  1 e 2
           o, // indice dell'operatore
           j;  //indice generico
   float x1, x2, y=0;
   divisionByZero=false;
   domainError=false;

/* Questa funzione altera "intLine" durante il calcolo. Può essere chiamata più volte dall'esterno con diversi valori di iVal, e può essere anche chiamata ricorsivamente da se stessa durante l'elaborazione delle espressioni fra parentesi.
La variabile contenente la stringa da analizzare, che rimane stabile da una chiamata dall'esterno all'altra di compute() è line.
Ogni volta che compute() è chiamata dall'esterno ricopio line in intLine, mentre durante le chiamate di compute() a compute() non effettuo questa ricopiatura*/
   if(!recursiveCall)
       intLine=line;
   recursiveCall=false;

   // se vi è una parentesi aperta richiamo ricorsivamente la funzione sostituendo '(' con '['
   if((j=intLine.indexOf('('))>=0){
       intLine[j]='[';
       recursiveCall=true;
       compute(iVal);
   }
   //adesso effettuo l'analisi fra l'ultima '[' e la prima ') che la segue
   int start=intLine.lastIndexOf('['),
        end=intLine.indexOf(')',start);
   if(start==-1){
       start=0;
       end=intLine.length()-1;
   }else{
       //elimino le parentesi in quanto nel resto della funzione ne tratto il contenuto riconducendolo ad un unico '#' all'interno di spazi
       intLine[start]=' ';
       intLine[end]  =' ';
   }

   // Prima valuto le chiamate a funzione
   ret=computeFun1(start, iVal); //funzioni con ()
   if(ret.length()>0){
     domainError=true;
     return 0;
   }

   //Prima applico gli operatori prioritari da sinistra verso destra:
   for(o=start; o<end; o++){
     if(intLine[o]!='*' && intLine[o]!='/')continue;
     //cerco il dato a sinistra
     d1=qMax(intLine.lastIndexOf('#',o-1),intLine.lastIndexOf('@',o-1));
     if(d1<start)
        {ret="Internal error\"x1_*/\"";return 0;}
     if(intLine[d1]=='#')
       x1=pConst[d1];
     else{ //a questo punto intLine[d1]='@'
       if(pUnaryMinus[d1]){
         x1=-pVar[d1][iVal];
       }else{
          x1=pVar[d1][iVal];
       }
     }//cerco il dato a destra:
     d2=rxDatumPtr.indexIn(intLine,o+1);
     if(d2<0||d2>end){ret="Unable to find second operand. \nInvolved operator: \""+ intLine.mid(o,1)+"\"";return 0;}
     if(intLine[d2]=='#')
       x2=pConst[d2];
     else{  //a questo punto intLine[d2]='@'
       if(pUnaryMinus[d2]){
         x2=-pVar[d2][iVal];
       }else{
         x2=pVar[d2][iVal];
       }
     }
     //effettuo il calcolo:
     if(intLine[o]=='*')ret=prod(x1,x2,y);
     if(intLine[o]=='/')ret= div(x1,x2,y);
     if(ret.length()>0){
         divisionByZero=true;
         return 0;
     }
     //sostituisco il risultato: metto '#' dove c'era l'operatore e in pConst del suo indice il relativo valore.
     for(j=d1; j<=d2; j++)intLine[j]=' ';
     intLine[o]='#';
     pConst[o]=y;
   }
   //Poi applico gli operatori meno prioritari da sinistra verso destra:
   for(o=start; o<end; o++){
     if(intLine[o]!='+' && intLine[o]!='-')continue;
     //cerco il dato a sinistra
     d1=qMax(intLine.lastIndexOf('#',o-1),intLine.lastIndexOf('@',o-1));
     if(d1<start)
        {ret="Internal error\"x1_*/\""; return 0;}
     if(intLine[d1]=='#')
       x1=pConst[d1];
     else{ //a questo punto intLine[d1]='@'
       if(pUnaryMinus[d1]){
         x1=-pVar[d1][iVal];
       }else{
         x1=pVar[d1][iVal];
       }
     }
     //cerco il dato a destra:
     d2=rxDatumPtr.indexIn(intLine,o+1);
     if(d2<0){ret="Internal error\"x2_*/\"";return 0;}
     if(intLine[d2]=='#')
       x2=pConst[d2];
     else{  //a questo punto intLine[d2]='@'
       if(pUnaryMinus[d2]){
         x2=-pVar[d2][iVal];
       }else{
         x2=pVar[d2][iVal];
       }
     }
     //effettuo il calcolo:
     if(intLine[o]=='+')ret=sum(x1,x2,y);
     if(intLine[o]=='-')ret=subtr(x1,x2,y);
     //sostituisco il risultato: metto '#' dove c'era l'operatore e in pConst del suo indice il relativo valore.
     for(j=d1; j<=d2; j++)intLine[j]=' ';
     intLine[o]='#';
     pConst[o]=y;
   }

   //adesso devo valutare se la parentesi che sto calcolando era preceduta da un operatore unario. Questo capita se prima di essa vi è '+' o '-' e prima ancora, dopo eventuali spazi, una parentesi o l'inizio stringa.
   unary=false;
   if(start==1)
     if(intLine[0]=='+' || intLine[0]=='-') unary=true;
   if(start>1){
     if(intLine[start-1]=='+' || intLine[start-1]=='-') unary=true;
     //ma in realtà prima di start-1, saltati gli spazi, deve esservi una parentesi aperta o inizio rigo:
     int index=start-2;
     while(intLine[index]==' ' && index>=0)
         index--;
     if (intLine[index]!=' ' && intLine[index]!='(') unary = false;
   }
   if (unary) {
       if(intLine[start-1]=='-')unaryMinus=true;
       intLine[start-1]=' ';
   }
   //a questo punto la stringa contiene un unico carattere '#' o '@' e il relativo valore è il risultato, a meno dell'eventuale '-' unario a sinistra della parentesi considerata:
   d1=intLine.indexOf('#',start);
   if(d1<0){
      d1=intLine.indexOf('@');
      if(d1<0)
        {ret="Internal error \"y\"";return 0;}
      else{
          result=pVar[d1][iVal];
          if(unaryMinus)result=-result;
        // ci può essere un '-' unario anche interno alla '@'
          if(pUnaryMinus[d1])result=-result;
      }
   }else{
     if(unaryMinus)pConst[d1]=-pConst[d1];
      result=pConst[d1];
    }
   unary=false;
   unaryMinus=false;
   return result;
}

QString CLineCalc::computeFun1(int start, int iVal){
 /* Percorre sequenzialmente la stringa line, fra gli indici start ed end, ed esegue da sinistra a destra tutte le chiamate a funzioni specificate dai puntatori pFun.
*/
  int i=0,j;
  float y=0;
  QString ret="";
  while(true){
    i=intLine.indexOf('&',start);
    if(i==-1)return "";
    j=i+1;
    while(intLine[j]==' ')j++;
    //A questo punto l'argomento della funzione può essere una costante (carattere '#') o una variabile (carattere '@')
    if(intLine[j]=='#'){
//      if(pFun[i]==sqrt && pConst[j]<0)
        if(pFun[i]==fun1[7] && pConst[j]<0)
          ret="Domain error when evaluating sqrt()";
      y=pFun[i](pConst[j]);
    }
    if(intLine[j]=='@'){
//      if(pFun[i]==sqrt && pVar[j][iVal]<0)
        if(pFun[i]==fun1[7] && pVar[j][iVal]<0)
        ret="Domain error when evaluating sqrt()";
      y=pFun[i](pVar[j][iVal]);
    }
    intLine[i]='#';
    intLine[j]=' ';
    pConst[i]=float(y);
    return ret;
  }
}


SXYNameData CLineCalc::checkAndFindNames(){
/* Routine che verifica la validità dei nomi presenti nella stringa "line", e li inserisce
 * con le necessarie informazioni di corredo nella struttura nameData.
 * La stringa  "line" darà già stata generata a partire da lineUser, con soppressione di
 * "int", spazi in eccesso e conversione di virgole in punti)
 *
 * CLineCalc accetta come nomi di variabili generici nomi che comincino con una lettera
 * e contengano lettere e numeri.
 * Però quando xyNaming=true essa è usata all'interno di XY e si hanno ulteriori restrizioni
 * sui nomi. In tal caso, infatti, i nomi sono del tipo f#v# o v#.
 *
 * In questa routine si fanno delle elaborazioni su "line", (è già stata generata a
 * partire da lineUser, con soppressione di "int", spazi in eccesso e conversione di
 * virgole in punti) per trovare gli identificatori dei nomi
 * Nel caso in cui sia xyNaming=true:
 *   - si verifica che gli identificatori dei nomi rispettino lo standard f#v# / v#
 *   - per ogni variabile si identificano i valori numerici contenuti all'interno del nome
 *     (carattere '#')
 *   - si analizza il risultato per vedere se le variabili appartengono tutte al medesimo
 *     file ovvero a quale set di files appartengono.
 *   - si crea la stringa (per il solo uso di visualizzazione all'utente) SXYNameData.
 * lineFullNames
 *
 * Questa analisi verrà utilizzata dal programma chiamante per preparare i dati per
 * "getNamesAndMatrix()" e, nel caso di files multipli, quindi con diversi valori delle
 * grandezze sull'asse x, anche per preparare i vettori da passare a compute().

struct SVarNums{
    int fileNum, varNum;
  };
struct SXYNameData{
  bool  allLegalNames;
  QList <int> fileNums;
  QList <SVarNums> varNums;
}
*/
    nameData.allLegalNames=true;
    nameData.fileNums.clear();
    nameData.varNumsLst.clear();
    nameData.varNames.clear();
    nameData.ret="";

    SVarNums varXYNums;

    bool eol=false;
    int i, //indice del carattere considerato misurato su line
        j;
    QString varStr;

    if(!constantsAreSharps)
      substConstsWithPointers(); //scrive i '#' in "line"

    i=0;
    //Ora procedo con l'analisi considerando la ricerca di nomi validi
    while(!eol){
       i=rxLetter.indexIn(line,i); //l'inizio della variabile dev'essere una lettera
       if(i<0){ eol=true;  break; }
       if(allowMathFunctions)
         j=rxNotLetterDigitBracket.indexIn(line,i+1); //la fine della variabile è il primo carattere non lettera né digit né parentesi chiusa (è parentesi ad es. nel caso di 'abs(v9)')
       else
         j=rxNotLetterDigit.indexIn(line,i+1); //la fine della variabile è il primo carattere non lettera né digit
       if(j>=0)
         varStr=line.mid(i,j-i);
       else{
         // In questo caso posso avere una o più parentesi chiuse. Devo scegliere il primo carattere a sinistra della prima parentes chiusa.
         if(line[line.length()-1]==')'){
            j=rxNotLetterDigit.indexIn(line,i+1);
            varStr=line.mid(i,j-i);
         }else{
            varStr=line.mid(i,line.length());
         }
       }
       if(allowMathFunctions && varStr[varStr.length()-1]==')')
           varStr.chop(1);
       //ora varStr contiene la stringa di variabile (comincia con lettera e contiene lettere e digits).
       if(!xyNaming){
         if(!nameData.varNames.contains(varStr))
             nameData.varNames.append(varStr);
         i+=varStr.length();
         continue;
       }
       //Verifico se si tratta di un nome di funzione, altrimenti procedo con la lettura della variabile:
       if(allowMathFunctions){
         bool isFunction=false;
         foreach (int i,allowedFunIndexes){
           if(varStr==funStr[i]){
             isFunction=true;
             break;
          }
         }
         if(isFunction){
           i+=varStr.length();
           continue;
         }
       }
       // Lettura dei numeri # in v# o f#v#:
       varXYNums=readVarXYNums(varStr);
       //Se varNum=-1 c'è stato un errore di lettura
       if(varXYNums.varNum==-1){
         if(allowMathFunctions)
           nameData.ret=
             "The following incorrect function or variable name "
             "was read in the input string: \"" +varStr+"\"" ;
         else
           nameData.ret=
             "The following incorrect variable name was read in the input string: \"" +varStr+"\"";
           return nameData;
       }
       // Ora qui devo verificare se i numeri di file e gli indici di variabile sono validi
       if(!fileNumsLst.contains(varXYNums.fileNum)){
         nameData.ret=
           "The string \""+line+"\" refers to the following non-existent file number: "   +QString::number(varXYNums.fileNum);
         return nameData;
       }
       if(varXYNums.varNum > varMaxNumsLst[varXYNums.fileNum-1]){
         nameData.ret=
           "The string contains reference to non-existent variable number: "   + QString::number(varXYNums.varNum) +
           "\nreferring to file number: "+ QString::number(varXYNums.fileNum) ;
         return nameData;
       }

       if(varXYNums.varNum<0){
           nameData.ret="Invalid variable name: \""+QString(varStr) + "\"";
           goto errorReturn;
       }
       if(!nameData.fileNums.contains(varXYNums.fileNum))
           nameData.fileNums.append(varXYNums.fileNum);
       if(!nameData.varNumsLst.contains(varXYNums))
           nameData.varNumsLst.append(varXYNums);
       if(!nameData.varNames.contains(varStr))
           nameData.varNames.append(varStr);
       i+=varStr.length();

       if(i>line.count()-1)
           eol=true;
    }
    nameData.line=line;
    nameData.lineInt=lineInt;
    if(nameData.varNames.count()==0){
      nameData.ret="The given string must contain at least one variable";
    }
    return nameData;

errorReturn:
  nameData.allLegalNames=false;
  return nameData;
}

QString CLineCalc::substConstsWithPointers(){
    /* In questa funzione si sostituiscono le costanti con il carattere '#', ed in corrispondenza della sua posizione, il relativo valore viene messo nel corrispondente puntatore a float pConst[i].
Per prima cosa si tratta l'eventuale unario che si trova a inizio stringa, e poi si procede con numeri tutti positivi */
   bool eol=false, unary=false, unaryMinus=false, ok;
   int i=0,j, k1, k2;
   QString numStr;
   i=-1;
   while(!eol){
     i++;
     i=rxNum.indexIn(line,i);
     if(i<0){
       eol=true;
       break;
     }
     if(i>0){
        // se immediatamente prima di i vi è un digit o una lettera il digit che ho trovato è all'interno di una variabile e non mi interessa
        if(rxLetterDigit.indexIn(line,i-1)==i-1) continue;
        //devo verificare se il numero è preceduto da un operatore unario ('+' o '-'). Prima di tutto cerco il più recente unario:
        k1=max(line.lastIndexOf('+',i-1), line.lastIndexOf('-',i-1));
        //alla posizione i vi è un unario se prima di esso, escluso al più un ' ', non vi è nulla o una parentesi aperta
        if(k1==0 && line[1]!='(')
            unary=true;
        else if(k1>0){
            k2=line.lastIndexOf('(',k1-1);
            if(k2==k1-1 || (k2==k1-2 && line[k1-1]==' ')) unary=true;
        }
        if(unary && line[k1]=='-') unaryMinus=true;
        if(unary)line[k1]=' ';
      }
      j=rxNotNum.indexIn(line,i+1);
      //Se il numero era in formato esponenziale, line[j] contiene la lettera "E" o "e".
      if(j>0){
        if(line[j]=='E'||line[j]=='e'){
          if(line[j+1]=='+'||line[j+1]=='-')
            j=rxNumSepar.indexIn(line,j+2);
          else
            j=rxNumSepar.indexIn(line,j+1);
        }
      }
    if(j<0)j=line.length();
      numStr=line.mid(i,j-i);
      if(unaryMinus)
         pConst[i]=-numStr.toFloat(&ok);
      else
        pConst[i]=numStr.toFloat(&ok);
      if(!ok)return"Erroneous number substring: \""+numStr+"\"";
      unary=false;
      unaryMinus=false;
      line[i]='#';
      if(j<0)j=line.length();
      for(int k=i+1;k<j;k++)
          line[k]=' ';
   }
   constantsConverted=true;
   return"";
}

QString CLineCalc::unitOfMeasuref(){
  return unitOfMeasure;
}

QString fillNames(QString inpStr, int defaultFileNum){
    /* Questa routine scrive eventuali nomi di forma compatta in forma completa. Es. v1+v2 diviene, se defaultFileNum è 2, f2v1+f2v2. Questo serve quando l'utente cambia il valore del file di default (cioè del defaultFileNum). In questo caso sarebbe assurdo cambiare il significato dei nomi originariamente introdotti, ma va chiarito il senso dei nomi con l'integrazione del nome del file a cui essi si riferivano.
     */
    QString filledLine=inpStr;
    QString fs = "f"+ QString::number(defaultFileNum);  //ad es. "f2"
    int i=-1;
    while (1) {
        i++;
        if(i>=filledLine.count()) break;
        if(filledLine[i] != 'v'){
            continue;
        }
        if(i==0){
            filledLine.insert(i,fs);
            i+=fs.count();
        } else  if(!filledLine[i-1].isDigit()){
            filledLine.insert(i,fs);
            i+=fs.count();
        }
    }
    return filledLine;
}


void CLineCalc::getFileInfo(QList <int> fileNumsLst_, QList <QString> fileNamesLst_, QList <int> varMaxNumsLst_){
 /* Questa funzione serve per consentire il check sintattico sulle stringhe introdotte dall'utente, e quindi fornisce la lista dei numeri di files sono utilizzabili nelle funzioni di variabili.
I nomi dei files vengono invece usati per creare la stringa corretta di tooltip per le variabili funzione (non attualmente, nov. 2016) che si usa "f1:", "f2:" ecc.)*/
  fileNumsLst=fileNumsLst_;
  fileNamesLst=fileNamesLst_;
  varMaxNumsLst=varMaxNumsLst_;
}


QString CLineCalc::getNamesAndMatrix(QList <QString> nameList, QList <QString> unitList, float ** y_, QList <QString *> namesFullList, int selectedFileIdx){
  /* Questa funzione overloaded oltre all'usuale calcolo di getNamesAndMatrix, compila anche
   *  la riga "lineFullNames", in cui ai nomi codificati delle variabili (tipo "f1v2")
   *  sono sostituiti nomi espliciti (tipo "busVoltage1").
-  nameList: lista dei nomi codificati delle variabili (tipo "f1v2")
-  y_ matrice dei dati su cui calcolare la funzione
-  namesFullList: ogni elemento della lista è un puntatore ad un vettore di stringhe
                  relativo ad un dato file, contenente i nomi effettivi delle variabili
                  (tipo v:UA)
-  selectedFileIndex è l'indice del file selezionato
-  selfilePosInLists  indica la posizione del file selezionato nelle liste. Può ad esempio
                      essere presente il solo file di num 2, il cui indice è 1, ma a quel
                      punto le liste hanno un unico elemento, di indice 0 (non 1!)

La costruzione di lineFullNames segue la seguente logica:
- se un nome è del tipo v# (ad es. v3) ad esso è sostituito il corrispondente nome completo
- se un nome è del tipo f#v# (ad es. f1v3) è sostituita solo la parte v#, e preceduta da ':. Ad esempio f1:sin(W*t)

*/

    //   QUI CORREGGERE METTENDO L'USO DI fileIdxToLists !!!

  QString ret;
  funText="*";

  myUnitList=unitList;


//Se tutti gli elementi provengono da un unico file nella finestra di visualizzazione dei dati mostro quel nome, altrimenti comparirà "*". Per fare la verifica guardo che i nomi che comprendono f# siano seguiti tutti dal medesimo numero il quale, se sono presenti nomi v# deve coincidere con il numero del file selezionato
  bool allNamesFromOneFile=true; // vera se tutte le variabili di una funzione di variabili provengono dal medesimo file
  int oneFileIndex=selectedFileIdx; //dà l'indice del file nel caso allNamesFromOneFile=true.
  //In una prima passata vedo se esiste almeno un nome del tipo f#:
  foreach(QString str,nameList){
    if(str[0]=='f'){
        oneFileIndex=str.mid(1,1).toInt()-1;
       break;
    }
  }
  // Nella seconda passata verifico se eventuali ulteriori nomi f# provengono dal medesimo file:
  foreach(QString str,nameList){
    if(str[0]=='f'){
      if(str.mid(1,1).toInt()!=oneFileIndex+1){
         allNamesFromOneFile=false;
         break;
      }
    }
  }
  //Nel caso in cui oneFileIndex è diverso da selectedFileIndex devo verificare che non esistano nomi del tipo v# (cioè senza f#):
  if(oneFileIndex!=selectedFileIdx)
    foreach(QString str,nameList){
      if(str[0]=='v'){
         allNamesFromOneFile=false;
         break;
      }
    }
  if(allNamesFromOneFile)
    funText=fileNamesLst[oneFileIndex];
    /* Nella precedente riga non si può fare funText=fileNamesLst[oneFileIndex)] in quanto può capitare che ho salvato ad es. un unico file di num 2 e non posso chiamare il nome di indice 1, visto che avendo un unico file l'unico indice è 0.*/

  //Ora riempo la stringa lineFullNames
  lineFullNames=lineInt;
  //La procedura prevede per ogni nome di nameList di percorrere la stringa alla ricerca del carattere "v", e, tutte le volte che viene trovato, sostituire il nome codificato con il nome esteso, secondo quanto specificato nel commento all'inizio di questa funzione.
  for (int i=0; i<nameList.count(); i++){
    int fileIndex=selectedFileIdx;
    QString name=nameList[i];
    if(name.indexOf("v")<0)
      return "Error in getNamesAndMatrix";
      int varIndex=name.remove(0,name.indexOf("v")+1).toInt()-1; //indice della variabile (es. se è f4v3 index è 3-1=2)
      name=nameList[i];
      if(name.indexOf("v")>1)
        if(name[name.indexOf("v")-2]=='f')
           fileIndex= name.mid(name.indexOf("v")-1,1).toInt()-1; //indice del file (es. se è f4v3 index è 4-1=3)
    //aggiungo il primo carattere alla lista dei caratteri che serve per le unità di misura:
    unitCharLst.append(namesFullList[fileIndex][varIndex][0]);

    int pos=lineFullNames.indexOf(name);
    //Ora procedo con la sostituzione dei nomi completi
    while ((pos=lineFullNames.indexOf(name)) >= 0){
      QString insertVar, myNum;
      if(allNamesFromOneFile)
        insertVar=namesFullList[fileIndex][varIndex];
      else
        insertVar="f"+myNum.setNum(fileIndex+1)+":"+namesFullList[fileIndex][varIndex];
      lineFullNames.remove(pos,name.count());
      lineFullNames.insert(pos,insertVar);
    }
  }
  unitCharLstFilled=true;

  ret=getNamesAndMatrix(nameList, y_);
  if(ret!="")
    return ret;

  unitOfMeasure=computeUnits();

  gotExplicitNames=true;
  return "";
}


QString CLineCalc::getNamesAndMatrix( QList <QString> nameList, float ** y_){
  /*Questa routine fa quanto segue:
   * -  effettua l'associazione delle costanti esplicite al rispettivo puntatore pConst[i]
   *    mediante la chiamata a constantsToPointers()
   * -  effettua l'associazione dei nomi di variabili-funzione al rispettivo puntatore
   *    pVar[i] mediante la chiamata a variablesToPointers()
   *
   * Essa va chiamata dopo getLine() e prima di compute()
   * Il primo parametro contiene una lista di nomi. Il secondo una matrice, realizzata
   * con la mia funzione "CreateFmatrix", quindi attraverso puntatore ad un vettore di
   * puntatori alle righe.
   * Ogni riga contiene i dati numerici di una delle funzioni, con corrispondenza ordinata
   * ai nomi riportati in nameList.
   * I valori di y devono essere allocati e disponibili esternamente all'oggetto CLineCalc.

Successivamente valuterò se, per ragioni di efficienza e occupazione di memoria, aggiungere anche una funzione che non necessiti di ricevere una lunga lista di valori, ma solo il puntatore alla matrice con tutti i valori di tutti i files, generando i nomi internamente, secondo una convenzione univoca, ad es. f#v#, indicando quindi il numero del file e il numero di variabile all'interno del file.
Dopo che ha ricevuto i valori la funzione si prepara al successivo calcolo iterativo con la sostituzione di costanti e variabili con puntatori
*/

  if(!lineReceived)
      return "INTERNAL ERROR:\nNo Line received before \"getAndPrepare()\".";
  myNameList=nameList;
  yReceived=true;
  ret=substConstsWithPointers();  //sostituisco tutte le costanti con 'puntatori' a float

   if(allowMathFunctions){
     ret=substFunsWithPointers();
     if(ret!="") return ret;
   }

  // A questo punto devono essere presenti solo variabili, operatori e parentesi. Fa eccezione il carattere '.' il quale è considerato accettabile nell'alfabeto, in quanto può far parte dei numeri, ma non è al momento un operatore valido, né un carattere che può appartenere ad un nome di variabile. Pertanto devo intercettare questo caso.
  if(ret=="" && line.contains('.')){
    int index=line.indexOf('.');
    QString dotStr;
   dotStr.setNum(index+1);
    QString ordinalStr="th";
    if(index==0)  ordinalStr="st";
    if(index==0)  ordinalStr="nd";
    ret="the input string contains a dot (character '.') not belonging to a numerical constant. This is invalid.\n"
        "The offending dot is in the " +dotStr+ ordinalStr+ " position in the string.";
  }
  //Fra una costante e la successiva o fra una costante e una variabile. oltre al più degli spazi, ci deve essere un operatore. Non deve quindi esistere alcuna sottostringa che contenga solo '#', 'à'@', e ' '.
  QRegExp rxInvalid=QRegExp("# *#");
  if(rxInvalid.indexIn(line)>-1){
    ret="Two consecutive constants without operators between have been detected.\nThis is invalid";
  }

  if(ret.length()==0)
    ret=substVarsWithPointers(y_); //sostituisco tutte le variabili con 'puntatori' a float
  if(ret!="")
      return ret;
  rxInvalid=QRegExp("@ *@");
  if(rxInvalid.indexIn(line)>-1){
    ret="Two consecutive variables without operators between have been detected.\nThis is invalid";
  }
  rxInvalid=QRegExp("# *@");
  if(rxInvalid.indexIn(line)>-1){
    ret="A constant is followed by a variable without operators in-between.\nThis is invalid";
  }
  rxInvalid=QRegExp("@ *#");
  if(rxInvalid.indexIn(line)>-1){
      ret="A variable is followed by a constant without operators in-between.\nThis is invalid";
  }
  int index;
    rxInvalid=QRegExp(" *\\* *@"); //zero or more spaces, operator '*', zero or more spaces, '@'
    index=rxInvalid.indexIn(line);
    if(index>-1){
      if(index==0) //se ho trovato il pattern all'inizio prima non ci sono operatori
        ret="A non-unary operator is followed by a variable,\nbut not preceded by any variable or operator.\nThis is invalid";
      else{ //se il pattern trovato non era all'inizio vado a guardare cosa c'era prima
        if(line[index-1]!='@' && line[index-1]!='#' && line[index-1]!=')')
        ret="A non-unary operator is followed by a variable,\nbut not preceded by any variable or constant.\nThis is invalid";
      }
    }

    //Dopo ogni operatore vi deve essere una costante o una variabile. Siccome gli operatori consecutivi sono stati già filtrati, mi rimane solo da verificare che non vi sia un operatore a fine riga.
  rxInvalid=QRegExp("[^ ]");  //cercando da fondo cerco l'ultimo carattere che non è spazio
  index=rxInvalid.indexIn(line,-1);
  if(index>-1){
    if(line[index]!='@' && line[index]!='#' && line[index]!=')')
      ret="An operator is at the very end of the input string.\nThis is invalid";
  }

  if(ret!="")return ret;
  pointersPrepared=true;
    return "";
}


void CLineCalc::getExplicitNames(QList<QList <QString> >  names_){
    /* Con questa funzione acquisisco i nomi espliciti di tutte le variabili di tutti i files.
       Si tratta di un array di array, con dimensioni dinamiche. As esempio explicitNames[2][3] dà la stringa che descrive la terza variabile del secondo file
*/
    explicitNames=names_;
    gotExplicitNames=true;
}

QString CLineCalc::getLine(QString line_, int defaultfileNum_){
  /* Funzione che riceve la stringa che verrà utilizata per il calcolo delle funzioni di grafici.

Riceve la stringa, compila le versioni semplificate fa un po' di diagnostica e alloca spazio per i puntatori.

Le stringhe da compilare sono le seguenti (cfr CLineCalcDevel.docx)
lineUser      è la stringa introdotta dall’utente
lineNoInt     è a stringa ottenuta da lineUser mediante soppressione (Se esistenti) di “int(“ e “)”
lineSimple    è la stringa ottenuta da lineNoInt mediante semplificazione dei nomi delle variabili. Ad es. “f1v2” è trasformato in “v2” se il file predefinito è 1

*/
   int j, k;
   constantsConverted=false;
   constantsAreSharps=false;
   defaultFileNum=defaultfileNum_;
   gotExplicitNames=false;
   integralRequest=false;
   lineIsSimplified=false;
   lineReceived=true;
   pointersPrepared=false;
   varListsReceived=false;
   variablesAreSimplified=false;
   yReceived=false;
   delete[] pConst;
   delete[] pUnaryMinus;
   delete[] pFun;
   delete[] pVar;
   delete[] pUnit;

   if(line_.length()<1)return "null strings are not accepted";
   lineUser=line_;
   //Ora gestisco l'eventuale richiesta di integrale
   lineNoInt=lineUser;
   if(lineUser.mid(0,4)=="int(" && lineUser.right(1)==")"){
       //Se vi è una richiesta di integrale la funzione semplicemente registra tale richiesta, e poi continua a processare la riga privata di "int(" e ")"
       //Sarà onere del programma chiamante leggere la booleana integralRequest e procedere a fare l'integrazione del valore ritornato da CLineCalc
       integralRequest=true;
       lineNoInt=lineUser.mid(4,lineUser.size()-5);
   }
   lineNoInt=lineNoInt.simplified();
   //Il carattere di separazione decimale usato internamente è '.'. Pertanto se trovo ',' faccio la conversione. Però può accadere che io debba usare la stringa originale per l'emissione di messaggi d'errore. Pertanto mantengo la stringa con il separatore originale in lineUser.
   lineNoInt.replace(',','.');

   line=lineNoInt;
//   lineFirstChar=line;
//   for(int i=0; i<line.count(); i++)
//       lineFirstChar[i]=' ';
   //per semplicità alloco spazi per ogni carattere della stringa "noInt".
   pConst=new float[line.length()];
   typedef float (*FuncPtr)(float);
   pFun = new FuncPtr[line.length()];
   pUnaryMinus=new bool[line.length()];
   for (int i=0; i<line.length(); i++)
       pUnaryMinus[i]=false;
   pVar=new float*[line.length()];
   pUnit=new QString[line.length()];

   // **** ora una semplice diagnostica.
   //1) verifica che tutti i caratteri appartengono all'alfabeto previsto.
   for(int i=0; i<line.length(); i++){
     if(rxAlphabet.indexIn(line.mid(i,1))<0)
        return "The string cannot contain character "+line.mid(i,1)+"\'";
   }
   //2) dopo un operatore non deve essere presente un altro operatore:
   j=-1;
   while(1){
     j++;
     j=rxOper.indexIn(line,j);
     if(j>=0){
         //ora in j è l'indice di un operatore. Il carattere successivo non deve essere un operatore; se è ' ' quello ancora dopo non dev'essere un operatore.
         k=rxOper.indexIn(line,j+1);
         if(k==j+1 || (k==j+2 && line[j+1]==' '))
            return "The string contains consecutive operators without numbers or brackets in between.";
     } else break;
   }
   //3) bilanciamento parentesi: lungo la stringa non ci devono essere più chiuse che aperte e alla fine devono essere bilanciate
   int par=0;
   for(int i=0; i<line.length(); i++){
     if(line[i]=='(') par++;
     if(line[i]==')') par--;
     if(par<0)break;
   }
   if(par!=0) return "The string contains unbalanced brackets";

   checkAndFindNames();
   lineInt=lineNoInt;
   if(integralRequest)
     lineInt="int("+lineNoInt+")";
   return "";
}


QString CLineCalc::giveLine(QString str){
   if(str=="funText")return funText;
   if(str=="lineUser")return lineUser;
   if(str=="lineNoInt")return lineNoInt;
   if(str=="line")return line;
   if(str=="lineInt")return lineInt;
   if(str=="lineFullNames")return lineFullNames;
   return "";
}


QString CLineCalc::computeUnits(){

    int i;
    QString unit="";

    // In questa versione semplificata la stringa deve contenere solo un alfabeto semplificato:
    QRegExp rxSimpleAlphabet=QRegExp("[ @#+-*()]");
    //1) verifica che tutti i caratteri appartengono all'alfabeto previsto.
    for(int i=0; i<line.length(); i++){
        if(rxSimpleAlphabet.indexIn(line,i)<0){
          return "";
      }
    }
    // 2) verifico se ho un singolo prodotto e nel caso eseguo la valutazione dell'unità
    i=line.indexOf('*');
    // Se ho un secondo prodotto esco con unità indefinita:
    if(i>=0 && i<line.count()-1)
      if(line.indexOf('*',i+1)>0)
        return "";
    if(i>=0){
      // Ho un singolo prodotto e eseguo il calcolo dell'unità:
      int l,r; //left e right indexes)
      l=line.lastIndexOf('@',i);
      if (l<0) return "";
      r=line.indexOf('@',i);
      if (r<0) return "";


      QString ul=pUnit[l],   ur=pUnit[r];

      if((ul=="A" && ur=="V") || (ur=="A" && ul=="V") )
        unit="W";
      else
        unit="";
      return unit;
    }


    // A questo punto, in questa versione semplificata, mi basta verificare che tutte le unità presenti coincidano
    // Deve essere presente almeno una variabile, della quale prendo l'unità; poi verifico che tutte le atre siano uguali
    i=line.indexOf('@');
    if (i<0)
      return "";
    unit=pUnit[i];
    //Ora devo verificare che tutte le unità coincidano con unit
    for(i=0; i<line.length(); i++){
      if(line[i]=='@'){
        QString storedUnit=pUnit[i];

        if(storedUnit!=unit){
          unit="";
          break;
        }
      }
    }
    return unit;

}


SVarNums CLineCalc::readVarXYNums(QString varStr){
    /* semplice routine privata che interpreta il nome di una variabile contenuta in varStr secondo lo standard XY. Il nome può quindi essere f#v# o v#.
Se il nome è incorretto ritorna varNum=-1 e fileNum indefinito.
Se il nome è di tipo v# il filenum è defaultFileNum*/
    bool  ok=false;
    int  j=0, k, fileNum=defaultFileNum, varNNum;
    SVarNums varNums;  //E' il valore di ritorno. Se varNNum<0 vi è stato un errore
    switch(varStr.at(0).toLatin1()){
      case 'f':
        j=rxNotDigit.indexIn(varStr,1); //j dovrebbe contenere il primo carattere dopo il numero dopo f
        if(j<0) goto errorReturn;
        fileNum=varStr.mid(1,j-1).toInt(&ok);
        if(ok==false) goto errorReturn;
        if(varStr[j]!='v') goto errorReturn;
      [[clang::fallthrough]]; case 'v':
//        if(varStr[0]=='v') j=0;
        k=rxNotDigit.indexIn(varStr,j+2); //k dovrebbe contenere il primo carattere dopo il numero dopo v. Siccome tale carattere non deve esistere, mi attendo k=-1
        if(k>=0) goto errorReturn;
        varNNum=varStr.remove(0,j+1).toInt(&ok);
        if(ok==false) goto errorReturn;
        varNums.fileNum=fileNum;
        varNums.varNum=varNNum;
        break;
      default:
        varNums.varNum=-1;
    }
    if(varStr.at(0).toLatin1()=='v') varNums.fileNum=defaultFileNum;
    return varNums;

errorReturn:
    varNums.varNum=-1;
    return varNums;
}

QString CLineCalc::substFunsWithPointers(){
    /* In questa funzione si sostituiscono le chiamate a funzione con il carattere
     *  '&', ed in corrispondenza della sua posizione il relativo valore viene messo
     * nel corrispondente puntatore a float.
     * QUESTA FUNZIONE  dà per scontato che sia stata già eseguita
     * substConstsToPointers().
     * Possono invece essere presenti variabili, ma tutte con nomi diversi dai nomi
     * di funzione ammessi.
    */

    int  i,j; //indici generici

   //check che questa routine sia stata chiamata a valle di substPointersToConsts():
   if (!constantsConverted)
     return "functionsToPointers() called ahead of substConstsWithPointers()";

   //Percorro line, individuo i nomi di funzioni e sostituisco con i rispettivi puntatori.

   foreach(i,allowedFunIndexes){
       int index=-1;
       while (1){
         index=line.indexOf(funStr[i],++index);
         if(index>-1){
           //Qui non sono ancora certo di aver trovato una stringa funzione, in quanto potrebbe essere la prima parte di un nome di variabile lungo, ad es. sinModified (contiene sin)
           if(line[index+funStr[i].count()]!='(')
               continue;
/* Ora devo verificare che non ho trovato una stringa funzione illecita per avere a SX del testo, ad es. "xsin"  invece di "sin". Devo quindi escludere lettere e digits
*/
            if(index>0){
                QChar c=QChar(line[index-1]);
                if(c.isDigit() || c.isLetter()){
                  QString str;
                  str=line.mid(index-1,1)+funStr[i];
                  return "string \""+ str +
                  "\" appears to be the name of a non-allowed function.\n";
              }
            }
           //Ho trovato una stringa funzione, deve essere seguita da '('
           //Trovo il carattere dopo la stringa:
           j=rxNotLetterDigit.indexIn(line,index);
           if(j<1 || line[j]!='('){
             QString str;
             if(j<1)
               str=line.mid(index,line.count()-index);
             else
               str=line.mid(index,j-index+1);
             return "string "+ str +
             "appears to be the name of a built-in function\n"
             "but it is not followed by the character '(',\n"
              "that is against rules.";
           }else{
             line[index]='&';
             pFun[index]=fun1[i];
             for (j=1; j<funStr[i].count(); j++)
               line[index+j]=' ';
             //do' la possibilità ad altre istanze della medesima funzione di essere presee in considerazione:
//             break;
             continue;
           }
         } else
         goto nextFunction;
       }
       nextFunction:
       continue;
    }
   return "";
}


QString CLineCalc::substVarsWithPointers(float ** y_){
    /* In questa funzione si sostituiscono le variabili con il carattere '@', ed in corrispondenza della sua posizione, il relativo valore viene messo nel corrispondente puntatore a float.
    Il puntatore viene ritrovato all'interno della lista di puntatori in precedenza tramessi myPointerList, sulla base della corrispondente lista di nomi, contestualemente trasmessa, nameList.
    La funzione viene chiamata a valle di constantsToPointers, quindi si sa che non sono presenti numeri espliciti.
    Ricordiamo che y_ punta già ad un vettore di puntatori alle variabili-funzione di cui si vuole fare l'elaborazione. Questi puntatori vanno però rimappati in pVar in quanto pVar è indicizzato sulla posizione dei rispettivi caratteri in "line". Il numero di elementi di pVar è inferiore a quello di variabili funzione.

*/
   bool eol=false, unary=false, unaryMinus=false;
   int i,j, k1, k2, index;
   QString varStr;
   //
   i=0;
   while(!eol){
      unary=false;
      unaryMinus=false;
      i=rxLetter.indexIn(line,i);
      if(i<0){
          eol=true;
          break;
      }
      j=rxNotLetterDigit.indexIn(line,i+1);
      if(j>=0)
        varStr=line.mid(i,j-i);
      else
        varStr=line.mid(i,line.length());
      line[i]='@';
      //devo verificare se il numero è preceduto da un operatore unario ('+' o '-'). Prima di tutto cerco il più recente '+' o '-':
      k1=max(line.lastIndexOf('+',i-1), line.lastIndexOf('-',i-1));
      //alla posizione k1 vi è un unario se prima di esso, escluso al più un ' ', non vi è nulla o una parentesi aperta
      if(k1==0 && line[1]!='(')
          unary=true;
      else if(k1>0){
          k2=line.lastIndexOf('(',k1-1);
          if(k2==k1-1 || (k2==k1-2 && line[k1-1]==' ')) unary=true;
      }
      if(unary && line[k1]=='-') unaryMinus=true;
      if(unary)line[k1]=' ';

      index=myNameList.lastIndexOf(varStr);
      if(index<0)
          return "variable name not found in memory: \""+varStr+"\"";
//il seguente pVar[i] è un puntatore alla cella contenente il primo elemento della variabile  puntata dal carattere in i-esima posizione di prepLine. I valori numerici saranno pVar[i][j].
/***************** NOTA PER IL DEBUG ***************
 Nell'uso di CLineCalc in associazione con CVarTableComp la funzione "getAndPrepare" è chiamata anche con una variabile y fittizia "yLine" solo per fare un approfondito check sintattico della stringa.
*/
      pVar[i]=y_[index];

      if(unitCharLstFilled){
//        lineFirstChar[i]=unitCharLst[index];
        pUnit[i]=myUnitList[index];
      }
      pUnaryMinus[i]=unaryMinus;
      if(j<0)j=line.length();
      //Devo mettere a blank i caratteri del nome della varabile dopo '@':
      for(int k=i+1;k<j;k++)
          line[k]=' ';
      i+=varStr.length();
   }
   ret="";
   return ret;
}



QString CLineCalc::sum(float x1, float x2, float & y){
    y=x1+x2;
   return "";
}

QString CLineCalc::subtr(float x1, float x2, float & y){
   y=x1-x2;
   return "";
}

QString CLineCalc::power(float x1, float x2, float & y){
   y=powf(x1,x2);
   return "";
}

QString CLineCalc::prod(float x1, float x2, float & y){
   y=x1*x2;
   return "";
}

QString CLineCalc::div(float x1, float x2, float & y){
   if(x2==0)return "division by zero";
   y=x1/x2;
   return "";
}

bool SVarNums::operator== (const SVarNums & x){
    return this->fileNum == x.fileNum && this->varNum == x.varNum;
}

