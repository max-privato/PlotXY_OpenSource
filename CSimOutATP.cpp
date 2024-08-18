 SReturn CSimOut::loadFromPl4File(QString fullName)  {
/* Function per la lettura delle informazioni da un file di output di ATP (pl4).
    A partire da Marzo 99 la routine è in grado di interpretare il formato
  del tipo NewPl4=2.
  Il valore di ritorno contiene sia un messaggio che un codice numerico.
  Valori del codice:
  code=0 tutto ok
  code=1 errore grave e file non letto
  code=2 il messaggio di ritorno è un warning e il file è letto
*/
  static bool warnPl4Issued=false;
  FILE * fpIn;
  unsigned char Pl4Type;
  QFileInfo fi(fullName);
  SReturn  retVal;

  retVal.code=0;

  runType=rtTimeRun;
  fileLength=fi.size(); //se non lo trova il size darà 0
  lastFileSize=fileLength;
  if(fileLength==0){
      retVal.msg="File missing or having zero bytes!";
      retVal.code=1;
      return retVal;
  }

  fpIn=fopen(qPrintable(fullName),"rb");
  if(fpIn==NULL){
    retVal.msg="Unable to open file (does it exist?)";
    retVal.code=1;
    return retVal;
  }
  fileInfo=fi;
  fread(&Pl4Type,1,1,fpIn);

  rewind(fpIn);
  if(Pl4Type==132){
    retVal.msg=loadFromPl4Type2(fpIn);
    fileType=PL4_2;
  }else{
    retVal.msg=loadFromPl4Type1(fpIn);
    fileType=PL4_1;
    if(!warnPl4Issued){
        retVal.msg= "The pl4 file format being loading is of old type (0 or 1).\n"
                    "Please use NEWPL4=2 (or NOPISA=0) in your ATP STARTUP file.\n\n"
                    "This warning will be issued once per session, whenever old pl4 files are loaded.";
        retVal.code=2;
        warnPl4Issued=true;
    }
  }

  delete [] sVars;
  sVars=new SVar[numOfVariables];
  for(int i1=0; i1<numOfVariables; i1++)
      sVars[i1].unit=giveAutoUnits(varNames[i1][0]);

  fclose (fpIn);
  if(runType==rtTimeRun)
    allowsPl4Out=true;
  else
    allowsPl4Out=false;
  return retVal;
  }


