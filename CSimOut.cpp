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

#include <QDateTime>
#include <qdebug.h>
#include <QLibrary>
#include <QMessageBox>
#include <fcntl.h>
#include <limits.h>
#include "ExcludeATPCode.h"
#include "MTLmatrix.h"
#include "mat.h"
#include "matrix.h"
#include "CSimOut.h"

//le seguenti due righe sono per Matlab (versione 7)
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#define MAXDATACHAR 1000


CSimOut::CSimOut(QWidget *){
    sVars=nullptr;
    varNames=nullptr;
    factorUnit=nullptr;
    y=nullptr;
    /* La seguente variabile è stata modificata il 4-12-99 da false a true. Questo significa che in assenza di modificazioni si assume che il passo di campionamento sia variabile.
    Questo implica in particolare che i files PL4 siano considerati a passo variabile, il che in generale è giusto in quanto il passo può essere modificato durante la simulazione tramite il comando  CHANGE PLOT FREQUENCY, come chiarito da WSM e illustrato con un esempio da Dan Wolff, che ha fornito il file di test dal nome CHG_PLT.pl4
    */
    variableStep=true;
    commasAreSeparators=false;
    useMatLib=false;
    timeVarIndex=0;
    runType=rtUndefined;
    numOfPoints=-1;
}


//----------------------------------------------------------------------
void CSimOut::addPrefix(QString &VarName, QString unit, QString CCBM, int Var){
/* questa funzione crea la prima parte del nome di variable XY a partire dalle informazioni
 *  presenti nella riga corrispondente del file cfg COMTRADE. In particolare dal parametro
 *  unit ricava eventuali fattori moltiplicativi che sono usati per trasformare le
 * rispettive grandezze in unità SI e informazioni riguardo l'unit di misura.
*/
  QChar CCBM_1;
  if(CCBM.length()>0)
    CCBM_1=CCBM[0];
  else
    CCBM_1=-1;  //qui sta per indefinito, e infatti verr convertito nel prefisso con '?'.

  //elimino eventuali spazi iniziali e finali da unit:
  unit=unit.trimmed();
  //Processo eventuale fattore moltiplicativo 'k' o 'm':
  factorUnit[Var]=1;
  if(unit.length()>1 ){
    if(unit[0]=='k'){
      factorUnit[Var]=1000.;
      unit=unit.remove(0,1);
    }else if (unit[0]=='m'){
      factorUnit[Var]=1.f/1000.f;
      unit=unit.remove(0,1);
    }else {
      if(unit.left(3)!="DEG")  unit="?";
    }
  }

  if(unit[0]=='V') VarName="v:"+VarName;
  else if (unit[0]=='A')   VarName="c:"+VarName;
  else if (unit[0]=='W')   VarName="p:"+VarName;
  else if (unit[0]=='J')   VarName="e:"+VarName;
  else if (unit.left(3)=="DEG") VarName="a:"+VarName;
  else if (CCBM_1.isLetter())  VarName=CCBM.left(1)+":"+VarName;
  else{
    factorUnit[Var]=1;
    VarName="?:"+VarName;
  }
}