//-----------------------------------------------------------------------------------
QString CSimOut::loadFromPl4Type1(FILE * fpIn)  {
    // ********************************
    // Definizione variabili da Pl42mat
    // ********************************
    char c;
  int i,i1,
        headerLength;
    int iL;
    QString retStr;
    // Primo record del file .PL4
    struct record1 {
        char time19[19];// Data su 19 caratteri
        int totBusNum,  // N. di tutti i nodi della rete (N. di nomi in record 3)
                        //nome originario: NTOT
            nv,         // nell'out ci sono NV tensioni di nodo o di ramo. La
                        // discriminazione fra i due casi avviene sul valore
                        // iTotVar ( 0 per terra)
            nc,         // NC-NV= numero di variabili fra  correnti, TACS,
                        // MODELS, UM, SM
            nonBusNamesNum, // N.di nomi in record 2 (Nomi TACS, MODELS, SM): NonBusNames
                            //Nome originario: MAXBUS
            UMNamesNum,     // N.di indici in record 4 (indicizzano i nomi delle UM)
                           //nome originario: NUMBUS
            LFirst,LLast;
        } rec1;
    char
        *nonBusNames=0, // Vettore dei nomi (6char) delle variabili TACS, MODELS, UM, SM
                        // Contengono anche le stringhe "TACS  ", "MODELS",
                        // "UM-#  ", "MACH #" (essendo '#' in realt un digit)
                      //nome originario: TEXVEC
        *busNames=0;  // Vettore dei nomi (6 car) di tutti i nodi della rete
                      //nome originario: BUS
    int *iUM=0;       // Vettore degli indici dei nomi delle variabili della U.M.
                      //nome originario: JBUSUM
    int *iTotVar=0;   // Vettore degli indici (long) delle variabili registrate
                      //nome originario: IBSOUT
    int nvar,j,l;

    // Lettura del primo record
    // Non si può leggere tutta insieme la struttura perché in memoria al campo
    // time[19] potrebbe essere aggiunto un byte per ragioni di allineamento
    fread(&rec1,1,19,fpIn);                     // Legge la data
    if(!isdigit(rec1.time19[0])||!isdigit(rec1.time19[1])||rec1.time19[2]!='-'){
        retStr="Not a valid ATP C-Like file";
        goto Return;
    }
    Date=QString(rec1.time19).mid(0,9);
    Time=QString(rec1.time19).mid(11,8);
    fread(&rec1.totBusNum,sizeof(rec1.totBusNum),7,fpIn); // Legge i rimanenti campi

    //Variabili da inizializzare per l'eventuale salvataggio successivo in formato PL4:
 //   ModHFS=(enum EModHFS )0;
 //   IHSPl4=1;

    // Alloco lo spazio per i nomi contenuti nei record seguenti
    nonBusNames=new char[rec1.nonBusNamesNum*6];
    busNames   =new char[rec1.totBusNum  *6];
    if(rec1.UMNamesNum>0)iUM=new int[rec1.UMNamesNum];
    iTotVar=new int[rec1.nc];

    // Lettura del secondo record: vettore del nome delle variabili diverse dai nomi dei nodi (quindi variabili TACS, MODELS, UM, SM)
    fread(nonBusNames,6,(size_t)rec1.nonBusNamesNum,fpIn);

    // Lettura del terzo record: vettore del nome di tutti nodi della rete (anche quelli non utilizzati per l'output)
    fread(busNames,6,(size_t)rec1.totBusNum,fpIn);

    // Lettura del quarto record: vettore degli indici dei nomi delle variabili della U.M.
    if(rec1.UMNamesNum>0)fread(iUM,(size_t)rec1.UMNamesNum,sizeof(int),fpIn);

    // Lettura del quinto (e ultimo) record: vettore degli indici dei nomi delle variabili (escluso la UM)
    fread(iTotVar,sizeof(int),(size_t)rec1.nc,fpIn);

    if(rec1.UMNamesNum>0)
        headerLength=sizeof(rec1)-1+6*(int)(rec1.nonBusNamesNum+rec1.totBusNum) +4*(int)(rec1.UMNamesNum +rec1.nc);
    else
        headerLength=sizeof(rec1)-1+6*(int)(rec1.nonBusNamesNum+rec1.totBusNum) +4*(int)(rec1.nc);
    // Se il numero di bytes del file è pari, allora in STARTUP era LENREC=1,
    // oppure si usava la versione ATP per Win32 che forza comunque internamente
    // LENREC a 1. Allora devo bypassare il numero di spazi presenti per rendere
    // la dimensione dell'intestazione pari alla dimensione delle righe dei dati.
    if((fileLength/2*2)==fileLength){
        char c;
        int DRD, //DataRowDimension
        XB;  //ExtraBytes

        DRD=(1+(int)rec1.nc/2)*4;

        XB=DRD-(headerLength-headerLength/DRD*DRD); // =DRD-resto della divisione HeaderLength/DRD
        for(int ii=0;ii<XB;ii++) fread(&c,1,1,fpIn);
          headerLength+=XB;
    }

    // Per ogni variabile ci sono 2 indici.
    nvar=rec1.nc/2;	  //Numero totale di variabili i cui valori numerici sono memorizzati sul file (eccetto il tempo)
    numOfVariables=nvar+1;

    for(i=0; i<rec1.LFirst-headerLength-1; i++)  	fread(&c,1,1,fpIn);
    delete[] varNames;
    varNames=new QString[numOfVariables];
    varNames[0]="t";

    // Tensioni (di nodo o di ramo): prime nv/2 coppie di indici in iNonUM
    // Se sarà j=0, la tensione sarà di nodo, altrimenti di ramo.
    for(i=0;i<rec1.nv/2;i++) {
      l=iTotVar[2*i];
      j=iTotVar[2*i+1];
      if(j==0){
        varNames[i+1]="v:" + QString(busNames+(l-1)*6).mid(0,6);
        blockNumOfVariables[0]++;
      }else{
        varNames[i+1]="v:" + QString(busNames+(l-1)*6).mid(0,6) + "-" + QString(busNames+(j-1)*6).mid(0,6);
        blockNumOfVariables[1]++;
      }
    }
    // Correnti, TACS, MODELS, UM, SM: dalla nv/2+1 alla nc/2=nvar coppia di indici in iTotVar
    for(i=rec1.nv/2;i<nvar;i++) {
      l=iTotVar[2*i];
      j=iTotVar[2*i+1];
      if( (l<=rec1.totBusNum)&&(j<=rec1.totBusNum) ) {
        // Si tratta di corrente (di ramo, ovviamente)
        varNames[i+1]="c:" + QString(busNames+(l-1)*6).mid(0,6) + "-" + QString(busNames+(j-1)*6).mid(0,6);
        blockNumOfVariables[2]++;
      }else if( (l>rec1.totBusNum)&&(l<=rec1.totBusNum+rec1.nonBusNamesNum)&&
          (j>rec1.totBusNum)&&(j<=rec1.totBusNum+rec1.nonBusNamesNum) ) {
        // Si tratta di variabile TACS, MODELS o SM
        l-=rec1.totBusNum;
        j-=rec1.totBusNum;
        switch(nonBusNames[(l-1)*6]){
          case 'T':  //Variabile TACS
            varNames[i+1]="t: " + QString(nonBusNames+(j-1)*6).mid(0,6);
            blockNumOfVariables[3]++;
            break;
          case 'M': //Variabile Synchronous Machine o MODELS
            if(nonBusNames[(l-1)*6+1]=='O'){  //variabile MODELS
              varNames[i+1]="m:" + QString(nonBusNames+(j-1)*6).mid(0,6);
              blockNumOfVariables[4]++;
            } else{
              //Synchronous machine:
              varNames[i+1]="s" + QString(nonBusNames+(l-1)*6+5).mid(0,1) + ":" +  QString(nonBusNames +(j-1)*6).mid(0,6);
              blockNumOfVariables[5]++;
            }
            break;
          case 'U': //compare solo dal 2007 nel caso di rec1.UMNamesNum<0
            if(*(nonBusNames+(l-1)*6+4)==' ') //Variabili fino a 9
               varNames[i+1]="u" + QString(nonBusNames+(l-1)*6+3).mid(0,1) + ":" + QString(nonBusNames +(j-1)*6).mid(0,6);
            else                              //Variabili da 10 in poi
               varNames[i+1]="u" + QString(nonBusNames+(l-1)*6+3).mid(0,2) + ":" + QString(nonBusNames +(j-1)*6).mid(0,6);
            break;
          }
        }else if( (l>rec1.totBusNum+rec1.nonBusNamesNum) && (j>rec1.totBusNum +rec1.nonBusNamesNum) ) {
          // Si tratta di variabile di Universal Machine (UM)
          l=l-rec1.totBusNum-rec1.nonBusNamesNum;
          j=j-rec1.totBusNum-rec1.nonBusNamesNum;
          l=iUM[l-1];
          j=iUM[j-1];
          if(*(nonBusNames+(l-1)*6+4)==' ') //Variabili fino a 9
            varNames[i+1]="u" + QString(nonBusNames+(l-1)*6+3).mid(0,1) + ":" + QString(nonBusNames +(j-1)*6).mid(0,6);
          else                                   //Variabili da 10 in poi
            varNames[i+1]="u" + QString(nonBusNames+(l-1)*6+3).mid(0,2) + ":" +  QString(nonBusNames +(j-1)*6).mid(0,6);
          blockNumOfVariables[5]++;
      }
    }

    // Numero di punti contenuti nel file
    if(rec1.LLast==1)
        numOfPoints=(fileLength+1-rec1.LFirst)/(numOfVariables*sizeof(int));
    else
        numOfPoints=(rec1.LLast-rec1.LFirst) / (numOfVariables*sizeof(int));
//  numOfPoints=(fileLength-47-2*(3*rec1.nonBusNamesNum+3*rec1.totBusNum+ 2*rec1.UMNamesNum+ 2*rec1.nc))/(4*(nvar+1));

    //Ora alloco spazio per la matrice dei dati ed effettuo la lettura:
    if(y!=NULL)DeleteFMatrix(y);
    y=CreateFMatrix(numOfVariables,numOfPoints);
    if(y==NULL){
        retStr="Unable to allocate space for data storage";
        goto Return;
    }

    for(iL=0; iL<numOfPoints; iL++){
        for(i1=0; i1<numOfVariables; i1++){
            i=(int)fread(&y[i1][iL],sizeof(float),1,fpIn);
            if(i!=1){
                numOfPoints=iL;
                retStr="";
                goto Return;
            }
        }
    }
Return:
//Disallocazione variabili locali allocate dinamicamente:
    delete[] nonBusNames;
    delete[] busNames;
    if(rec1.UMNamesNum>0)delete[] iUM;
    delete[] iTotVar;

    return retStr;
}

//----------------------------------------------------------------------
QString CSimOut::loadFromPl4Type2(FILE * fpIn){
  char c, charString[20];
  int i, iL, i1, iDummy,
        HL,  // "HeaderLength", lunghezza in Bytes dell'header
        DRD; //"DataRowDimension", dimensione in Bytes di una riga di dati
  struct HeadInteger{
    int numInt, NEnerg, KNT, Precision;
    float TMax;
    int DeltaT, NV, NC, LFirst, LLast, IHSPl4, ModHFS, numHFS, commBytes;
  } headInt;
  //Bypasso il primo carattere:
  fread(&c,1,1,fpIn);
  //lettura data e orario:
  fread(charString,1,19,fpIn);
  Date=QString(charString).mid(0,9);
  Time=QString(charString).mid(11,8);
  //Lettura numeri interi:
  fread(&headInt,sizeof(HeadInteger),1,fpIn);
  iDummy=0;
  //Se il numero di interi supera 14, allora esso è 15; il 15° intero, se positivo
  //indica che il file contiene risultati di Harmonic Frequency Scan
  if(headInt.numInt>14)
    fread(&iDummy,sizeof(int),1,fpIn);
  if(iDummy>0)  runType=rtHFreqScan;
  //Non leggo eventuali altri interi che potrebbero essere inseriti in futuri formati:
  for(i=0;i<headInt.numInt-15; i++)
    fread(&iDummy,sizeof(int),1,fpIn);
  numOfVariables=headInt.NC/2+1;
  //leggo i nomi delle variabili:
  delete[] varNames;
  varNames=new QString[numOfVariables];
  varNames[0]="t";
  if(headInt.IHSPl4>1){
    varNames[0]="f";
    if(runType!=rtHFreqScan) runType=rtFreqScan;
  }
//  ModHFS=(enum EModHFS )headInt.ModHFS;
//  IHSPl4=headInt.IHSPl4;
  //Correzione di un baco nel formato del pl4. Infatti per simulazioni nel tempo
  //HeadInt.NumHFS dovrebbe essere 1, mentre talvolta è 0 (file BugNumHFS.pl4)
  if(headInt.numHFS==0)
      headInt.numHFS=1;
    for(i=1; i<numOfVariables; i++){
      fread(charString,1,16,fpIn);
//      QString qstring=new QString(charString);
      switch(charString[3]){
        case '1': //Variabile SM
          varNames[i]="s"+QString(charString+9).mid(0,1)+":"+QString(charString+10).mid(0,6);
          break;
        case '2': //Variabile TACS
          varNames[i]="t:"+QString(charString+10).mid(0,6);
          break;
        case '3': //Variabile MODELS
          varNames[i]="m:"+QString(charString+10).mid(0,6);
          break;
        case '4': //node voltage
          varNames[i]="v:"+QString(charString+4).mid(0,6);
          break;
          case '5': //Variabile UM
  //  Vecchia riga, probabilmente erronea:
  //  VarNames[i]="u"+AnsiString(CharString+9,1)+":"+AnsiString(CharString+10,6);
            if(charString[8]==' ') //Macchine da 1 a 9
              varNames[i]="u"+QString(charString+7).mid(0,1)+":"+QString(charString+10).mid(0,6);
            else //macchine da 10 a 99
              varNames[i]="u"+QString(charString+7).mid(0,2)+":"+QString(charString+10).mid(0,6);
            break;
        case '6': //Branch o Switch power
          varNames[i]="p:"+QString(charString+4).mid(0,6)+"-"+QString(charString+10).mid(0,6);
          break;
        case '7': //Branch o Switch energy
          varNames[i]="e:"+QString(charString+4).mid(0,6)+"-"+QString(charString+10).mid(0,6);
          break;
        case '8': //Branch voltage
          varNames[i]="v:"+QString(charString+4).mid(0,6)+"-"+QString(charString+10).mid(0,6);
          break;
        case '9': //Branch current
          varNames[i]="c:"+QString(charString+4).mid(0,6)+"-"+QString(charString+10).mid(0,6);
          break;
/*
        case 0: //In taluni casi (es. FSCAN.pl4)  stato trovato che CharString sia di tutti \0!
          VarNames[i]="?:";
          break;
*/
    }

    //Tratto il caso di Frequency Scan in polari:
      /* Qui arrivo sempre con "i" dispari. Il successivo i pari contiene l'angolo del numero
         complesso in polari. Deve sempre esistere un indice pari al valore corrente di i.
         Di conseguenza metto 'a' al posto del secondo carattere, normalmente ':' della prossima
         variabile in modo che alla prossima lettura il nome della variabile cominci per "va"
         oppure "ca"
         La ragione per cui qui arrivo sempre con i dispari è che alla prima passata ho 1;
         qui sotto aumento ancora di 1, e il ciclo del loop aumenta di un altra unità.
         */
    if(headInt.IHSPl4>1 && headInt.ModHFS==1){
      varNames[i+1]=varNames[i];
      varNames[i+1][1]='a';
      fread(charString,1,16,fpIn);
      i++;
    }

    //Tratto il caso di Frequency Scan in rettangolari:
    if(headInt.IHSPl4>1 && headInt.ModHFS==2){
        varNames[i].prepend("#");
    //      varNames[i]=AnsiString(varNames[i][1])+AnsiString(varNames[i].c_str());
      varNames[i+1]=varNames[i];
      varNames[i][1]='r';
      varNames[i+1][1]='i';
      fread(charString,1,16,fpIn);
      i++;
    }

    //Tratto il caso di Frequency Scan in polari + rettangolari:
    if(headInt.IHSPl4>1 && headInt.ModHFS==3){
      varNames[i+1]=varNames[i];
      varNames[i+1][0]='a';
      varNames[i+2]=varNames[i].prepend("#");
      varNames[i+3]=varNames[i+2];
      varNames[i+2][1]='r';
      varNames[i+3][1]='i';
      fread(charString,1,16,fpIn);
      fread(charString,1,16,fpIn);
      fread(charString,1,16,fpIn);
      i+=3;
    }
  }

  /*********************************************************
  Commento Importante!!
  La lunghezza dell'header ad oggi (Gen 2000) non è completamente
  definita, in quanto sussiste incertezza nel caso di PISA FREQUENCY SCAN.
  Infatti in alcune versioni di ATP il numero di campi da 16 bytes
  per i nomi è NC/2 (ad es. Watcom ATP), in altri casi NC/4 (ad es. djgpp ATP).
  Pertanto il seguente calcolo è una SOTTOSTIMA di HL, in modo da avere
  una SOVRASTIMA di NumOfPoints
  *********************************************************/

  HL=  76 +  //carattere iniziale pi stringa della data (19)
            //pi spazio occupato dagli interi prima dei nomi (56)
       16*(numOfVariables/headInt.numHFS-1) +  //Nomi delle variabili
       headInt.commBytes;       //Bytes dedicati ai commenti
    DRD=numOfVariables*4;
    numOfPoints=(fileLength-HL)/DRD;

  rewind(fpIn);
  for(i=0; i<headInt.LFirst-1; i++)  	fread(&c,1,1,fpIn);

    //Ora alloco spazio per la matrice dei dati ed effettuo la lettura:
    if(y!=NULL)DeleteFMatrix(y);
    y=CreateFMatrix(numOfVariables,numOfPoints);
    if(y==NULL){
        return "Unable to allocate space for data storage";
    }

    for(iL=0; iL<numOfPoints; iL++){
        for(i1=0; i1<numOfVariables; i1++){
            i=fread(&y[i1][iL],sizeof(float),1,fpIn);
            if(i!=1){
                numOfPoints=iL;
                return "";
            }
        }
    }
    return "";
}