QString CSimOut::giveAutoUnits(QChar c){
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


struct ParamInfo  CSimOut::giveParamInfo(){
    return paramInfo;
}


struct DataFromModelicaFile  CSimOut::inputMatModelicaData(FILE * pFile){
  /* Questa funzione porta dentro i dati letti da un file matlab modelica per successive
   * elaborazioni.
   * Essi potranno essere tutti trasferiti direttamente in array mySO senza on con
   * eliminazione degli alias
   * Questa ruoutine è pensata per essere richiamtat ssolo da loadfromModelicaMatFile.
   *
   * Se elimino le variabili alias, poi posso avere un hint che dia solo la variabile
   * mantenuta o anche la lista degli alias eliminati, a seconda del secondo parametro
   * passato in loadfromModelicaFile()
   * **** Nota importante. E' stato visto che in tutte le matrici su file c'è scambio
   *      righe/colonne rispeto al file txt. Però per mia chierezza voglio che gli array
   *      abbiano come righe e come colonne quelle che vedo nel file txt.
   *      di conseguenza nella definizione dell'header SCAMBIO LA POSIZIONE DI "nRows"
   *      CON QUELLA DI "nCols"
*/

    struct DataFromModelicaFile fileData;
    fileData.retString="";
    /* Le matrici dataInfo, data_1 e data_2 di fileData vengono allocate nella presente funzione e disallocate, dopo che verranno utilizzate, in loadFromModelicaMatFile(). Notare che una delle due versioni di quest'ultima è sempre chiamata a valle della presente funzione. Quindi c'è empre e comunque una coppia allocatore/disallocatore.*/
    char * pVarName=nullptr;


    //*** fase 1 leggo Aclass e passo avanti.
    struct Header {
//        int type,nRows,nCols,imagf,namlen;  Commentta per via di quanto spiegato nel commento di inizio function
      int type,nCols,nRows,imagf,namlen;
    } header;


    // lettura intestazione: Aclass
    if(fread(&header,sizeof(header),1,pFile)!=1){
        fileData.retString= "Error reading \"Aclass\" in loadFromModelicaMatFile";
        return fileData;
    }
    // lettura nome Aclass:
    pVarName=new char[header.namlen];
    fread(pVarName,size_t(header.namlen),1,pFile);
    if(strcmp(pVarName,"Aclass")!=0){
        delete[] pVarName;
        fileData.retString="Error reading AClass";
        return fileData;
    }
    // lettura Aclass:
    // Faccio staticamente su 44 caratteri in quanto dovrebbe essere sempre una matrice 4x11. caso mai cambio dopo. Così il debug è molto facilitato
    char  aClassData[44];
    fread(aClassData,44,1,pFile);


    //*** fase 2 leggo "name" e passo avanti.
    // lettura intestazione:
    if(fread(&header,sizeof(header),1,pFile)!=1){
        fileData.retString="Error reading \"name\" in loadFromModelicaMatFile";
        return fileData;
    }
    // lettura nome (quindi "name"):
    delete [] pVarName;
    pVarName=new char[header.namlen];
    fread(pVarName,size_t(header.namlen),1,pFile);

    if(strcmp(pVarName,"name")!=0){
        delete[] pVarName;
        fileData.retString="Error reading \"name\"";
        return fileData;
    }
    // lettura dati:
    char *dummyStr;
    dummyStr=new char[header.nCols+1]; //il +1 per via del carattere nullo di fine stringa
//    QString *allNames=new QString[header.nRows];
    for (int i=0; i<header.nRows; i++){
      fread(dummyStr,size_t(header.nCols),1,pFile);
      dummyStr[header.nCols]=0;
//      allNames[i]=QString(nameData);
      fileData.namesLst.append(QString(dummyStr).trimmed());
    }
    delete[] dummyStr;


    //*** fase 3 leggo "description" e passo avanti.
    // lettura intestazione:
    if(fread(&header,sizeof(header),1,pFile)!=1){
        fileData.retString= "Error reading \"description\" in loadFromModelicaMatFile";
        return fileData;
    }
    // lettura nome (quindi "description"):
    delete [] pVarName;
    pVarName=new char[header.namlen];
    fread(pVarName,size_t(header.namlen),1,pFile);

    if(strcmp(pVarName,"description")!=0){
        delete[] pVarName;
        fileData.retString="Error reading \"description\"";
        return fileData;
    }
    // lettura dati:
    dummyStr = new char[header.nCols+1];
    for(int i=0; i<header.nRows; i++){
      fread(dummyStr,size_t(header.nCols),1,pFile);
      dummyStr[header.nCols]=0;
      fileData.descriptionLst.append(QString(dummyStr).trimmed());
    }
    delete[] dummyStr;

    for(int i=0; i<fileData.descriptionLst.count(); i++){
      int start, end;
      if(fileData.descriptionLst[i].count()==0){
        fileData.unitLst.append("");
        continue;
      }
      /* Dymola aggiunge l'unità di misura in parentesi quadre alle descritpio. In tal
       *  modo essa è facilmente individuata. Il seguente codice tiene conto del fatto
       * che le  voci di descriptionLst sono già state trimmed()
       *
       */
       start=fileData.descriptionLst[i].lastIndexOf('[');
       end=fileData.descriptionLst[i].lastIndexOf(']');
       if(start<0||end<0){
         fileData.unitLst.append("");
         continue;
       }

      /* Il seguente codice tenta di comprendere l'unità di misura nelle description senza
       * presupporre che essa sia fra parentesi quadre, ma lasciando che possa essere anche
       * fra tonde.
       * Non va male ma per ora lo disabilito in quanto sembra che Dymola e Modelica)
       * siano rigorose nel mettere le unità sempre in fondo alle descriptions fra
       * parentesi quadre.
      */
      /*
        QChar last=fileData.descriptionLst[i][fileData.descriptionLst[i].count()-1];
        if(last ==')')
          start=fileData.descriptionLst[i].lastIndexOf('(');
        else if(last ==']')
          start=fileData.descriptionLst[i].lastIndexOf('[');
        else{
          fileData.unitLst.append("");
          continue;
        }
       if(start==-1){
         fileData.unitLst.append("");
         continue;
       }
       */

       QString str=fileData.descriptionLst[i];
       str.chop(1);
       str=str.remove(0,start+1);
       // In alcune descriptions fra parentesi trovo contenuto inaccettabile. Scarto i casi a me noti:
       if(str.lastIndexOf('#')>-1 || str.lastIndexOf('=')>-1 ||
               str.contains("flange") || str.contains("abs") || str.contains(",")    )  {
         fileData.unitLst.append("");
         continue;
       }
       //Solo nel caso degli ohm faccio la correzione con carattere unicode. Questo ha una
       //legittimazione specifica dagli standard MSL (tichet su trac.modelica.org #2142,
       // comment 7)
       if(str=="Ohm"||str=="ohm")
           str=QString(0x03A9); //unicode per omega maiuscolo

       fileData.unitLst.append(str);
    }


    //*** fase 4 leggo "dataInfo" e passo avanti.
    // lettura intestazione:
    if(fread(&header,sizeof(header),1,pFile)!=1){
        fileData.retString= "Error reading dataInfo header in loadFromModelicaMatFile";
        return fileData;
    }
    // lettura nome (quindi "dataInfo"):
    delete [] pVarName;
    pVarName=new char[header.namlen];
    fread(pVarName,size_t(header.namlen),1,pFile);
    if(strcmp(pVarName,"dataInfo")!=0){
      fileData.retString= "Error reading \"dataInfo\" in loadFromModelicaMatFile";
      return fileData;
    }

    //Leggo dataInfo.
//    int dataInfoDBG[100][4];

    fileData.dataInfo=CreateIMatrix(header.nRows,header.nCols);
    // lettura dati:
    fileData.numOfAllVars=1;
    for (int i=0; i<header.nRows; i++){
      for (int j=0; j<header.nCols; j++){
        int di;
        fread(&di,4,1,pFile);
        fileData.dataInfo[i][j]=di;
//        if(i<100)
//         dataInfoDBG[i][j]=di;
      }
//      qDebug()<<fileData.dataInfo[i][0]<<fileData.dataInfo[i][1]
//              <<fileData.dataInfo[i][2]<<fileData.dataInfo[i][3];

      if(fileData.dataInfo[i][0]==2)
         fileData.numOfAllVars++;
     }
    fileData.dataInfoRows=header.nRows;


    //*** fase 5 leggo "data_1" e passo avanti.
    // lettura intestazione:
    if(fread(&header,sizeof(header),1,pFile)!=1){
        fileData.retString= "Error reading \"data_1\" in loadFromModelicaMatFile";
        return fileData;
    }

    //lettura nome (quindi "data_1"):
    delete [] pVarName;
    pVarName=new char[header.namlen];
    fread(pVarName,size_t(header.namlen),1,pFile);

    if(strcmp(pVarName,"data_1")!=0){
        delete[] pVarName;
        fileData.retString="Error reading \"data_1\"";
        return fileData;
    }

    //Leggo data_1.
    fileData.data_1=CreateFMatrix(header.nRows,header.nCols);
    //lettura dati:
    for (int i=0; i<header.nRows; i++){
      for (int j=0; j<header.nCols; j++){
        float fn;
        double dn;
        if(header.type==10){
           fread(&fn,sizeof(float),1,pFile);
           fileData.data_1[i][j]=fn;
         }else{
           fread(&dn,sizeof(double),1,pFile);
           fileData.data_1[i][j]=float(dn);
        }
      }
   }


    //*** fase 6 leggo "data_2" e passo avanti.
    // lettura intestazione:
    if(fread(&header,sizeof(header),1,pFile)!=1){
        fileData.retString= "Error reading \"data_2\" in loadFromModelicaMatFile";
        return fileData;
    }
    //lettura nome (quindi "data_2"):
    delete [] pVarName;
    pVarName=new char[header.namlen];
    fread(pVarName,size_t(header.namlen),1,pFile);

    if(strcmp(pVarName,"data_2")!=0){
        delete[] pVarName;
        fileData.retString="Error reading \"data_2\"";
        return fileData;
     }

    //Leggo data_2.
    fileData.data_2=CreateFMatrix(header.nRows,header.nCols);
    // lettura dati:
    for (int i=0; i<header.nRows; i++){
      for (int j=0; j<header.nCols; j++){
        float fn;
        double dn;
        if(header.type==10){
           fread(&fn,sizeof(float),1,pFile);
           fileData.data_2[i][j]=fn;
         }else{
           fread(&dn,sizeof(double),1,pFile);
           fileData.data_2[i][j]=float(dn);
        }
      }
   }
    fileData.numOfData2Rows=header.nRows;
    fileData.numOfData2Cols=header.nCols;

   delete [] pVarName;
   return fileData;
}

QString CSimOut::loadFromAdfFile(QString fullName, bool csv){
/* Function per la lettura delle informazioni da un file avente la semplice struttura
 * che ho definito per l'estensione ADF (Ascii Data File) La sua descrizione completa è
 *  nel file "Input formats and naming conventions".
*/
  char  *str=nullptr, //Stringa per contenere le prime due righe del file.
        *str1, *pStr, *pBuffer,
      *fStr,  //Stringa che stabilisce il formato di lettura dei numeri
      *tStr; //Stringa per la ricerca dei token
  bool autoStep=false;
  bool acceptsCommas=false;
  int i, iRow, c, i1, iName,
              rowLength[3]; //Numero di Bytes delle prime tre righe
  float autoStepValue=0; //inizializzazione solo per evitare un warning
  QString retStr="", XVarName;
  long iL;
  FILE * fpIn;

  fileType=ADF;
  QFileInfo fi(fullName);
  runType=rtUndefined;
  allowsPl4Out=false;
  fpIn=fopen(qPrintable(fullName),"r");
  if(fpIn==nullptr){
      return "Unable to open file "+fullName+ "\n(does it exist?)";
  }
  fileInfo=fi;
  //Misuro la lunghezza delle prime tre righe, poi il numero di righe successive, poi riavvolgo.
  for(iRow=0; iRow<3; iRow++){
    i=0;
    do{
      c=fgetc(fpIn);   i++;
    }while(c!=EOF && c!='\n');
    rowLength[iRow]=i+1;
  }

  //Per calcolare il numero di punti, conteggio il numero di caratteri '\n'
  i=1;
  do{
    c=fgetc(fpIn);
    if(c=='\n')i++;
  }while(c!=EOF);
  numOfPoints=i;
  if(csv)
    numOfPoints++;

  rewind(fpIn);

  acceptsCommas=commasAreSeparators || csv ;
  if(acceptsCommas){
    fStr=strdup("%f,");
    tStr=strdup(" \t,");
  }else{
    fStr=strdup("%f");
    tStr=strdup(" \t");
  }
  //Seconda passata. Per prima cosa analizzo la prima riga:
  if(!csv){
    str=new char[rowLength[0]];
    fgets(str,rowLength[0],fpIn);
    //Elimino l'eventuale commento presente nella prima riga:
    pStr=strstr(str,"//");
    if(pStr){
      *pStr='\0';
    }
    pStr=strstr(str,"%");
    if(pStr){
      *pStr='\0';
    }
    //verifico se è presente l'opzione "accepts commas":
    pStr=strstr(str,"/ac");
    if(pStr){
       acceptsCommas=true;
       *pStr='\0';
    }
    if(acceptsCommas){
      fStr=strdup("%f,");
      tStr=strdup(" \t,");
    }else{
      fStr=strdup("%f");
      tStr=strdup(" \t");
    }
    //Ora la stringa può contenere il passo automatico il nome della variabile x e null'altro.
    QRegExp notSeparators, separators;
    if(acceptsCommas){
      separators=QRegExp("[, \t]");
      notSeparators=QRegExp("[^, \t]");
    }else{
      separators=QRegExp("[ \t]");
      notSeparators=QRegExp("[^ \t]");
    }

    //La stringa residua può contenere step e xVariableName (entrambi opzionali)
    //La analizzo con una QString in cui faccio le ricerche attraverso la regular expression "separators".
    QString qString=QString::fromLatin1(str);
    QString itemStr;
    //Tolgo eventuali separatori iniziali:
    i=qString.indexOf(notSeparators);
    qString=qString.mid(i);
    //Ora in itemStr metto la stringa definente l'autostep:
    i=qString.indexOf(separators);
    itemStr=qString.mid(0,i);
    bool  ok=false;
    autoStepValue=itemStr.toFloat(&ok);
    if(ok){
      autoStep=true;
      //A questo punto devo verificare se è anche stato specificato un nome per la variabile automatica
      //Come prima cosa individuo il primo separator dopo l'autostepValue:
      i=qString.indexOf(separators);
      qString=qString.mid(i);
      // Se non ho trovato il nome della variabile definisco il nome default, altrimenti lo leggo. Tolgo tutti i separatori e verifico se rimane una stringa vuota:
      i=qString.indexOf(notSeparators);
      if(i==-1){
        XVarName="X_(auto)";
      }else{
        i=qString.indexOf(notSeparators);
        // qui i indica il primo carattere del nome automatico
        qString=qString.mid(i);
        XVarName=qString;
        //dopo qString ci devono essere solo separatori o carattere di fine riga. Eseguo quindi questa verifica
        i=qString.indexOf(separators);
        if (i<0)
          i=qString.indexOf("\n");
//        i=qString.mid(i).indexOf(notSeparators);

        if(i<0)
          return ("Error: the first row contains invalid characters after the x variable name");
      }
    }else
      autoStep=false;
    delete[] str;
  }
  //Seconda passata. Ora analizzo la seconda riga (la prima per i csv):
  if(csv){
    str=new char[rowLength[0]];
    fgets(str,rowLength[0],fpIn);
  }else{
    str=new char[rowLength[1]];
    fgets(str,rowLength[1],fpIn);
  }
 //Ora utilizzo str per calcolare il numero di variabili e str1 per effettuare la successiva lettura. Bypasso eventuali spazi iniziali:
  i1=-1;
  do
    i1++;
  while(str[i1]==' '||str[i1]=='\t');
  //Ora cerco i nomi come "token":
  pStr=str+i1;
  str1=strdup(pStr);
  //Elimino il carattere '\n' in fondo all'ultima variabile
  //Nel caso di MAC prima di \n ho anche un \r che devo eliminare

  pStr[strlen(pStr)-1]=0;
  if(pStr[strlen(pStr)-1]=='\r')
      pStr[strlen(pStr)-1]=0;

  pBuffer=strtok(pStr,tStr);
  if(pBuffer==nullptr){
    retStr="Invalid file structure (invalid row of variable names)";
    goto Return;
  }
  /*** Occorre notare che in Mac a fine riga leggo "\r\n" invece di "\n".
   pertanto (Sept 2012) modifico il codice in modo da considerare questa eventualità.  */
  for(iName=1; 1; iName++){
    pBuffer=strtok(nullptr,tStr);
    if(pBuffer==nullptr)break;
    if(*pBuffer=='\r'){
      *pBuffer=0;
      break;
    }
  }
  numOfVariables=iName+autoStep;

  delete[] varNames;
  varNames=new QString[numOfVariables];
  delete [] sVars;
  sVars=new SVar[numOfVariables];

  //Elimino il carattere '\n' in fondo all'ultima variabile
  pStr=str1;
  pStr[strlen(pStr)-1]=0;
  pBuffer=strtok(pStr,tStr);
  if(pBuffer==nullptr){
    retStr="Invalid file structure (invalid row of variable names)";
    goto Return;
  }
  if(autoStep) varNames[0]=XVarName;
  varNames[autoStep]=QString(pBuffer);
  for(iName=autoStep+1; 1; iName++){
    pBuffer=strtok(nullptr,tStr);
    if(pBuffer==nullptr)break;
    if(*pBuffer=='\r'){
      *pBuffer=0;
      break;
    }
    varNames[iName]=QString(pBuffer);
  }

  if(csv &&trimQuotes){
      int sc;
      for(iName=0; iName<numOfVariables; iName++){
          sc=-1;
          QString str=varNames[iName];

          //Elimino i double quotes iniziali:
          while (++sc<str.length()){
            if(str[sc]=='"')
              str[sc]=' ';
            else
              break;
          }
          sc=str.length();
     //Elimino i double quotes finali:
          while (--sc>=0){
            if(str[sc]=='"')
              str[sc]=' ';
            else
             break;
          }
          str=str.trimmed();

          varNames[iName]=str;
      }
  }
  //Ora alloco spazio per la matrice dei dati ed effettuo la lettura:
  if(y!=nullptr)
      DeleteFMatrix(y);
  y=CreateFMatrix(numOfVariables,numOfPoints);
  if(y==nullptr){
    retStr="Unable to allocate memory for variables";
    goto Return;
  }
  //Eventuale assegnazione valori alla variabile X (auto):
  if(autoStep){
    y[0][0]=0;
    for(iL=1; iL<numOfPoints; iL++)
      y[0][iL]=y[0][iL-1]+autoStepValue;
  }
  //Soltanto la prima riga di valori la leggo con conteggio del numero di dati presenti. Questo mi consente di intercettare l'errore più frequente nei files ADF, ovvero numero di variabili specificate nella seconda riga incongruente con il numero di dati per riga presenti.
  delete str;
  str=new char[rowLength[2]];
  fgets(str,rowLength[2],fpIn);
/*** In Mac a fine riga leggo "\r\n" invece di "\n".
   Pertanto (Sept 2012) modifico il codice in modo da considerare questa eventualità.  */
  pStr=str;
  strtok(pStr,tStr);
  i=sscanf(str,"%f",&y[autoStep][0]);
  // Se non è GV.PO.CommasAreSeparators né acceptCommas tStr contiene ',' e quindi str non contiene ','; altrimenti se nel file di input è presente ',' esso è in str, e questo è erroneo. Faccio questa verifica:
  if(strchr(str,',')){
      retStr="The data section of file contains the invalid  \',\'.\n"
             "If you want to use the character \',\' between adjacent numbers, you should either\n"
             "select the program option \"Accept commas as separators in input ADF files\"\n"
              " or use the option \"/ac\" in the adf file";
      goto Return;
  }
  for(i1=autoStep+1; 1; i1++){
    pBuffer=strtok(nullptr,tStr);
    if(pBuffer==nullptr || *pBuffer=='\n' || *pBuffer=='\r')break;
    try{
      i+=sscanf(pBuffer,fStr,&y[i1][0]);
    }catch(...) {
     //Un caso frequente di errore di lettura è quando il separatore decimale è errato.
     //Per fornire un messaggio di errore preciso verifico se è questo il caso:
     if(strchr(fStr,','))
       retStr="a number in row 3 contains invalid character \'.\'.";
     else
       retStr="invalid number in row 3.\n";
      goto Return;
    }
  }
  if(i!=numOfVariables-autoStep){
    QString iStr, nVarStr;
     iStr.setNum(i);
    nVarStr.setNum(numOfVariables);
    retStr="The number of numerical values in row 3 is " +iStr+ ";\n instead, it must be equal to "+nVarStr+", the number of variable names read from row 2.";
    goto Return;
  }

  for(iL=1; iL<numOfPoints; iL++){
    for(i1=autoStep; i1<numOfVariables; i1++){
      i=fscanf(fpIn,fStr,&y[i1][iL]);
      /* l'uso di fscanf funziona sia se i numeri sono separati da spazi e/o tab, sia da
       * una virgola. Sono da evitare combinazioni di spazi/tab e virgole, in quanto i
       * risultati possono risultare imprevedibili. Ad esempio un tab seguito da una
       * virgola non è un corretta spaziatura.*/
      if(i!=1){
        if(iL==1){
          retStr="Only a single row of data was successfully read\n"
          "that is considered insufficient to proceed";
        }else{
          QString msg="The selected Data File contains only "+QString::number(iL)+
                      " valid points\n"
                       "that is not consistent with a number of rows of "+
                        QString::number(numOfPoints+2);
//          QMessageBox::warning(parent, "SimOut warning",msg, QMessageBox::Ok);
          return (msg);
        }
        goto Return;
      }
    }
  }
  delete [] sVars;
  sVars=new SVar[numOfVariables];
  for(i1=autoStep; i1<numOfVariables; i1++){
      sVars[i1].unit=giveAutoUnits(varNames[i1][0]);
  }
Return:
  fileType=ADF;
  variableStep=!autoStep;
  //Disallocazione di variabili locali allocate dinamicamente:
  delete[] str;
  free(str1);
  //chiusura file e uscita:
  fclose (fpIn);
  return retStr;
}

QString CSimOut::loadFromComtradeFile(QString cfgFileName){
  /* Gestione del valore di ritorno (messaggio d'errore)
  Emetto due tipologie di messaggi d'errore, subito dopo la label return_.
  1) messaggio d'errore standard in cui specifico il tipo del file e il numero
    della riga. Questo tipo di messaggio viene emesso se si arriva alla label "_return"
    con numRow!=0; e retStr="".
    Il messaggio contiene l'estensione del file in cui si è verificato l'errore e il
    relativo numero di riga, specificato in rowNum
  2) messaggio d'errore specifico. In questo caso il messaggio è contenuto in retStr
    Questo messaggio viene emesso se si arriva alla label "_return" con retStr!=""
    e numRow!=0
  3) se numRow è 0 viene ritornato "", il che significa: Nessun errore.


  Se non devo emettere un messaggio d'errore arrivo in fondo al file (label "_return" con
  rowNum=0; altrimenti rowNum contiene il numero della riga in cui si Ã¨ verificato
  l'errore.

  ********  Nomenclatura  nel caso di presenza di files binari con dati digitali *******
  - Il nome "signal" o la frazione di nome "sig" fa riferimento al numero di grandezze
    elementari nella registrazione.
  - la frazione di nome "comb" sta ad indicare variabili entro cui piÃ¹ segnali digitali
    sono combinati

  *** Nel caso di file ascii, i nomi  fullDigiVars e extraDigiSigs
  *** vanno considerati indefiniti

  */
  char *row, *pToken;
  short int si;
  unsigned int u, u1;
  int i, iErr, c, var, point, len, maxLen, *min, *max;
  int rowNum, totSignals, analogSigs, digiSigs,
          digiCombs=0, extraDigiSigs=0;  //Initializing to zero to avoid warning message
  float *factor, *offset, skew;
//  float triggerTime;
//  float startTime;
  FILE *pFile;
  QString msg, extStr="???", retStr, CCBM, unit, datFileName, version;
  QString auxStr, subType;
  QFileInfo fi(cfgFileName);

  fileType=COMTRADE;
  runType=rtUndefined;
  fileInfo=fi;
  allowsPl4Out=false;
  /* Fase 1: Preparazione e scrittura file di estensione cfg*/
  //Apertura file:
  pFile=fopen(qPrintable(cfgFileName),"r");
  if(pFile==nullptr)return "Unable to open CFG file (does it exist?)";
  //Prima passata; leggo la lunghezza massima di riga:
  maxLen=0;
  i=0;
    do{
        c=fgetc(pFile);
    i++;
    if(c=='\n'){
      maxLen=max(int(i),maxLen);
      i=0;
    }
    }while(c!=EOF);
  rewind(pFile);
  row=new char[maxLen+1];
  rowNum=0;
  //Leggo la versione del formato sulla prima riga:
  fgets(row,maxLen+1,pFile);  rowNum++;
  StrTok.getStr(row);
  StrTok.giveTok();
  //A questo punto è stato letto "StationName", parametro non critico
  StrTok.giveTok();
  //A questo punto è stato letto  il "recording device indicator", parametro non critico
  pToken=StrTok.giveTok();
  if(pToken==nullptr)
    version="1991";
  else{
    version=QString(pToken);
    version=version.mid(0,4);
    if(version!="1999"&&version!="1997"){
      if(version[0]=='\n') version="\\n";
      QMessageBox msgBox;
      msgBox.setText(
         "A problem has been found in the first row of the input file:\n"
         "the revision of COMTRADE standard appears to be 1999, but \""+version+"\"\n"
         "has been found in the rev_year field instead of \"1999\".\n"
         "The file has been assumed to be 1999 compliant.\n") ;
      msgBox.exec();
      version="1999";
    }
  }
  if(StrTok.commas>1 && version=="1991"){
    delete[] row;
    return "Input file(s) are not an IEEE ASCII COMTRADE set";
  }

  //Dati dalla seconda riga:
  fgets(row,maxLen+1,pFile);  rowNum++;
    strtok(row,",");
  sscanf(row,"%d",&totSignals);
    pToken=strtok(nullptr,",");
  auxStr=QString(pToken);
  len=auxStr.size();
  auxStr=auxStr.mid(0,len-1);
  sscanf(qPrintable(auxStr),"%d",&analogSigs);

  digiSigs=totSignals-analogSigs;
  //La seguente riga è conseguenza del fatto che nel Numero letto dal file non è conteggiato
  //il tempo, che invece in Simout è conteggiato come una variabile
  numOfVariables=totSignals+1;
  delete[] varNames;
  delete [] sVars;
  sVars=new SVar[numOfVariables];

  varNames=new QString[numOfVariables];
  factor=new float[numOfVariables];
  delete[] factorUnit;
  factorUnit=new float[numOfVariables];
  offset=new float[numOfVariables];
  min=new int[numOfVariables];
  max=new int[numOfVariables];
  varNames[0]="t";
  //Righe di definizione dei segnali (con l'indice parto da 1 e arrivo a Signals+1
  //perché nei miei vettori l'indice 0 è relativo alla variabile tempo)
  extStr="CFG";
  for (var=1; var<analogSigs+1; var++){
    fgets(row,maxLen+1,pFile); rowNum++;
    StrTok.getStr(row);
    pToken=StrTok.giveTok();
    //A questo punto pToken contiene il numero d'ordine, parametro critico.
    //Verifica di errori per parametri critici:
    if(pToken==nullptr || *pToken==0)goto _return;
    //(Non uso in alcun modo il numero d'ordine)
    pToken=StrTok.giveTok();
    //A questo punto pToken contiene il nome (identifier), parametro non critico
    //Verifica di errori per parametri non critici:
    if(pToken==nullptr)goto _return;
    varNames[var]=QString(pToken);
    pToken=StrTok.giveTok();
    //A questo punto pToken contiene la "fase", parametro non critico
    //Verifica di errori per parametri non critici:
    if(pToken==nullptr)goto _return;
    //(Non uso in alcun modo la fase)
    pToken=StrTok.giveTok();
    //A questo punto pToken contiene il "ccbm", parametro non critico
    //Verifica di errori per parametri non critici:
    if(pToken==nullptr) goto _return;
    CCBM=QString(pToken);
    pToken=StrTok.giveTok();
    //A questo punto pToken contiene l'unità di misura, parametro critico

    /* In Febbraio 2018 Perry Clements ha fornito il file FieldOverCurrent.cfg
     * che non conteneva
     * l'unità di misura, generato da Voltage regulator  Basler DECS-250.
     * Sebbene secondo lo standard del 1999 il pu sia un parametro critico,
     * non crea problemi accettare l'assenza di unità di misura. E' molto
     * probabile che in una versione dello standard successiva al 1999 questo
     * parametro sia stato convertito in non critico.
     * Pertanto in assenza di unità invece di emettere un messaggio di errore
     * ora  metto una unità vuota.
     * */
//    if(pToken==NULL || *pToken==0)  goto _return;
    if(pToken==nullptr)
      unit="";
    else
      unit=QString(pToken);
    /*La seguente riga vuole evitare un errore di "access violation" alla successiva chiamata a addPrefix se unit è vuoto.
    */
    if(unit=="")
      unit="X";
    pToken=StrTok.giveTok();
    //A questo punto pToken contiene il fattore (multiplier), parametro critico
    //Verifica di errori per parametri critici:
    if(pToken==nullptr || *pToken==0)goto _return;
    sscanf(pToken,"%f",&factor[var]);
    pToken=StrTok.giveTok();
    //A questo punto pToken contiene il l'offset (adder), parametro critico
    //Verifica di errori per parametri critici:
    if(pToken==nullptr || *pToken==0)goto _return;
    sscanf(pToken,"%f",&offset[var]);
    pToken=StrTok.giveTok();
    //A questo punto pToken contiene lo "skew", parametro non critico
    //Verifica di errori per parametri non critici:
    if(pToken==nullptr) goto _return;
    sscanf(pToken,"%f",&skew);
    pToken=StrTok.giveTok();
    //A questo punto pToken contiene il "min", parametro critico
    //Verifica di errori per parametri critici:
    if(pToken==nullptr || *pToken==0)goto _return;
    sscanf(pToken,"%d",&min[var]);
     pToken=StrTok.giveTok();
    //A questo punto pToken contiene il "max", parametro critico
    //Verifica di errori per parametri critici:
    if(pToken==nullptr || *pToken==0)goto _return;
    iErr=sscanf(pToken,"%d",&max[var]);
    if(iErr!=1)goto _return;

    //L'ultima parte della riga, specifica del formato 1999 non la leggo; pertanto
    //il presente loop di for è valido sia per il formato 1991 che per il 1999.
    addPrefix(varNames[var],unit,CCBM, var);
    sVars[var].unit=unit.simplified();
//    int iii=0;
  }
  for (var=analogSigs+1; var<numOfVariables; var++){
    fgets(row,maxLen+1,pFile); rowNum++;
    StrTok.getStr(row);
    pToken=StrTok.giveTok();
    //A questo punto pToken contiene il numero d'ordine, parametro critico.
    //Verifica di errori per parametri critici:
    if(pToken==nullptr || *pToken==0)goto _return;
    //(Non uso in alcun modo il numero d'ordine)
    pToken=StrTok.giveTok();
    //A questo punto pToken contiene il nome (identifier), parametro non critico
    //Verifica di errori per parametri non critici:
    if(pToken==nullptr)goto _return;
    varNames[var]=QString(pToken);
    pToken=StrTok.giveTok();
    //A questo punto pToken contiene la "fase", parametro non critico
    //Verifica di errori per parametri non critici:
    if(pToken==nullptr)goto _return;
    //(Non uso in alcun modo la fase)

    addPrefix(varNames[var]," ","d", var);
    //L'ultima parte della riga, specifica del formato 1999 non la leggo; pertanto
    //il presente loop di for è valido sia per il formato 1991 che per il 1999.
    factor[var]=1;
    offset[var]=0;
  }
  //Lettura della frequenza;
  float frequency;
  fgets(row,maxLen,pFile);
  sscanf(row, "%f",&frequency);
  rowNum++;
  // Ora c'è la parte relativa ai tempi di campionamento nrates (per me numOfRates). Nell'implementazione attuale ammetto solo un unica frequenza di campionamento per tutto il file.
  // Se mi verrà  fornito un esempio a frequenza multipla aggiornerà il SW
  int numOfRates;
  fgets(row,maxLen,pFile);
  rowNum++;
  sscanf(row, "%d",&numOfRates);
  //Se ho più di un tempo di campionamento ho un file non supportato ed emetto un messaggio d'errore:
  if(numOfRates>1){
        retStr="\nThe file contains more than one sampling rate;"
               " this is currently not supported.\n"
               "\nIf you send your multi-sample data set (CFG, and both ASCII and BINARY DAT)"
               "\nto   massimo.ceraolo@unipi.it\n"
               "you will get a chance of having this limitation removed.\n";
        goto _return;
    }
  //lettura di samp e endsamp (per me pari a sampleRate e numOfPoints):
  float sampleRate;
  fgets(row,maxLen,pFile);
  rowNum++;
  StrTok.getStr(row);
  pToken=StrTok.giveTok();
  //A questo punto pToken contiene samp (sampleRate), critico, reale
  //Verifica di errori per parametri critici:
  if(pToken==nullptr || *pToken==0)goto _return;
  sscanf(pToken,"%f",&sampleRate);
//  StrTok.getStr(row);
  pToken=StrTok.giveTok();
  sscanf(pToken,"%d",&numOfPoints);
  //A questo punto pToken contiene il nome (identifier), parametro non critico
  //Verifica di errori per parametri non critici:
  if(pToken==nullptr)goto _return;

  //Dalle due righe della data prendo solo i numeri finali per determinare l'istante iniziale nel caso di nSamples>0 se vi è un pretrigger.
  fgets(row,maxLen,pFile); rowNum++;
  auxStr=QString(row);
  auxStr=auxStr.mid(auxStr.lastIndexOf(":")+1);
//  startTime=auxStr.toFloat();
  fgets(row,maxLen,pFile); rowNum++;
  auxStr=QString(row);
  auxStr=auxStr.mid(auxStr.lastIndexOf(":")+1);
//  triggerTime=auxStr.toFloat();

  //lettura di "BINARY o ASCII:
  fgets(row,maxLen,pFile); rowNum++;
  msg=QString(row);
  /* subtype può essere o "ASCII" o "BINARY". msg contiene il subtype più il carattere \n (DOS) o il subtype più la coppia \r\n (MAC). */
  subType=msg.left(6);
  if(subType[5]=='\n'||subType[5]==' '||subType[5]=='\r') subType=msg.left(5);

  if(subType!="ASCII" && subType!="BINARY" ){
    retStr="Input file(s) are not an IEEE COMTRADE set";
    goto _return;
  }
  if(subType=="BINARY"){
    digiCombs=digiSigs/16;
    extraDigiSigs=digiSigs-16*digiCombs;
  }

  if(version=="1999"){
    fgets(row,maxLen,pFile);
    sscanf(row,"%f",&factor[0]);
    // I dati sono registrati in microsecondi. Pertanto un factor di 1 significa un fattore effettivo di 1e-6.
    // Correzione 03/03/2016:
    factor[0]*=1.0e-6f;
  }else
    factor[0]=1.0e-6f;
  offset[0]=0;

  fclose(pFile);

  /* Fase 2: Preparazione e lettura file di estensione dat*/
  //Apertura file:
  rowNum=0;
  extStr="DAT";
  datFileName=cfgFileName.left(cfgFileName.size()-3)+"DAT";
  if(subType=="ASCII")
    pFile=fopen(qPrintable(datFileName),"r");
  else
    pFile=fopen(qPrintable(datFileName),"rb");
  if(pFile==nullptr)
    return "Unable to open DAT file (does it exist?)";
  do{
        c=fgetc(pFile);
    i++;
    if(c=='\n'){
      maxLen=max(int(i),maxLen);
      i=0;
    }
    }while(c!=EOF);
  rewind(pFile);

  delete[] row;
  row=new char[maxLen+1];

    //Ora alloco spazio per la matrice dei dati ed effettuo la lettura:
    if(y!=nullptr)DeleteFMatrix(y);
    y=CreateFMatrix(numOfVariables,numOfPoints);
  if(y==nullptr){
        retStr="Unable to allocate memory for variables";
        goto _return;
    }

  if(subType=="ASCII"){
    for(point=0; point<numOfPoints; point++){
      fgets(row,maxLen+1,pFile);
        //pToken=strtok(Row,",");
        strtok(row,",");
      for(var=0; var<numOfVariables; var++){
        pToken=strtok(nullptr,",");
        sscanf(pToken,"%d",&i);
        y[var][point]=factor[var]*i+offset[var];
      }
    }
  } else {
    for(point=0; point<numOfPoints; point++){
      // 1) lettura del numero di campione "n":
      fread (&u, 4, 1, pFile);
      // 2) lettura del tempo "timestamp":
      fread (&u1, 4, 1, pFile);
      //Ma se numOfRates è diverso da 0 uso quello per calcolare il tempo:
      if(numOfRates>0){
          //Al tempo andrebbe aggiunto initialTime per avere una rappresentazione completa di quello che c'è sul file. Però PlotXY è concepito per far partire i segnali da 0 e quindi non faccio la traslazione (il decremento perché il primo campione è 1, ma il relativo istante 0):
//          y[0][point]=(float)--u/sampleRate;
          y[0][point]=float(point)/sampleRate;
      }
      else
         y[0][point]=factor[0]*u1;
      // 3) lettura variabili analogiche:
      for(var=1; var<analogSigs+1; var++){
        fread (&si, 2, 1, pFile);
        y[var][point]=factor[var]*si+offset[var];
      }
      // 4) lettura variabili digitali a canale combinato pieno:
      for(var=analogSigs; var<analogSigs+16*digiCombs; var+=16){
        fread (&si, 2, 1, pFile);
        for(i=0;i<16;i++){
//          y[var+i][point]=si&(short int)(2^i);
          y[var+i][point]=si&short(2^i);
        }
      }
      // 5) lettura variabili digitali a canale combinato parzialmente vuoto:
      if(extraDigiSigs){
        var=analogSigs+16*digiCombs;
        fread (&si, sizeof(short int), 1, pFile);
        for(i=0;i<extraDigiSigs;i++){
//          y[var+i][point]=si&(short int)(2^i);
          y[var+i][point]=si&short(2^i);
        }
      }
    }
  }
  //Processo FactorUnit:
  for(var=1; var<numOfVariables; var++){
    if(factorUnit[var]!=1.0f)
      for(point=0; point<numOfPoints; point++)
        y[var][point]*=factorUnit[var];
  }
  fclose(pFile);
  rowNum=0;
_return:

  for(int i1=0; i1<numOfVariables; i1++){
      sVars[i1].unit=giveAutoUnits(varNames[i1][0]);
  }

  QString str;
  if(rowNum==0)
    retStr="";
  else{
    fclose(pFile);
    if(retStr=="")
      retStr="Error in "+extStr+" file, row n. "+str.setNum(rowNum);
  }
  fileType=COMTRADE;
  delete[] row;
  delete[] factor;
  delete[] offset;
  delete[] min;
  delete[] max;
  return retStr;
}

QString CSimOut::loadFromLvmFile(QString fileName) {
    /* Funzione per la lettura dati da file LVM (Lab View Measurement)
  La tecnica che adotto è la seguente:
  1. leggo il numero di caratteri di tutte le righe dell'intestazione di file e del
     primo segmento
  2. alloco spazio per la lettura e leggo le riche in altrettante stringhe
  3. analizzo tutte le stringhe
  4. alloco spazio per le variabili e leggo i numeri
  ## scarto tutti i segmenti oltre il primo
 */

  // Operazioni preliminari di apertura del file
  bool autoStep=false;  //verrà usato quando avrò un esempio di XColumns One
  int c, i, iL, i1, iChar, iName, numAsterisks, numRead;
  int numHeadRows=0;
  int maxHeadRowLen;
  char dataRow[MAXDATACHAR], *pDataRow=dataRow;
  char *headerRow, *pBuffer, *pStr;
  char shortRow[20];
  char tokStr[4], *tStr=tokStr; //Stringa per la ricerca dei token
  QFileInfo fi(fileName);

  fileType=LVM;
  dataRow[MAXDATACHAR-1]='\0';
  // Elenco delle Keys (ordine alfabetico, nomi stile Qt) e valori default (quando esistono):
  int channels=0;
  char decimalSeparator='.';
//  QString date;
//  QString multiHeadings="No";
  char separator[2]={'\t', '\0'};
  QString Time, xColumns="One", retStr="", xVarName;
  tokStr[0]='\t';
  tokStr[1]='\0';
  FILE * fpIn;
  fpIn=fopen(qPrintable(fileName),"r");
  if(fpIn==nullptr){
        return "Unable to open file (does it exist?)";
  }
  fileInfo=fi;

 // 1. lettura del numero di caratteri di tutte le righe delle intestazioni file e
 //    primo segmento.
 //    Per prima cosa cercaro la fine dell'intestazione del primo
 //    segmento, e contemporaneamente calcolo la massima lunghezza di linea. La fine del
 //    segmento la trovo cercando il sesto '*' e poi raggiungendo il successivo '\n'.
  numAsterisks=0;
  maxHeadRowLen=0;
  iChar=0;
  while(1){
    c=fgetc(fpIn);
    iChar++;
    if(c==EOF) break;
    if(c=='*')numAsterisks++;
    if(c=='\n'){
      numHeadRows++;
      if(iChar>maxHeadRowLen)  maxHeadRowLen=iChar;
      iChar=0;
    }
    if(numAsterisks==12)break;
  }
  numHeadRows++;
  //Prima di riavvolgere calcolo il numero di punti; per farlo conteggio il numero
  // di caratteri '\n'
    i=0;
    do{
      c=fgetc(fpIn);
      if(c=='\n')
      i++;
    }while(c!=EOF);
    /*Il valore di numOfPoints è i-3. Le 3 unità da togliere sono i caratteri '\n' a fine
   riga **End_of_Header***," il fine riga contenente i nomi delle variabili, e l'ultima
   riga vuota a fine file.
   Siccome l'utente potrebbe aver ritoccato il file, ad esempio tagliando alcuni punti,
   e non aver lasciato la riga vuota in fondo, faccio una routine di lettura robusta
   che accetta anche questa situazione, ed aumento di 1 NumOfPOints, per allocare
   sufficiente spazio nella matrice. La lettura avverrà poi di tutti i punti presenti
   sul file.
  */
    numOfPoints=i-2;

  // Ora che so quante sono le righe dei due header, riavvolgo e procedo con la lettura dei dati, separando "key" da "value":
  headerRow=new char[maxHeadRowLen+2]; //MaxHeadRowLen non contiene '\n' e  '\0'!
  rewind(fpIn);
  bool SecondHeader=false;

  //A questo punto percorro le righe dei due header, e quando trovo un campo che devo
  //interpretare ne valuto il valore.
  for(int i=0;i<numHeadRows; i++){
    headerRow=fgets(headerRow,maxHeadRowLen+1,fpIn);
    //Channels
    if(strncmp("Channels",headerRow,8)==0)
        numRead=sscanf(headerRow+9,"%d",&channels);
    //DecimalSeparator
    if(strncmp("Decimal_Separator",headerRow,17)==0)
        numRead=sscanf(headerRow+18,"%c",&decimalSeparator);
    //The variable numRead is needed only for debugging purposes.
    //The following row has the only purpose of avoiding a "Wunused-but-set-variable warning (here is not necessary):
//    numRead++;

    //XColumns
    if(strncmp("X_Columns",headerRow,9)==0){
        strncpy(shortRow,headerRow+10,7); //il 10 perché occorre escludere anche '\t'
        xColumns=QString(shortRow);
    }
    if(strncmp(headerRow,"***End_of_Header***",19)==0){
       if(SecondHeader){
          break;
       }else{
          SecondHeader=true;
       }
    }
  }
  headerRow=fgets(headerRow,maxHeadRowLen+1,fpIn);
  //Il seguente if deve fare i conti con il fatto che in Windows alla fine della riga ho soltanto '\n', mentre con Mac o '\r\n'. Ad esempio per il caso di "No" con Win ho "No\n", con Mac "No\r\n". Pertanto il seguente confronto è realizzato escludendo i terminatori.
  if(xColumns.left(2)=="No"){
    numOfVariables=channels;
  }else if (xColumns.left(3)=="One"){
    numOfVariables=channels+1;
  }else if (xColumns.left(5)=="Multi"){
    numOfVariables=2*channels;
  }

  //A questo punto headerRow contiene i nomi delle variabili.
  delete[] varNames;
  varNames=new QString[numOfVariables];

  //Elimino il carattere '\n' in fondo all'ultima variabile (in Mac  i \r\n)
  pStr=headerRow;
  pStr[strlen(pStr)-1]=0;
  // se c'è rimasto '\r' lo tolgo:
  if(pStr[strlen(pStr)-1]=='\r')
      pStr[strlen(pStr)-1]=0;
  pBuffer=strtok(pStr,tStr);
  if(pBuffer==nullptr){
    retStr="Invalid file structure (invalid row of variable names)";
    goto Return;
  }
  if(autoStep) varNames[0]=xVarName;
  varNames[autoStep]=QString(pBuffer);
  for(iName=autoStep+1; iName<numOfVariables; iName++){
    pBuffer=strtok(nullptr,tStr);
    if(pBuffer==nullptr)break;
    varNames[iName]=QString(pBuffer);
  }

  //Ora alloco spazio per la matrice dei dati ed effettuo la lettura:
  if(y!=nullptr)DeleteFMatrix(y);
  y=CreateFMatrix(numOfVariables,numOfPoints);
  if(y==nullptr){
    retStr="Unable to allocate memory for variables";
    goto Return;
  }

  /*Per la lettura dei numeri avrei gradito usare il codice di loadFromAdfFile.  Il problema
  è che il linguaggio C non mi dà accesso ad una conversione diretta di stringhe rappresentative di numeri a numeri con carattere decimale diverso da '.'.
  Allora procedo diversamente:
  1) leggo riga per riga in dataRow
  2) leggo i numeri da dataRow.
  */
  iL=-1;
  while(1){
    pDataRow=fgets(pDataRow,MAXDATACHAR,fpIn);
    //Faccio una logica robusta che accetta in fondo al file anche una riga vuota (in Win un carattere '\n', in Mac i caratter '\r\n'):
    if(pDataRow==nullptr)   goto Return;
    if(pDataRow[0]=='\n' || pDataRow[0]=='\r') goto Return;
    if(dataRow[MAXDATACHAR-1]!='\0'){
       retStr="Data Error 1 when reading numerical values";
       goto Return;
    }
    if(decimalSeparator!='.'){
      i=0;
      do{
        c=dataRow[i];
        if(c==',')dataRow[i]='.';
        i++;
      }while(c!='\n');
    }
    iL++;
    pStr=strtok(dataRow,separator);
    if(pStr==nullptr){
      retStr="Data Error 2 when reading numerical values";
      goto Return;
    }
    i=sscanf(pStr,"%f",&y[0][iL]);
    for(i1=1; i1<numOfVariables; i1++){
      pStr=strtok(nullptr,separator);
      i=sscanf(pStr,"%f",&y[i1][iL]);
      if(i!=1){
        numOfPoints=iL;
        if(iL==1){
          retStr="Only a single row of data was successfully read\n"
          "that is considered insufficient to proceed";
        }
        goto Return;
      }
    }
  }
  Return:
  /* I punti che ho effettivamente letto sono iL+1. Come si è detto sopra
  il numOfPoints finora calcolato può essere anche di 1 superiore ad esso in quanto nel conteggio iniziale delle righe ho dato la possibilità di avere o meno in fondo ai dati una riga vuota (cioè costituita del solo '\n').
  */
//  numOfPoints=iL+1;
  fileType=LVM;
  variableStep=!autoStep;
  delete [] sVars;
  sVars=new SVar[numOfVariables];
  for(int i1=0; i1<numOfVariables; i1++){
      sVars[i1].unit=giveAutoUnits(varNames[i1][0]);
  }

  //chiusura file e uscita:
  fclose (fpIn);
  return retStr;
}

QString CSimOut::loadFromMatFile(QString fileName, bool allVars_, bool addAlias_) {
    /* Questa routine guarda il file mat e seleziona la routine di gestione.
    */
    char bytes[4];
//    int type;
    FILE * pFile;
    QFileInfo fi(fileName);
    QString ret="";

    allowsPl4Out=false;
    runType=rtUndefined;
    //Apertura file:
    pFile=fopen(qPrintable(fileName),"rb");
//    fread(&type,1,1,pFile);
    if(pFile==nullptr)
        return "Unable to open file in \"loadFromMatFile\"";
    if(fread(bytes,4,1,pFile)!=1){
        fclose (pFile);
        return "Unable to read file in \"LoadFromMatFile\"";
    }
    fileInfo=fi;
    fclose (pFile);
    if(bytes[0]==0 || bytes[1]==0 || bytes[2]==0 || bytes[3]==0){
     // si individua il formato 4 secondo quando scritto nel testo evidenziato in fondo a pag 1-6 del file "matfile_format.pdf"
        ret=loadFromMatFile4(fileName,allVars_, addAlias_);
       if(fileType!=MAT_Modelica)
          fileType=MAT_V4;
    }else{
      fileType=MAT;
      if(useMatLib)
        ret=loadFromMatFileLib(fileName);
      else
        ret=loadFromMatFile5(fileName);
    }

    if(fileType!=MAT_Modelica){
      delete [] sVars;
      sVars=new SVar[numOfVariables];
      for(int i1=0; i1<numOfVariables; i1++){
        sVars[i1].unit=giveAutoUnits(varNames[i1][0]);
      }
    }
  return ret;
}



QString CSimOut::loadFromMatFile4(QString fileName, bool allVars_,bool addAlias_) {
  /*  Routine per leggere i files formato matlab 4.
Il file contiene sequenzialmente coppie header-matrice.
  * Ogni header ha dimensione 20 bytes.
  * Nei files qui accettati prima della scrittura le variabili in Matlab devono
  * essere messe in colonne.
  * In tal modo se ho una matrice, ad ogni colonna della matrice verrà attribuito
  * un nome unico.
  * Una conseguenza naturale di questo approccio è che non possono essere presenti
  * matrici composte da righe come variabili.
  *
  *
  * Essendo i numeri tutti in floating point, essi sono memorizzati su 8 bytes.
  * La struttura variabile della dimensione degli array rende opportuno non allocare
  * le variabili subito, ma dopo una prima passata in cui si fa l'analisi del file.
  */

  char * pVarName=nullptr;
  bool isDouble=true;
  int matrixIndex, var, point, iCol, tVarIndex=-1, varIndex;
  float aux;
  double dAux;
  FILE * pFile;
  struct Header {
    int type,nRows,nCols,imagf,namlen;
  } header;
  QString ret="";
  //Apertura file:
  pFile=fopen(qPrintable(fileName),"rb");
  if(pFile==nullptr)
    return "Unable to open file";
  //Faccio una prima passata per diagnostica e per leggere il numero di variabili:
  matrixIndex=0;
  numOfPoints=-1;
  numOfVariables=0;
  while (1) {
    //Proseguo fino alla fine del file aumentando ad ogni variabile letta numOfVariables
    if(fread(&header,sizeof(header),1,pFile)!=1)
        break;
    if(header.imagf!=0){
      ret= "file format appears to contain complex numbers, which is not allowed";
      goto Return;
    }

    // Le nostre variabili serivano da arrays in Matlab memorizzate per colonne, e quindi devo avere più righe, in quanto ad esse corrispondono più punti
    if(header.nRows==1){
      ret= "Input matlab file contains variables having only one value.\n"
           "Check whether they were stored as rows (incorrect)\n"
           "or columns (correct)";
       goto Return;
    }

    if(header.type==0)
      isDouble=true;
    else
      isDouble=false;

    delete [] pVarName;
    pVarName=new char[header.namlen];
    fread(pVarName,size_t(header.namlen),1,pFile);

    //Se la prima variabile ha come nome "Aclass" si ipotizza che si tratti di un file creato con Modelica (Dymola e OM sano formati quasi identici e entrambi mettonon una prima variabile con questo nome a inizio file). Di conseguenza rimando l'interpretazione alla specifica routine.

    if(strcmp(pVarName,"Aclass")==0){
        fileType=MAT_Modelica;
        rewind(pFile);
        if(allVars_)
          ret=loadFromModelicaMatFile(pFile);
        else
          ret=loadFromModelicaMatFile(pFile,addAlias_);
        fclose (pFile);
        return ret;
    }

    //Se c'è una variabile di nome "t", composta da un'unica colonna, essa verrà intesa
    //contenere il tempo; essa verrà pertanto spostata alla prima posizione nella matrice
    //di memorizzazione interna.
    if(strcmp(pVarName,"t")==0 && header.nCols==1)
       tVarIndex=numOfVariables;
    if(header.nRows==0){
      ret="File contains variables having a null number of rows";
      goto Return;
    }
    if(isDouble){
      for(point=0;point<header.nRows*header.nCols;point++)
        if(fread(&dAux,sizeof(double),1,pFile)!=1)break;
    }else{
      for(point=0;point<header.nRows*header.nCols;point++)
        if(fread(&aux,sizeof(float),1,pFile)!=1)break;
    }
    if(matrixIndex>0 && numOfPoints!=header.nRows){
      ret="The file contains variables having different numbers of rows";
      goto Return;
    }
    numOfPoints=header.nRows;
    numOfVariables+=header.nCols;
    matrixIndex++;
  }

    //Ora alloco spazio per le matrici dei dati ed effettuo la lettura effettiva:
    delete[] varNames;
    varNames=new QString[numOfVariables];
    if(y!=nullptr)
        DeleteFMatrix(y);
    y=CreateFMatrix(numOfVariables,numOfPoints);
    if(y==nullptr)
        return "Unable to allocate space for data storage";

    rewind(pFile);
    iCol=0;
    for(var=0; var<numOfVariables; var++){
        //Il seguente if serve per riordinare le variabili in modo che la variabile "t",
        //se presente, venga messa al primo posto:
        if(tVarIndex==-1){
            varIndex=var;
        }else{
            if(var<tVarIndex)
                varIndex=var+1;
            else if (var==tVarIndex)
                varIndex=0;
            else
                varIndex=var;
        }

        if(iCol==0) {
            fread(&header,sizeof(header),1,pFile);
            delete[]pVarName;
            pVarName=new char[header.namlen];
            fread(pVarName,size_t(header.namlen),1,pFile);
            varNames[varIndex]=QString(pVarName);
        }
        if(header.nCols>1) {
            iCol++;
            varNames[varIndex]=QString(pVarName)+"("+QString(iCol)+")";
            if(iCol==header.nCols)iCol=0;
        }
        if(isDouble){
            for(point=0; point<numOfPoints; point++){
                fread(&dAux,sizeof(double),1,pFile);
                y[varIndex][point]=float(dAux);
            }
        }else{
            for(point=0; point<numOfPoints; point++)
                fread(&y[varIndex][point],sizeof(float),1,pFile);
        }
    }

    Return:
    fclose (pFile);
    return ret;
}

QString CSimOut::loadFromMatFile5(QString fileName) {
  /* Questa funzione è pensata per la lettura in Matlab di files aventi struttura successiva
   * al formato V4.
   * La struttura dei files Matlab nelle versioni più recenti è molto complessa e al
   * momento ne sono state implementate solo alcune funzioni.
   * In particolare manca del tutto la decompressione dei dati, mentre nelle versioni
   * Matlab a partire dalla 7, la compressione è effettuarata per default.
   * Al momento quindi questa funzione non è da attivarsi come default di PlotXY.
   * Come default si attiva la funzione LoadFromMatFileLib() che consente, nei sistemi
   * in cui è presente Matlab, la lettura da qualsiasi formato; nei sistemi che non hanno
   * matlab, la lettura di files mat è meno rilevante, e comunque può sempre essere fatta
   * con l'opzione -V4.
   * Quando sarà implementata anche la compressione risulterà possibile liberarsi
   * completamente dalle librerie Matlab per la lettura delle variabii.

*/
    bool smallData;
    char c, fileHeader[129];
    char arrayFlags[4], currVarName[17];
    int iVar, dataType, numberOfBytes, numOfPoints1, undefined, numDimens, dimens[2];
    int  num, type;
    union {struct {short type,bytes;}small;int type;} type0;
    FILE * pFile;
    enum {int8=1, uInt8, int16, uint16, int32, uint32, singleFloat, doubleFloat=9,
          int64=12, uint64};

        QString retStr="";
    //Apertura file:

    pFile=fopen(qPrintable(fileName),"rb");
    if(pFile==nullptr)
        return "Unable to open file";
    fileHeader[127]=0;
    fread(fileHeader,128,1,pFile);
    // Non faccio la verifica big-endian / little-endian perché con i processori Intel, di mio interesse,
    // è usato sempre il little-endian. Quando dovessi implementare anche questa funzione potrei usare le Endian Conversion functions del Qt.
    //Qui come prima cosa faccio il conto delle variabili e del numero di punti per ognuna di esse. Siccome accetto anche che vi siano matrici con più colonne da considerare variabili indipendenti, non posso desumere il numero di variabili dalle dimensioni del file e della prima variabile incontrata, ma le devo contare tutte, facendo uso dell'accesso diretto tramite fseek() e l'informazione sempre disponibile dei bytes occupati da ogni singola variabile.
    // DA FARE!!!
    numOfVariables=0;
    while(1){
        if(fread(&dataType,4,1,pFile)<1)
            break;  // tipo dell'intero segmento
        fread(&numberOfBytes,4,1,pFile);  //bytes dell'intero segmento
        numOfVariables++;
        fseek(pFile, long(numberOfBytes),SEEK_CUR);
    }
    rewind(pFile);
    fread(fileHeader,128,1,pFile);
    delete[] varNames;
    varNames=new QString[numOfVariables];

    for(iVar=0;iVar<numOfVariables; iVar++){
        smallData=false;
        fread(&dataType,4,1,pFile);  // tipo dell'intero segmento
        if(dataType==15){
        retStr="The file contains compressed variables\n"
                "that is a feature currently not supported.\n"
                "Please save matlab data using the \"-V4\" option.";
        goto Return;
        }
        fread(&numberOfBytes,4,1,pFile);  //bytes dell'intero segmento

/* Ora ci sono i sottosegmenti. Per ognuno di essi i primi 4 bytes ne dicono il formato, il secondi il numero di bytes.*/
        //Primo sottosegmento: Array flags
        fread(&type,4,1,pFile); //tipo dei dati dell'Array flags (in realtà meglio leggerli come caratteri)
        fread(&num,4,1,pFile); //numero di bytes della prossima riga (sempre 8!) Il dato è riportato perché ogni sottosegmento comincia con il tipo di dato e poi ha il numero di bytes seguenti l'intestazione.
        fread(arrayFlags,4,1,pFile); //Array dei flags. L'elemento 0 ha la classe, l'1 i flag (scambiati rispetto alla figura di pag. 1-20 del file pdf perché abbiamo il little-endian
        fread(&undefined,4,1,pFile);

        //Secondo sottosegmento:  il dimensions array:
        fread(&type,4,1,pFile); //tipo dati array dimensions (INT32)
        fread(&num,4,1,pFile); //numero di bytes dell'array dimensions (al netto dell'intestazione)
        numDimens=num/4; //ci saranno 4 bytes per ognuna delle dimensioni
        if(numDimens*4!=num)  //se non sono allineato agli 8 bytes c'è il padding:
            fread(&undefined,4,1,pFile);
        if(numDimens>2){
            retStr="The file contains arrays with more than two dimensions\n"
                    "that is not allowed";
            goto Return;
        }
        for(int i=0; i<numDimens; i++)
            fread(dimens+i,4,1,pFile);

        //Dopo il Dimensions Array c'è l'Array Name:
        fread(&type0,4,1,pFile); //tipo del prossimo dato numerico (è sempre miINT8, quindi sovrapponibile con lettura per caratteri senza problemi di swap di bytes fra little e big endian.)
        //Tratto il caso di Small Data Element

        if(type0.small.bytes>0)smallData=true;
        if(smallData){
            type=type0.small.type;
            num=type0.small.bytes;
        }else{
            type=type0.type;
            fread(&num,4,1,pFile); //numero di caratteri per il nome dell'array
        }
        //Il nome in PlotXY lo tronco a 16 caratteri; pertanto leggo o 8 o 16 bytes (la scrittura viene fatta in campi di 8 bytes con scarto dei residui):
        if(smallData){ //in tal caso ho uno small data element format
            fread(currVarName,4,1,pFile); //numero di caratteri per il nome dell'array
        }else if(num<9){
            fread(currVarName,8,1,pFile); //numero di caratteri per il nome dell'array
        }else{
            fread(currVarName,16,1,pFile); //numero di caratteri per il nome dell'array
        }
        //leggo e scarto eventuali caratteri residui:
        if(num>16 && num%8!=0)
            for(int i=0; i<8-num%8; i++)
                fread(&c,1,1,pFile);
        if(num<17)
            currVarName[num]='\0';
        else
            currVarName[16]='\0';
        varNames[iVar]=QString(currVarName);

        //Ora il subsegmento con la parte numerica:
        fread(&type0,4,1,pFile); //formato  del prossimo dato numerico
        // Anche qui può accadere che vi sia uno smalldata, se ho un unico valore.
        //Invece di fare la lettura completa emetto un essaggio di errore perché un unico valore non è ammesso in PlotXY:
        if(type0.small.bytes>0){
            retStr="The file contains a variable with one value, that is not allowed";
            goto Return;
        }
        type=type0.small.type;
        /* E' stato verificato che se la variabile cotiene tutti numeri indicati come double nel works space, ma mappabili come interi (es. 1.0, 2.0, 3.0 ecc) nel fie essi vengono riportati con type 2 anziché 9! (file falseint.mat)*/
        if(type!=uInt8 && type!=singleFloat && type!= doubleFloat){
            retStr="Invalid format:\nonly single and double precision variables\nare allowed in mat files to be read";
            goto Return;
        }
        fread(&num,4,1,pFile); //numero di bytes per i dati
        if(type==uInt8)
            numOfPoints1=num;
        else if(type== singleFloat)
            numOfPoints1=num/8;
        else
            numOfPoints1=num/8;
        if(iVar==0)
            numOfPoints=numOfPoints1;
        else if(numOfPoints1!=numOfPoints){
            retStr="Error: the number of Points all variables must be the same\n"
               "while variable "+ varNames[iVar-1] + " has " + QString::number(numOfPoints)+ " points\n"
               "and variable "  + varNames[iVar]   + " has " + QString::number(numOfPoints1)+" points.";
            goto Return;
        }
        //L'allocazione della matrice si fa in occasione della lettura delle prima variabile:
        //Ora alloco spazio per la matrice dei dati ed effettuo la lettura:
        if(iVar==0){
            if(y!=nullptr)
                DeleteFMatrix(y);
            y=CreateFMatrix(numOfVariables,numOfPoints);
            if(y==nullptr){
                retStr="Unable to allocate space for data storage";
                goto Return;
            }
        }
        if(type==uInt8){
            short s;
            for(int i=0; i<numOfPoints; i++){
                fread(&s,1,1,pFile);
//                float f=(float)s;
                y[iVar][i]=float(s);
            }
        }else  if(type==doubleFloat){
            double D;
            for(int i=0; i<numOfPoints; i++){
                fread(&D,8,1,pFile);
                y[iVar][i]=float(D);
            }
        }else{
            for(int i=0; i<numOfPoints; i++)
                fread(&y[iVar][i],4,1,pFile);
        }
    }
    Return:
    fclose (pFile);
    return retStr;
}

//----------------------------------------------------------------------
QString CSimOut::loadFromMatFileLib(QString fileName) {
    /* Routine per effettuare la lettura di matrici MATLAB.
   Questa versione demanda l'effettiva lettura a funzioni di libreria fornite da
   MATLAB, che garantiscono un'ottima compatibilità con versioni del file fino a
   quella associata alla libreria utilizzata.
   La libreria qui utilizzata è quella fornita con Matlab 7.0.
   Sembrerebbe non possibile usare Matlab R2008 in quanto non fornisce versioni
   delle librerie lib dichiaratamente compatibili con Borland.

   La routine può essere usata per leggere files Matlab creati da PlotXY
   (sia versione 4 che 7) o altri software MC's che si appoggiano alle funzioni
   del presente file (es. pl42mat, Converter).
   Può anche essere utilizzata per leggere files Matlab creati in altro modo, purchÈ
   contangano variabili matriciali, ad una o più colonne, tutte caratterizzate dallo stesso
   numero di righe. Pensa la routines a generare nomi per ognuna delle colonne delle
   variabili a più colonne.
   */
  bool done;
  QString currVarName, Ret="", Txt, Txt1;
  const char *name;
  const int *maxNumbersPerDim;
  int numOfMatVars, numOfDims, iCol, iPoint,
      iMatVar, iVar; //Indici della variabile mat e PlotXY. Quello della mat corre
                     //fra le varie matrici del file, iVar fra tutti i vettori PlotXY
                     //che sono in numero pari al numero di colonne globalmente
                     //presenti nel file
  float * fFileVar;
  double * dFileVar;
  MATFile *mfp;
  mxArray *mxVar = nullptr;
  typedef char ** (*prMatIntTOChar)(MATFile *, int *);
  typedef int (*prMatTOInt)(MATFile *);
  typedef MATFile *(*prChChTOMat)(const char *, const char *);
  typedef mxArray *(*prMatChTOMxArr)(MATFile *, const char **);
  typedef int *(*prMxArrTOInt)(const mxArray *);
  typedef int  (*prMxArrTOInt1)(const mxArray *);
  typedef mxClassID (*prMxArrTOMxId)(const mxArray *);
  typedef double * (*prMxArrTODouble)(const mxArray *);
  typedef void (*prVoidTOVoid)(void *);

  static prMatIntTOChar matGetDir;
  static prMatTOInt matClose;
  static prChChTOMat matOpen;
  static prMatChTOMxArr matGetNextVariable;
  static prMxArrTOInt mxGetDimensions;
  static prMxArrTOInt1 mxGetNumberOfDimensions;
  static prMxArrTOMxId mxGetClassID;
  static prMxArrTODouble mxGetPr;
  static prVoidTOVoid mxFree;

  QLibrary libMat("libMat"), libMx("libMx");
  done=libMat.load();
  if(done)
      done=libMx.load();
  if(!done){
    return "Unable to load \"libmat.dll\".\n\n"
           "This is commonly due to absence of a valid Matlab installation in the computer\n"
           "Please use \".mat\" file format 4.0, that does not require libmat.dll.";
  }else


 //Carico le procedure:
  matGetDir=prMatIntTOChar(libMat.resolve("matGetDir"));
  matClose=prMatTOInt(libMat.resolve("matClose"));
  matOpen =prChChTOMat(libMat.resolve("matOpen"));
  matGetNextVariable=prMatChTOMxArr(libMat.resolve("matGetNextVariable"));
  mxGetDimensions=prMxArrTOInt(libMx.resolve("mxGetDimensions"));
  mxGetNumberOfDimensions=prMxArrTOInt1(libMx.resolve("mxGetNumberOfDimensions"));
  mxGetClassID=prMxArrTOMxId(libMx.resolve("mxGetClassID"));
  mxGetPr     =prMxArrTODouble(libMx.resolve("mxGetPr"));
  mxFree      =prVoidTOVoid(libMx.resolve("mxFree"));

  if(matGetDir==nullptr || matClose==nullptr || matOpen==nullptr      ||
     matGetNextVariable==nullptr      || mxGetDimensions==nullptr  ||
     mxGetNumberOfDimensions==nullptr ||  mxGetClassID==nullptr    ||
     mxGetPr==nullptr                 ||  mxFree==nullptr                   )
      return "Error loading functions in \"libmat.dll\" or \"libMx.dll\".\n"
     "Please save  \".mat\" file using the \"-v4\" option:\n"
     "this will not require any external library.";

  //Apertura file:
  mfp=matOpen(qPrintable(fileName),"r");
  if(mfp==nullptr)
      return "Unable to open file";

  /*Determinazione numero e dimensione variabili e allocazione spazio: */
  matGetDir(mfp,&numOfMatVars);
  /* Essendo possibile importare in PlotXY anche files contenenti matrici a più colonne,
  evidentemente il numOfVariables di PlotXY potrà essere superiore a numOfMatVars.
  Ne effettuo quindi il calcolo:
  */
  matClose(mfp);
  mfp=matOpen(qPrintable(fileName),"r");
  numOfVariables=0;
  for(iVar=0; iVar<numOfMatVars; iVar++){
    mxVar = matGetNextVariable(mfp, &name);
    if(mxVar==nullptr)
        return "Unexpected error 1 while reading mat file variable";
    maxNumbersPerDim=mxGetDimensions(mxVar);
    numOfVariables+=maxNumbersPerDim[1];
    // The following assignment should better be placed immediately below the for loop. It is kept here to avoid a warning message "maxNumbersPerDim may be used unitialized"
    if(iVar==0)
      numOfPoints=maxNumbersPerDim[0];
  }
  delete[] varNames;
  varNames=new QString[numOfVariables];
  if(y!=nullptr)
      DeleteFMatrix(y);
  y=CreateFMatrix(numOfVariables,numOfPoints);
  if(y==nullptr)
      return "Unable to allocate space for data storage";
  /*Ora, nel seguente loop effettuo l'acquisizione completa: */
  matClose(mfp);
  mfp=matOpen(qPrintable(fileName),"r");
  iVar=-1;
  for(iMatVar=0;iMatVar<numOfMatVars;iMatVar++) {
    mxVar = matGetNextVariable(mfp, &name);
    if(mxVar==nullptr)
        return "Unexpected error 2 reading mat file variable";
    numOfDims=mxGetNumberOfDimensions(mxVar);
    if(numOfDims!=2)
      return "Invalid array in matlab file\n All arrays must be bi-dimensional!";
    maxNumbersPerDim=mxGetDimensions(mxVar);
    if(maxNumbersPerDim[0]!=numOfPoints){
      Txt=QString::number(maxNumbersPerDim[0]);
      Txt1=QString::number(numOfPoints);
      return "Invalid array in matlab file;\n"
         "array "+currVarName+" contains "+Txt+" points, while previous arrays contain "+Txt1+ " points.\n"
         "All arrays must share the same number of points";
    }
    if(mxGetClassID(mxVar)==mxDOUBLE_CLASS){
      dFileVar=reinterpret_cast<double *>(mxGetPr(mxVar));
//      dFileVar=(double *)mxGetPr(mxVar);
      for (iCol=0; iCol<maxNumbersPerDim[1]; iCol++){
        iVar++;
        varNames[iVar]=QString(name);
        if(maxNumbersPerDim[1]>1)varNames[iVar]=varNames[iVar]+"("+QString::number(iCol+1)+")";
          for (iPoint=0; iPoint<numOfPoints; iPoint++)
              y[iVar][iPoint]=float(dFileVar[iPoint+iCol*numOfPoints]);
      }
    }else if(mxGetClassID(mxVar)==mxSINGLE_CLASS){
      fFileVar=reinterpret_cast<float *>(mxGetPr(mxVar));
//      fFileVar=(float *)mxGetPr(mxVar);
      for (iCol=0; iCol<maxNumbersPerDim[1]; iCol++){
        iVar++;
        varNames[iVar]=QString(name);
        if(maxNumbersPerDim[1]>1)varNames[iVar]=varNames[iVar]+"("+QString::number(iCol+1)+")";
        for (iPoint=0; iPoint<numOfPoints; iPoint++)
          y[iVar][iPoint]=float(fFileVar[iPoint+iCol*numOfPoints]);
      }
    }else{
        return "Invalid array in matlab file;\n";
    }
  }

    /* Qui va ancora aggiunto il riordino delle colonne per far comparire la colonna
        con il tempo al primo posto. Il codice sarà analogo a quello usato per la versione
        4 di Matlab
    */

    matClose(mfp);
    mxFree(mxVar);
    return Ret;
}



QString CSimOut::loadFromModelicaMatFile(FILE * pFile){
  /* Questa è una funzione specializzata per la lettura dei files matlab creati da
   * Dymola e OM. Essi hanno la stessa struttura, riesco a gestirli da un'unica
   * funzione.
   * Per comprendere come funziona questa funzione la cosa più semplice è guardare il file
   * RL_DYM.TXT,, associandolo, durante la lettura a RL_DYM.mat.
   * Infatti la descrizione del file TXT di Dymola è molto completa e coincide con la
   * logica usata per scrivere la presente funzione.
   *
   * DIFFERENTI VERSIONI DELLA FUNZIONE
   * v. commento in loadFromModelicaMatFile(FILE * pFile, bool addAlias_
   *
*/
    QString ret;
    struct DataFromModelicaFile fd;
    QList <SVar> sVarsLst;

    //demando a una routine specializzata la lettura delle prime parti del file:
    fd=inputMatModelicaData(pFile);

   numOfVariables=fd.numOfAllVars;
   numOfPoints=fd.numOfData2Rows;
   if(y!=nullptr)
       DeleteFMatrix(y);
   y=CreateFMatrix(numOfVariables,numOfPoints);
   if(y==nullptr){
       ret= "unable to allocate y matrix";
       return "unable to allocate y matrix";
   }
   delete[] varNames;
   varNames=new QString[numOfVariables];
   delete [] sVars;
   sVars=new SVar[numOfVariables];

   int varIndex=0;
   paramInfo.names.clear();
   paramInfo.description.clear();
   paramInfo.units.clear();
   paramInfo.values.clear();
   paramInfo.code.clear();
   for (int i=0; i<fd.dataInfoRows; i++){
   /* Il numero di dataInfoRows è più grande di varIndex, in quanto il primo contiene
    * anche righe per i parametri
   */
     if (fd.dataInfo[i][0]==1){
       paramInfo.names.append(fd.namesLst[i]);
       paramInfo.description.append(fd.descriptionLst[i]);
       paramInfo.units.append(fd.unitLst[i]);
//       int dataInfoIdx=fd.dataInfo[i][1]-1;
       if(fd.dataInfo[i][1]>0)
          paramInfo.values.append(fd.data_1[0][fd.dataInfo[i][1]-1]);
       else
          paramInfo.values.append(-fd.data_1[0][-fd.dataInfo[i][1]-1]);
       continue;
     }else if (fd.dataInfo[i][1]>0){
       for (int j=0; j<numOfPoints; j++)
         y[varIndex][j]=fd.data_2[j][fd.dataInfo[i][1]-1];
      }else{
        for (int j=0; j<numOfPoints; j++)
          y[varIndex][j]=-fd.data_2[j][-fd.dataInfo[i][1]-1];
     }

     // Per favorire l'identificazione corretta dell'unità di misura sull'asse x, l'iniziale del tempo la metto minuscola:
     if(varNames[0]=="Time")
         varNames[0]="time";
     SVar sVar;
     sVar.name=fd.namesLst[i];
     sVar.infoIndex=i;
     sVar.description=fd.descriptionLst[i];
     sVar.unit=fd.unitLst[i];
     sVars[varIndex]=sVar;
     sVarsLst.append(sVar);
     varNames[varIndex]=fd.namesLst[i];
     varIndex++;
   }

   DeleteIMatrix(fd.dataInfo);
   DeleteFMatrix(fd.data_1);
   DeleteFMatrix(fd.data_2);
   return ret;
}


QString CSimOut::loadFromModelicaMatFile(FILE * pFile, bool addAlias_){
  /* Questa è una funzione specializzata per la lettura dei files matlab creati da
   * Dymola e OM. Essi hanno la stessa struttura, riesco a gestirli da un'unica
   * funzione.
   *
   * DIFFERENTI VERSIONI DELLA FUNZIONE
   * La versione con un solo argomento carica in memoria la lista completa delle variabili
   * anche quelle che sono alias di altre. L'utente potrà scegliere ad esempio resistor.i
   * o resistor.p.i.
   * La versione contenente due parametri invece crea una lista solo delle variabili
   * diverse le une dalle eltre, omettendo di produrre quelle che sono uguali ad altre
   * o uguali all'opposto di altre (definite ALIAS) che in effetti non sono presenti
   * nel file mat per ridurne, giustamente, le dimensioni. In quest'ultimo caso è
   * possibile chiedere che venga visualizzato nell'hint di varMenuTable anche la lista
   * delle alias o meno. Probabilmente si richiedereà sempre addAlias_=true.
   *
   * Quando entro qui il file è già aperto e riavvolto. La struttura che mi aspetto è
   * descritta in tutti i file TXT creati da dymola come output di simulazione, ed in
   * particolare in Bouncing0.txt presente nella cartella data del progetto Mat4ToTxt
   *

   *
*/
    int varIndex=0;
    QString ret;
    QList <int> loadedVarIndices;

    struct DataFromModelicaFile fd;
    char * pVarName=nullptr;

    //demando a una routine specalizzata la lettura delle prime parti del file:
    fd=inputMatModelicaData(pFile);
    numOfVariables=fd.numOfData2Cols;
    numOfPoints=fd.numOfData2Rows;


   //Ora ho inserito tutto tranne i valori. La matrice data_1 contiene i parametri, la data_2 i valori numerici "compatti". Infatti l'output di modelica contiene molte variabili eliminabili in quanto una uguale all'altra o all'opposto dell'altra. Ad esempio in un resistore vi sono tre correnti di cui due sono identiche e la terza è l'opposto della prima.
   // Per ora faccio la scelta di mettere fra le variabili di SO soltanto quelle non ridondate. Successivamente si può prevedere di aggiungere un array aggiuntivo per i parametri e un'opzione per non eliminare le variabili ridondanti


   if(y!=nullptr)
       DeleteFMatrix(y);
   y=CreateFMatrix(numOfVariables,numOfPoints);
   if(y==nullptr){
       return "Unable to allocate space for data storage";
   }
   delete[] varNames;
   varNames=new QString[numOfVariables];
   delete [] sVars;
   sVars=new SVar[numOfVariables];


   for (int i=0; i<fd.dataInfoRows; i++){
     if (fd.dataInfo[i][0]==1){
       continue;  //salto i parametri
     }else if (fd.dataInfo[i][0]==2 && fd.dataInfo[i][1]<0){
         continue;  //salto le variabili che sono l'opposto di altre
     }
     if(loadedVarIndices.contains(fd.dataInfo[i][1]-1))
        continue;  //salto variabili che sono copia di altre
     varIndex=fd.dataInfo[i][1]-1;
     if(varIndex>=numOfVariables){
       QMessageBox::critical(nullptr,"loadFromModelicaMatFile","critical error 1");
       return "";
     }
     for (int j=0; j<numOfPoints; j++){
       y[varIndex][j]=fd.data_2[j][varIndex];
     }
     loadedVarIndices<<varIndex;
     varNames[varIndex]=fd.namesLst[i];

   /* A partire dal 12/10/17 decido di utilizzare informazioni più articolate dei files
    * di dati, sulla scorta della riccchezza delle informazioni disponibili negli
    * output di Modelica. Pertanto aggiungo a varNames, la lista sVars. In un futuro
    * più o meno lontano l'array varNames dovrebbe scomparire.
    *
    * In questa versione della funzione a due argomenti description contiene, se addAlias
    *  = true,  oltre alla descrizione della variabile caricata, anche la lista delle
    * variabili non presenti nel file, e non caricate in PlotXY in "ALIAS".
  */

     // Per favorire l'identificazione corretta dell'unità di misura sull'asse x, l'iniziale del tempo la metto minuscola:
     if(varNames[0]=="Time")
         varNames[0]="time";
     SVar sVar;
     sVar.name=varNames[varIndex];
     sVar.infoIndex=i;
     sVar.description=fd.descriptionLst[i];
     sVar.unit=fd.unitLst[i];
     sVars[varIndex]=sVar;
   }

   //Fin qui la description è solo quella della variabile direttamente interessata. Se richiesto addAlias_ passo in sVar una descrizione più ricca, contenente anche gli alias
  //Prima passata: aggiungo la parola "ALIASES":
   if(addAlias_){
     //Per ogni variabile in sVars vedo se vi sono righe di alias da aggiungere
     for(int varIdx=0; varIdx<numOfVariables; varIdx++){
       for (int i=1; i<fd.dataInfoRows; i++){
         if(fd.dataInfo[i][0]==2 && fd.dataInfo[i][1]==varIdx+1 && i!=sVars[varIdx].infoIndex){
           sVars[varIdx].description+="\n*** ALIASES ***\n";
           break;
         }
       }
     }
   }
   //A questo punto le variabili che hanno alias avranno in fondo alla description già la dicitura "ALIASES:". Aggiungo le description del caso
   if(addAlias_){
     //Per ogni variabile in sVars vedo se vi sono righe di alias da aggiungere
     for(int varIdx=0; varIdx<numOfVariables; varIdx++){
       for (int i=1; i<fd.dataInfoRows; i++){
         if(fd.dataInfo[i][0]==2 && qAbs(fd.dataInfo[i][1])==varIdx+1 && i!=sVars[varIdx].infoIndex){
           if(fd.dataInfo[i][1]<0)
             sVars[varIdx].description+="-";
           sVars[varIdx].description+= fd.namesLst[i];
           sVars[varIdx].description+= " - ";
           int unitIdx=fd.descriptionLst[i].lastIndexOf('[');
           sVars[varIdx].description+= fd.descriptionLst[i].mid(0,unitIdx);
           sVars[varIdx].description+= "\n";
         }
       }
     }
   }

   //Prima di uscire creo una struttura stabile con informazioni sui parametri interrogabile dall'esterno tramite la funzione getParamInfo(). Devo escludere la prima riga che non è un vero parametro ma è il tempo. Inoltre diverse righe di dataInfo possono puntare allo stesso valore di data_1
   //Anche per i Parametri possono esistere degli alias. Uso la stessa loadedVarIndicesInfo usata per le variabili
   loadedVarIndices.clear();
   int numOfParams=0;
   paramInfo.names.clear();
   paramInfo.description.clear();
   paramInfo.units.clear();
   paramInfo.values.clear();
   int aliasCount=0;
   for (int i=1;  i<fd.dataInfoRows; i++){
     if(fd.dataInfo[i][0]==1){
       int parIdx=qAbs(fd.dataInfo[i][1])-1;
       int lastIndexOf=loadedVarIndices.lastIndexOf(parIdx);
       if(lastIndexOf>-1 && addAlias_){  //Caso in cui un parametro è stato memorizzato e gli devo aggiungere uno o più alias
         QString curDescription=paramInfo.description[lastIndexOf];
         QString curName=paramInfo.names[lastIndexOf];
         int lastCharIdx=curDescription.count()-1;
         //la seguente riga è importante perché se la riga è vuota lastCarIdx è -1
         if(lastCharIdx<0)
             continue;

         /* Come per le variabili il testo descrittivo di una variabile contenente alias
          * è così composto:
          * - nome
          * - descrizione con unità di misura
          * - la stringa "*** ALIASES ***"
          * - descrizioni degli alias con l'eventuale segno =-= ma senza l'unità di misura
         */

         if(paramInfo.code [lastIndexOf]==1){
           paramInfo.description[lastIndexOf]+="\n*** ALIASES ***\n";
           paramInfo.code[lastIndexOf]=2;
         }
         if(fd.dataInfo[i][1]<0)
           paramInfo.description[lastIndexOf]+="-";
         paramInfo.description[lastIndexOf]+= fd.namesLst[i];
         paramInfo.description[lastIndexOf]+= " - ";
         //Elimino l'unità di misura dalle description sotto la scritta *** ALIASES *** in quanto gli alias condividono l'unità della variabile base:
         int unitIdx=fd.descriptionLst[i].lastIndexOf('[');
         paramInfo.description[lastIndexOf]+= fd.descriptionLst[i].mid(0,unitIdx);
         paramInfo.description[lastIndexOf]+= "\n";
         aliasCount++;
         continue;
       }else { // caso di prima memorizzazione di un parametro
         loadedVarIndices<<fd.dataInfo[i][1]-1;
         paramInfo.code.append(1);
         paramInfo.description.append(fd.descriptionLst[i]);
         paramInfo.names.append(fd.namesLst[i]);
         paramInfo.units.append(fd.unitLst[i]);
         int varIndexToRead=fd.dataInfo[i][1]-1;
         paramInfo.values.append(fd.data_1[0][qAbs(varIndexToRead)]);
         numOfParams++;
       }
     }
   }

   /*
   Righe commentate che possono essere usate per scrivere su file risultati parziali in caso di problemi.
   FILE * outFile;
   //Apertura file:
   outFile=fopen("output ModelicaMat File.txt","w");
   if(outFile==NULL)return "Unable to open output file";
   //Scrittura dati:
   for(int iVar=0;iVar<numOfVariables;iVar++){
     fprintf(outFile,"%s\t",varNames[iVar].toLatin1().data());
   }
   fprintf(outFile,"\n");
   for(int point=0;point<qMin(10,numOfPoints);point++){
     for(int iVar=0;iVar<numOfVariables;iVar++)
        fprintf(outFile,"%f\t",y[iVar][point]);
     fprintf(outFile,"\n");
   }
   fclose(outFile);
*/

   fileType=MAT_Modelica;
   DeleteIMatrix(fd.dataInfo);
   DeleteFMatrix(fd.data_1);
   DeleteFMatrix(fd.data_2);
//   delete[]allNames;
   delete[] pVarName;
   return ret;
}


//----------------------------------------------------------------------
CSimOut::CStrTok::CStrTok(void){
  separ=',';
  string=nullptr;
}
void CSimOut::CStrTok::getStr(char * Str_){
  int i;
  delete[] string;
  lastTok=endStr=false;
  string=strdup(Str_);
  //Calcolo il numero di separatori presenti sulla stringa, per supporto ad evantuale
  //debug fatto sulla stringa da parte del programma utilizzante StrTok:
  commas=0;
  for(i=0; i<int(strlen(string)); i++)   if(string[i]==separ)commas++;
  lastBeg=string;
  lastEnd=strchr(lastBeg,separ);
  if(lastEnd==nullptr) lastEnd=strchr(lastBeg,0);
  *lastEnd=0;
}
char *CSimOut::CStrTok::giveTok(void){
  char *retStr;
  if(endStr)return nullptr;
  if(lastTok) {
    endStr=true;
    return lastBeg;
  }
  retStr=lastBeg;
  lastBeg=lastEnd+1;
  lastEnd=strchr(lastBeg,separ);
  if(lastEnd==nullptr){
    lastTok=true;
    lastEnd=strchr(lastBeg,0);
  }
  *lastEnd=0;
  return retStr;
}

CSimOut::CStrTok::~CStrTok(){
  delete[] string;
}


#ifndef EXCLUDEATPCODE
  #include "CSimOutATP.cpp"
#endif




QString CSimOut::namesAdfToMat(){
  /* Questa routine converte i nomi delle variabili originariamente lette da un file ADF in nomi adatti al Matlab.
  Le operazioni che effettua sono le seguenti:
  1·	if the first character is not a letter, the character 'x' is prefixed
  2·	if some inner character is not a letter, or a digit, it is converted into '_'.
  La routine ritorna 0 se la conversione dei nomi è andata a buon fine, 1 se ci sono stati errori
  */

  char c;
  int ic, var, length;
  for(var=0; var<numOfVariables; var++){
    length=varNames[var].size();
    //Operazione 1:
    if(!isalpha(varNames[var][0].toLatin1()))
      varNames[var]="x"+varNames[var];
    //Operazione 2:
    for(ic=0; ic<length; ic++){
      c=varNames[var][ic].toLatin1();
      if(!isalpha(c) && !isdigit(c)) varNames[var][ic]='_';
    }
  }
  return "";
}

QString CSimOut::namesComtradeToMat(bool mat){
  /* Questa routine converte i nomi delle variabili originariamente lette da un file COMTRADE dallo standard Simout, che è quello utilizzato in PlotXY allo standard Matlab, che è quello utilizzato in Pl42mat.
    Se però il parametro passato Mat è false, vuol dire che l'uscita va su un file ADF e allora ci può essre un maggior rispetto per i nomi originari, in quanto sono ammessi caratteri particolari quali '-', '(', ecc.
    Le modalità di conversione sono chiarite nel documento Conversion.doc
    */
  int i, var, len;
  char *pToken=nullptr, prefix[3];
  char dummy[9];
  QString name;
  //Nel seguente loop parto da 1 perché la variabile 0 è sempre il tempo, di nome "t".
  for(var=1; var<numOfVariables; var++){
    pToken=strdup(varNames[var].toLatin1().data());
    //tratto per prima cosa il caso di nome assente nel file Comtrade:
    len=int(strlen(pToken));
    if(pToken[len-1]==':'){
      strcpy(dummy,pToken);
      strcat(dummy,"Dummy");
      varNames[var]=QString(dummy);
      continue;
    }
    //Elimino il prefisso da pToken e lo metto in Prefix:
    strncpy(prefix,pToken,2);
    if(pToken[1]==':'){
      prefix[1]=0;
      pToken=pToken+2;
    }else
      //Qui siamo nel caso per es. di variabili comincianti per u1:, s1:, cr:, ci: ecc.
      //cioè nel caso in cui prima dei due punti ho due caratteri anziché 1
      pToken=pToken+3;

    strtok(pToken," ,\t");
    //Se ho dei separatori considero il nome come "multiword" e lo tratto di conseguenza.
    //Il primo passo consiste nell'eliminazione degli spazi e mettere tutto lowercase
    //eccetto i primi caratteri di ogni parola:
    if(pToken==nullptr)
        continue;
    varNames[var]="";
    while(pToken!=nullptr){
      name=QString(pToken).toLower();
      name[0]=(name[0].toUpper());
      varNames[var]+=name;
        pToken=strtok(nullptr," ,\t");
    }
    //Il secondo passo consiste nel convertire caratteri non alfanumerici in '_' (solo per files Matlab):
    if(mat){
      len=varNames[var].size();
      for(i=1;i<=len; i++)
        if(!varNames[var][i].isLetterOrNumber())
          varNames[var][i]='_';
    }
    //L'ultimo passo consiste nell'aggiunta del prefisso:
    varNames[var]=QString(prefix)+varNames[var];
  }

  delete [] pToken;
  return "";
}


QString CSimOut::namesPl4ToAdf(int addDigit){
  /* Questa routine converte i nomi delle variabili PlotXY, originariamente letti da file
  Pl4, allo standard Adf, secondo la stessa procedura in uso nel programma Pl4ToMat
  (descritta anche nel file Conversion.doc)
  Inoltre se addDigit è >0 viene appeso al nome il digit corrispondente al valore
  numerico di addDigit.
  Le operazioni da fare, esclusa la variabile tempo, sono le seguenti:
  1 mettere in minuscolo tutti i caratteri
  2 spezzare il nome nei due nomi di nodo (il secondo vuoto in alcuni casi)
  3 per ognuno dei nomi di nodo:
    - eliminare spazi in cima e in fondo
    - convertire ' ' in '_'
    - mettere in maiuscolo il primo carattere
  4 ricostruire il nome completo
  Inoltre su tutte le variabili (compresa la variabile tempo) va eventualmente appeso
  il digit corrispondente all'intero addDigit.

  La routine ritorna 0 se la conversione dei nomi è andata a buon fine, 1 se ci sono stati
  errori
  */

  char c;
  int var,j,iN,
        pPos, //Posizione del ':' all'inizio dei nomi
        length;
  QString node[2]; //nomi del primo e secondo nome componenti file->varNames[var]
  for(var=1; var<numOfVariables; var++){ //Parto da 1 perché esclude il tempo
    length=varNames[var].length();
    if(varNames[var][1]==':')
      pPos=2;
    else if(varNames[var][2]==':')
      pPos=2;
    else if(varNames[var][3]==':')
      pPos=3;
    else if(varNames[var][4]==':')
      pPos=4;
    else{
      puts("Internal error \"pPos\", contact Mr Ceraolo");
      return "-1";
    }
    //Operazione 1:
    varNames[var]=varNames[var].toLower();
    //Operazione 2:
    node[0]=varNames[var].mid(pPos,6);
    if(node[0]=="      ") node[0]="Terra ";
    if(length>8){
      node[1]=varNames[var].mid(pPos+7,6);
      if(node[1]=="      ") node[1]="Terra ";
    }else
      node[1]="";
    //Operazione 3
    for(iN=0; iN<2; iN++){
      if(node[iN]=="")break;
      node[iN]=node[iN].trimmed();
      for(j=0; j<node[iN].length(); j++){
          c=node[iN].toLatin1()[j];
        if(c==' ')
          node[iN][j]='_';
      }
      node[iN][0]=node[iN][0].toUpper();
//       node[iN][1]=(char)toupper(node[iN][1]);
    }

    //Operazione 4:
    if(varNames[var][0]=='c')varNames[var][0]='i';
    if(node[1]=="")
      varNames[var]=varNames[var].mid(0,pPos-1)+node[0];
    else
      varNames[var]=varNames[var].mid(0,pPos-1)+node[0]+node[1];
    //Operazioni finali:
    if(addDigit>0)
      varNames[var]=varNames[var]+QString(addDigit);
  }
  if(addDigit>0)
     varNames[0]=varNames[0]+QString(addDigit);
  return "";
}


//---------------------------------------------------------------------
QString CSimOut::saveToAdfFile(QString fileName, QString comment) {
  int i, *vars;
  QString ret;
  vars = new int[numOfVariables];
  for(i=0; i<numOfVariables; i++)vars[i]=i;
    ret=saveToAdfFile(fileName, comment, numOfVariables, vars);
  delete vars;
  return ret;
}

//---------------------------------------------------------------------
QString CSimOut::saveToAdfFile(QString fileName, QString comment, int nVars, int vars[]) {
  /* questa routine consente di salvare variabili selezionate.
   * Il vettore vars ha gli indici della matrice y da cui fare i grafici.
   * Il primo elemento di vars è sepre 0 perché serve a registrare il tempo.
   * nVars è il numero di elementi di vars, quindi pari al numero di segnali +1.
*/
  int iVar, point;
  FILE * pFile;
  //Apertura file:
  pFile=fopen(fileName.toLatin1().data(),"w");
  if(pFile==nullptr)return "Unable to open file";
  //Scrittura header:
  comment="//"+comment+"\n";
  fprintf(pFile,comment.toLatin1().data());
  if(fileType==PL4_1 ||fileType==PL4_2)
    namesPl4ToAdf(0);
  else if(fileType==COMTRADE)
    namesComtradeToMat(false);
  for(iVar=0;iVar<nVars;iVar++) {
      QString _name=varNames[vars[iVar]];
      QByteArray _nameArr=_name.toLatin1();
    fprintf(pFile,"%s\t",varNames[vars[iVar]].toLatin1().data());
  }
  fprintf(pFile,"\n");

  //Scrittura dati:
    for(point=0;point<numOfPoints;point++){
    for(iVar=0;iVar<nVars;iVar++)
      fprintf(pFile,"%g\t",double(y[vars[iVar]][point]));
    fprintf(pFile,"\n");
  }
  if(fclose (pFile))
    return "Error closing file";
  else
    return "";
}

QString CSimOut::saveToComtradeFile(QString cfgFileName, QString stationName) {
    int i, *vars;
    QString ret;
    vars = new int[numOfVariables];
    for(i=0; i<numOfVariables; i++)vars[i]=i;
      ret=saveToComtradeFile(cfgFileName, stationName, numOfVariables, vars);
    delete vars;
    return ret;
}

QString CSimOut::saveToComtradeFile(QString cfgFileName, QString stationName, int nVars, int vars[]) {
  /* questa routine consente di salvare variabili selezionate.
   * Il vettore vars ha gli indici della matrice y da cui fare i grafici.
   * Il primo elemento di vars è sepre 0 perché serve a registrare il tempo.
   * nVars è il numero di elementi di vars, quindi pari al numero di segnali +1.
  */
  QString CCBM, unit;
  int iVar, point, len;
  qint32 iMin, iVarMax, iTimeMax;
  float *factor, *offset, max;
  FILE *pFile;
  //la seguente variabile row contiene una stringa generica per scriverci sopra una riga da mettere poi nel file:
  QString row;
  QString datFileName, varName;
  QDateTime dateTime;


  /* Nei files comtrade i valori degli istanti di campionamento e delle variabili sono scritti in microsecondi.
 Per simulazioni molto lunghe questo può causare imprecisione nella scrittura.
Pertanto è prevista l'introduzione di appositi fattori:
- il "time multiplication factor" in ultima riga, per il tempo
- i Factor nelle righe delle variabili.
Naturalmente tale valore va calcolato con riferimento all'effettivo intervallo dei tempi e al campo entro cui posso variare gli int nel sistema in uso.
Da prove fatte con GTPPLOT negli anni '90  appariva che interi negativi non venivano gestiti correttamente da tale programma. Pertanto sia per le variabili che per il tempo uso interi positivi, di ampiezza tale da entrare sempre nel numero di digit ammessi dall standard, pari a 10 per il tempo e 6 per le variabili

  */
  iMin=0;
  iVarMax=99999;
  iTimeMax=999999999;  // sono solo 9 caratteri in quanto andare oltre causerebbe l'overflow dai 32 bit dell'intero. Con questo valore, a causa di errori di arrotondamento si può arrivare anche a numeri a 10 cifre, del tipo 1000000010, ma sono numeri comunque ampiamente al di sotto del limite di overflow che è 2^31=2147483648
  /*Dalle prove fatte, anche se è assurdo, risulta che IntMin non può essere posto
   *a -32768, che sarebbe il più piccolo numero rappresentabile su due Bytes se vogliamo
   *la compatibilità con GTPPLOT.
   *E' stata fatta anche una prova a -4096 che ha dato letture scorrette. Si può pertanto
   *ipotizzare che non sia possibile mettere qualsiasi numero negativo in IntMin, se vogliamo la compatibilità con GTTPLOT.
  */

  //Allocazione spazi:
  offset= new float[nVars];
  factor= new float[nVars];

  /* Fase 1: Preparazione file di estensione cfg*/
  //Apertura file:
  pFile=fopen(cfgFileName.toLatin1().data(),"w");
  if(pFile==nullptr)
      return "Unable to open file";
  //Scrittura prima riga  ())a differenza di adf, il commento lo metto al posto dello station name):
  row=stationName+",999,1999\n";
  fprintf(pFile,row.toLatin1().data());
  //Scrittura seconda riga (registro tutti i segnali come analogici; il -1 è perché nVars contiene il numero dei segnali +1, in quanto vars contiene anche il tempo):
  fprintf(pFile,"%d,%dA,0D\n",nVars-1,nVars-1);
  //Scrittura dati relativi ai vari segnali (tempo escluso):
  for(iVar=1;iVar<nVars;iVar++) {
    int index=vars[iVar];
    //Nome privo del prefisso immesso da PlotXY:
    if(fileType==PL4_1 ||fileType==PL4_2){
      len=varNames[index].count();
      if(varNames[index][1]==':')
        varName=varNames[index].mid(2);
      else
        varName=varNames[index].mid(3);
    }else
      varName=varNames[index];
    //Individuo l'unità di misura:
    CCBM[0]=varNames[index][0];
    CCBM[1]=0;
    if(varNames[index][3]==':')
      CCBM[1]=varNames[index][1];
    unit[1]=unit[2]=0;
    switch(CCBM[0].toLatin1()){
      case 'v': unit[0]='V'; break;
      case 'i': unit[0]='A'; break;
      case 'c': unit[0]='A'; break;
      case 'p': unit[0]='W'; break;
      case 'e': unit[0]='J'; break;
      case 'a': unit="DEG"; break;
      default:  unit[0]='?'; break;
    }
    //Individuo il valore minimo e massimo:
    offset[index]=max=0;
    for(point=0; point<numOfPoints; point++){
      if(y[index][point]<offset[index])offset[index]=y[index][point];
      if(y[index][point]>max)max=y[index][point];
    }
    factor[index]=(max-offset[index])/float(iVarMax-iMin);
    if(factor[index]==0)
      factor[index]=float(1.e-45); //L'es. deve essere 45: se metto 46 il risultato è portato a 0!
    //An,ch_id,ph,ccbm,uu,a,b,skew,min,max,primary,secondary,PS
    // Significato delle variabili esplicitamente assetnate nel descrittore fprintf:
    //  0.0: skew
    //  1.0, 1.0: primary, secondary
    //  p: PS

    fprintf(pFile,"%d,%s , ,%s,%s,%e,%e,0.0,%d,%d,1.0,1.0,p\n",
        iVar,varName.toLatin1().data(),CCBM.toLatin1().data(),unit.toLatin1().data(),
               factor[index],offset[index],iMin,iVarMax);  //a, b, min, max
  }
  //Line frequency (convenzionalmente sempre 50 Hz):
  fprintf(pFile,"50\n");
  //Number of Sampling rates. Io voglio salvare in comtrade risutati che possono derivare da simulazioni a passo variabile, quindi i campioni sono scritti con il proprio istante, con registrazione quindi continua, priva del sample rate. Pertanto nrates=0=stamp:
  fprintf(pFile,"0\n");
  //"Sampling rate" e "end sample"
  if(y[0][1]<=y[0][0]) return "First variable not monothonically increasing";
  fprintf(pFile,"0.0,%d\n",numOfPoints);
  //"Date" e "Time":
  dateTime=QDateTime::currentDateTime();
  row=dateTime.toString("dd/mm/yyyy,hh:mm:ss.zzz");
  // Siccome i decimali di secondo devono arrivre al microsecondo, aggiungo tre zeri in fondo:
  row=row+"000\n";
  fprintf(pFile,row.toLatin1().data());
  fprintf(pFile,row.toLatin1().data());
  //File type: per ora solo ASCII:
  fprintf(pFile,"ASCII\n");
  //Time multiplication factor.
  factor[0]=y[0][numOfPoints-1]/iTimeMax;
  // Siccome il tempo va inteso in microsecondi il fattore da scrivere su file deve essere moltiplicato per 1.0e6
  fprintf(pFile,"%e\n",double(factor[0])*1.0e6);
//  fprintf(pFile,"%e\n",factor[0]);

  if(fclose (pFile))
    return "Error closing file";

  /* Fase 2: Preparazione file di estensione dat*/
  //Apertura file:
  datFileName=cfgFileName;
  len=datFileName.count();
  datFileName[len-3]='d';
  datFileName[len-2]='a';
  datFileName[len-1]='t';
  pFile=fopen(datFileName.toLatin1().data(),"w");

  //Scrittura dati:
  for(point=0; point<numOfPoints; point++){
    fprintf(pFile,"%d,",point+1);
    fprintf(pFile,"%d",int(y[0][point]/factor[0]-0.5f));
    for(iVar=1; iVar<nVars; iVar++)
      fprintf(pFile,",%d",int((y[vars[iVar]][point]-offset[vars[iVar]])/factor[vars[iVar]]-0.5f));
    fprintf(pFile,"\n");
  }
  if(fclose (pFile))
    return "Error closing file";
  delete[] offset;
  delete[] factor;
  return "";
}


QString CSimOut::saveToMatFile(QString fileName) {
  /* Saves all variables in current CSimOut object in a file having fileName as file name
   */
  int var, point;
  FILE * pFile;
    header.type=110;
    header.nCols=1;
    header.imagf=0;

    header.nRows=numOfPoints;

    //file opening:
  pFile=fopen(fileName.toLatin1().data(),"w+b");
  if(pFile==nullptr)
    return "Unable to open file";

  //Before writing we convert names in matlab-compliant form:
  if(fileType==PL4_1 || fileType==PL4_2){
    namesPl4ToAdf(0);
    namesAdfToMat();
  }else if(fileType==ADF)
    namesAdfToMat();
  else if(fileType==COMTRADE)
    namesComtradeToMat(true);

  //Actual writing.
  for(var=0;var<numOfVariables;var++) {
    header.namlen=varNames[var].size()+1;
    fwrite(&header,sizeof(header),1,pFile);
    fwrite(varNames[var].toLatin1().data(),sizeof(char),size_t(header.namlen),pFile);
    for(point=0; point<numOfPoints; point++)
      fwrite(&y[var][point],sizeof(float),1,pFile);
  }
  if(fclose (pFile))
    return "Error closing file";
  else
    return "";
}

QString CSimOut::saveToMatFile(QString fileName, int nVars,int vars[]) {
  /* Selectively saves vVars variables (whose indexs are in vars{} from current CSimOut
   * object in a file having fileName as file name
  */
  int var, point;
  FILE * pFile;
    header.type=110;
    header.nCols=1;
    header.imagf=0;

    header.nRows=numOfPoints;

  //file opening:
  pFile=fopen(fileName.toLatin1().data(),"w+b");
  if(pFile==nullptr)
      return "Unable to open file";

  //Before writing we convert names in matlab-compliant form:
  if(fileType==PL4_1 || fileType==PL4_2){
    namesPl4ToAdf(0);
    namesAdfToMat();
  }else if(fileType==ADF)
    namesAdfToMat();
  else if(fileType==COMTRADE)
    namesComtradeToMat(true);

  //Actual writing.
  for(var=0;var<nVars;var++) {
    header.namlen=varNames[vars[var]].size()+1;
    fwrite(&header,sizeof(header),1,pFile);
    // Se i nomi vengono da un file Modelica contengono il puntino, che non è accettato da Matlab; correggo il puntino in underscore:
    QString name=varNames[vars[var]];
    name.replace('.','_');
    fwrite(name.toLatin1().data(),sizeof(char),size_t(header.namlen),pFile);
//    fwrite(varNames[vars[var]].toLatin1().data(),sizeof(char), size_t(header.namlen),pFile);
    for(point=0; point<numOfPoints; point++)
    fwrite(&y[vars[var]][point],sizeof(float),1,pFile);
  }
  if(fclose (pFile))
    return "Error closing file";
  else
    return "";
}

//---------------------------------------------------------------------
QString CSimOut::saveToPl4File(QString fileName) {
    int i, *vars;
  QString msg;
  vars = new int[numOfVariables];
  for(i=0; i<numOfVariables; i++)vars[i]=i;
    msg=saveToPl4File(fileName,numOfVariables, vars);
  delete vars;
  return msg;
}

//----------------------------------------------------------------------
QString CSimOut::saveToPl4File(QString fileName, int nVars, int vars[])  {
/* Function per la scrittura di variabili su un file di output di tipo ATP (pl4) con sottotipo "pisa" (Newpl4=2).
Essenzialmente viene usata all'interno di PlotXY per creare un file compatto
comprendente un sottoinsieme di variabili: quelle che costituiscono un grafico.
L'implementazione è appena incompleta:
- non viene scritto il parametro lLast nell'intestazione
- non viene fatto alcuno allineamento al byte fra la fine dell'intestazione e l'inizio dei dati
Questo un quanto queste due caratteristiche non sono in alcuno modo necessarie per la lettura attraverso PlotXY, e, in teoria non dovrebbero essere necessarie ad alcun programma di postprocessing. Le ho menzionate qui, in quanto i files creati potrebbero non risultare in futuro compatibili con qualche altro programma di post-processing, e, nel caso, la compatibilità potrebbe essere ottenuta agendo su questi due parametri.
*/
  unsigned char Pl4Type=132;
  char code[4]="   ";
  QString charString="11-Nov-18  11.00.00", nameString;
  FILE * fp;
  QString retStr;
  int iVar, iPoint;
  struct HeadInteger{
    int numInt, nEnerg, KNT, precision;
  float tMax, deltaT;
  int NV, NC, lFirst, lLast, IHSPl4, modHFS, numHFS, commBytes;
  } headInt;

  if((fileType==PL4_1 || fileType==PL4_2) && runType!=rtTimeRun){
    retStr="Sorry, output type PL4 not available for Frequency Scan and HFS runs\n"
    "Please, choose ADF or MAT output format.";
    return retStr;
  }
  if(fileType!=PL4_1 && fileType!=PL4_2){
    retStr="pl4 output available only when saving from pl4 files.";
    return retStr;
  }
  headInt.numInt=14;
  headInt.precision=headInt.KNT=headInt.nEnerg=0;
  headInt.tMax=y[0][numOfPoints-1];
  headInt.deltaT=y[0][1]-y[0][0];
  headInt.NV=0;
  headInt.NC=(nVars-1)*2;
  headInt.IHSPl4=1;
  headInt.modHFS=0;
  headInt.numHFS=1;
  headInt.lFirst=76 +  //carattere iniziale più stringa della data (19)
                       //più spazio occupato dagli interi prima dei nomi (56)
       16*(nVars/headInt.numHFS-1) + 1;
  headInt.commBytes=0;

  fp=fopen(fileName.toLatin1().data(),"w+b");
//  fp=fopen(fileName.c_str(),"wb");
  if(fp==nullptr){
    return "Unable to open file (does it exist?)";
  }
  fwrite(&Pl4Type,1,1,fp);
  //Scrittura data e ora:
  if(fileType==PL4_1 || fileType==PL4_2)
      charString=Date+"  "+Time;
//  fwrite(charString.c_str(),1,19,fp);
  fwrite(charString.toLatin1().data(),1,19,fp);
  //Scrittura intestazione:
  fwrite(&headInt,sizeof(HeadInteger),1,fp);

  //Scrittura nomi variabili:
  for(iVar=1; iVar<nVars; iVar++){
    switch(int(varNames[vars[iVar]][0].toLatin1())){
      case int('s'): //Variabile SM
        code[3]='1';
        break;
      case int('t'): //Variabile TACS
        code[3]='2';
        break;
      case int('m'): //Variabile MODELS
        code[3]='3';
        break;
      case int('v'):
        if(varNames[vars[iVar]].length()<9)
          code[3]='4';  //node voltage
        else
          code[3]='8';  //branch voltage
        break;
      case int('u'): //Variabile UM
        code[3]='5';
        break;
      case int('p'): //Branch o Switch power
        code[3]='6';
        break;
      case int('e'): //Branch o Switch energy
        code[3]='7';
        break;
      case int('c'): //Branch current
        code[3]='9';
        break;
    }
    switch(code[3]){
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
        nameString=varNames[vars[iVar]].mid(2,6);
        break;
      default:
        nameString=varNames[vars[iVar]].mid(2,6) +
                   varNames[vars[iVar]].mid(9,6);
        break;
    }
    fwrite(&code,4,1,fp);
    fwrite(nameString.toLatin1().data(),12,1,fp);
  }

  //Scrittura valori:
  for(iPoint=0; iPoint<numOfPoints; iPoint++)
    for(iVar=0; iVar<nVars; iVar++)
        fwrite(&y[vars[iVar]][iPoint],sizeof(float),1,fp);

    if(fclose (fp))
    return "Error closing file";
  else
    return "";
}

//----------------------------------------------------------------------
CSimOut::~CSimOut(){
  delete[] varNames;
  delete[] factorUnit;
    if(y!=nullptr)DeleteFMatrix(y);
}

