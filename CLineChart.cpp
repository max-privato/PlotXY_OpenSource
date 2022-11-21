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

#include <QtGui>
#include <QApplication>
#include <QFontMetrics>
#include <QInputDialog>
#include <QMessageBox>
#include <QScreen>
#include <QSet>
#include <QSvgGenerator>
#include <QString>
#include <stdio.h>
#include <time.h>
#include "CLineChart.h"
// Le seguenti due righe sono state commentate il 19/11/2018 in quanto è stato introdotto l'uso di qMin() e qMax()
//#define max(a, b)  (((a) > (b)) ? (a) : (b))
//#define min(a, b)  (((a) < (b)) ? (a) : (b))
#define NearInt(x) (int(x+0.5f))
#define NearIntD(x) (int(x+0.5))



static QString smartSetNum(float num, int prec){
    /*  Funzione PURA che scrive su stringa i numeri con un numero prefissato di cifre
     * significative nella versione più compatta possibile, ma senza perdita di informazioni.
     *  E' stato necessario implementarla
     * anche se esiste la funzione di stringa setNum, in quanto quest'ultima taglia gli zeri
     * finali. Pertanto, ad es. 0.1200  (o 0.12) con 4 cifre significative in setNum viene
     * 0.12, con perdita di informazione sugli ultimi due digit, mentre nella presente
     * routine, più correttamente, viene 0.1200
     *
    */
    QChar sep;
    QString out, prepS;
    //Prima di tutto scrivo il numero nella stringa da ritornare in formato esponenziale, e successivamente verifico se esiste la possibilità di una scrittura più compatta che metto nuovamente in out. Se non esisterà questa possibilità, la stringa di uscita sarà out così come derivante dalla seguente riga:
    out.setNum(num,'e',prec-1);
    //Prima di tutto devo capire quante cifre ho dopo l'esponente, se 1 o 2 I due casi, testati rispettivamente con Qt 5.2 (Mac-air) e Qt 5.7 (Fisso Home) sono e+## (qualunque sia l'esponente e formato differenziato a seconda se sia sufficiente un esponente a un dicit (nel qual caso ho E+#) o servano due digit nel qual caso ho E+##). In sostanza mi basta calcolare expSize e vedere sse è di 3 caratteri (E+#) o di 4 (E+##)
    int expSize=out.size()-out.indexOf('e');
    //Se l'esponente è 0 intanto mi riparmio le ultime 4 cifre:
    int exp;
    exp=out.right(expSize-1).toInt();
    //Se l'esponente è zero lo tolgo e via:
    if(exp==0){
      out.chop(expSize);
      return out;
    }

    //nelle seguenti analisi devo maneggiare il separatore decimale sep, che può essere punto o virgola. Faccio un codice che sia robusto in tal senso:
    if(out.contains('.'))
        sep='.';
    else
        sep=',';
    // la seguente prepS è la stringa da "prependere" se il numero è in modulo minore di 1 ovviamente nel caso in cui sia possibile rinunciare alla notazione esponenziale
    prepS="0"+QString(sep);
    //Il seguente loop è valido solo per esponenti positivi, e compatibili con il livello di precisione richiesto. Se il numero è troppo grande per essere rappresentato in utte le sue cifre in formato floating point non verrà processato e resterà attiva la notazione esponenziale precedentemente usata per la scrittura in out
    for (int i=1; i<prec; i++)
      if(exp==i &&prec>i){
        out.remove(sep);
        out.chop(expSize);
        //Adesso inserisco il puntino nella posizione giusta. Però poi lo ritolgo se diviene l'ultimo carattere.
        if(out[0]=='-')
          out.insert(i+1+1,sep);
        else
          out.insert(i+1,sep);
        if(out[out.length()-1]=='.')
            out.chop(1);
        break;
        if(i+1!=out.length())
          out.insert(i+1+(int)(out[0]=='-'),sep);
        break;
       }
// Rimangono da trattare solo i casi di esponente negativo. Vengono considerati solo esponenti fino a -2 in quanto al più voglio avere due zeri dopo la virgola. Oltre questo è più leggibile la notazione esponenziale:
    if(exp==-1 &&prec>1){
      out.remove(sep);
      if(out[0]=='-')
         out.insert(1,prepS);
      else
         out.insert(0,prepS);
      out.chop(expSize);
     }
    if(exp==-2 &&prec>2){
      if(out[0]=='-'){
        out.insert(1,"0");
        out.remove(sep);
        out.insert(1,prepS);
      }else{
        out.insert(0,"0");
        out.remove(sep);
        out.insert(0,prepS);
      }
      out.chop(expSize);
     }
    if(exp==-3 &&prec>3){
      if(out[0]=='-'){
        out.insert(1,"00");
        out.remove(sep);
        out.insert(1,prepS);
      }else{
        out.insert(0,"00");
        out.remove(sep);
        out.insert(0,prepS);
      }
      out.chop(expSize);
     }
    //Il seguente ulteriore caso comporta numeri in floatin point particolarmente lunghi, più lunghi del caso di forma esponenziale ed esponente a 3 digit. Pertanto ha senso mantenerlo solo se l'esponente è a 4 digit:
    if(expSize==4)
      if(exp==-4 &&prec>4){
        if(out[0]=='-'){
          out.insert(1,"000");
          out.remove(sep);
          out.insert(1,prepS);
        }else{
          out.insert(0,"000");
          out.remove(sep);
          out.insert(0,prepS);
        }
        out.chop(expSize);
       }

    return out;
}










static bool isEven(int i)  {
    /*Returns true if the inputted integer is even or zero
     * */
    if(i==0)
      return true;
    else{
      if(i/2*2==i)
        return true;
      else
        return false;
    }
  }


CLineChart::CLineChart(QWidget * parent):QLabel(parent)
{
/*
La reimplementazione della funzione virtual resizeEvent in questo file contiene la chiamata a plot().
*/
     myTimer = new QTimer(this);
    tooltipTimer = new QTimer(this);
    connect(tooltipTimer, SIGNAL(timeout()), this, SLOT(checkTooltip()));
    myTimer->setSingleShot(true);
    connect(myTimer, SIGNAL(timeout()), this, SLOT(resizeStopped()));

/* La sequenza delle righe di questa routine è la seguente:
1) definizione variabile r che serve per il prosieguo
2) attribuzione valori default  variabili indipendenti a complessità crescente e, per il medesimo tipo in ordine alfabetico
3) inizializzazioni finali*/

    // 1) definizione variabile plotRect.
    plotRect=rect();
    /* Qui plotRect ha valori di default con largheza di 100 e altezza di 30.
      Per ragioni non note alla chiamata a resizeEvent ha invece i valori corretti.
      Probabilmente questo è frutto della gestione di Qt dei layout destro e basso che lasciano libero giusto lo spazio che poi è attribuito a CLineChart. Comunque la situazione non è chiarita.
*/


//    2) attribuzione valori default variabili a complessità crescente e, a pari complessità, in ordine alfabetico
    addLegend=true;
    autoLabelXY=false;
    autoMark=false;
    blackWhite=false;
    copying=false;
    dataGot=false;
    dataCursDragging=false;
    dataCursVisible=false;
    dataCurs2Dragging=false;
    dataCurs2Visible=false;
    enableTooltip=true;
    exactMatch=false;
    forceYZero=false;
    linearInterpolate=true;
    makingSVG=false;
    plotDone=false;
    printing=false;
    rectTTVisible=false;
    showPlotCopiedDlg=true;
    strongFilter=false;
    twinScale=false;
    useBrackets=true;
    useUserUnits=false;
    useSmartUnits=true;
    variableStep1=false;
    writeTitle1=false;
    xGrid=false;
    yGrid=false;
    zoomed=false;
    zoomSelecting=false;
    // dopo le bool le int, in ordine alfabetico
    legendHeight=0;
    maxAutolineWPoints=2000;
    minXTic=4;
    minYTic=2;
    numOfVSFiles=0;
    swarmPointWidth=4;
    tooltipMargin=20;
    dataCursSelecting=0;
    //infine le variabili più complesse e le funzioni (in ordine alfabetico)
    manuMarks.lastMark=-1;
    baseFontName="Times";  // i font usati sono solo baseFontName e symbFontName!
    symbFontName="Symbol";
    plotType=ptLine;
    xAxis.type=atX;
    xAxis.scaleType=stLin;
    xVarParam.unitS="";
    xAxis.addZeroLine=false;
    yAxis.type=atYL;
    yAxis.scaleType=stLin;
    yAxis.addZeroLine=false;
    ryAxis.type=atYR;
    ryAxis.scaleType=stLin;
    ryAxis.addZeroLine=false;
    swarmPointSize=ssSquare;

    px=nullptr;
    py=nullptr;
    titleRectF=QRectF(0,0,0,0);
    tooltipRect.setWidth(5);
    tooltipRect.setHeight(5);
    setCursor(myCursor);
    titleText=QString("Double-Click here to set the title text!");
    numFont=baseFont=expFont=lgdFont=font();
    baseFontFamily=baseFont.family();

    //    3) inizializzazioni finali
    myImage= new QImage(rect().width(),rect().height(),QImage::Format_RGB32);
    myPainter=new QPainter(myImage); //L'associazione standard di myPainter, il painter standard è con myImage;
    //associazioni temporanee vengono fatte con altri paint devices in makeSvg() e print().
    hovVarRect=QRect(0,0,0,0);
    /* Prima di selezionare il font di riferimento devo fare designPlot().
    designPlot() richiede che un painter sia già stato creato, e questo è stato fatto qui sopra.
    Tutte le operazioni di scrittura di testo sul widget, qualunque sia il suo paintDevice, vengono effettuate attraverso myPainter.
    Pertanto non serve attribuire il font definito a CLineChart, bensì a myPainter->CLineChart.
*/
    fontSizeType=fsAuto;
    fixedFontPx=11;
    legendFontSizeType=LAuto;
    drawType=dtMC;
    setPlotPenWidth(pwAuto);
    designPlot();
    //il PointPx viene poi aggiornato al generalpointPx in designPlot().
    //In textOut2 viene inoltre modificato temporaneamente il solo pointPx del font del painter di image->


    //Nelle seguenti righe faccio predisposizioni affinché nella routine Plot si possa prima deallocare (rispettivamente con delete, DeleteIMatrix e DeleteFMatrix) gli array e poi riallocarli con le dimensioni giuste.
    cursorXValues=nullptr;
    cursorXValBkp=nullptr;
    curveParamLst.clear();
    startIndex=nullptr;
    stopIndex=nullptr;
    pixelToIndexDX=CLineChart::CreateIMatrix(1,1);
    cursorYValues=CLineChart::CreateFMatrix(1,1);
    cursorYValBkp=CLineChart::CreateFMatrix(1,1);
    setMouseTracking(true);
}


bool CLineChart::allowsZero(float MinRoundValue, float MaxRoundValue,int ntic){
/* Funzione che valuta se con i dati passati c'è una tacca in corrispondenza dello 0*/
    int i;
    float ticInterval=(MaxRoundValue-MinRoundValue)/float(ntic+1);
    for(i=0; i<ntic; i++){
        if( fabsf(MinRoundValue+i*ticInterval)<0.001f)return true;
    }
    return false;
}

void CLineChart::beginRounding(SDigits &Min, SDigits &Max, float MinVal, float MaxVal, unsigned Include0)
{
/* Questa funzione serve a compilare Min e Max (escluso RoundValue) a partire dal minimo e massimo valore di una certa scala, passati come Minval e Maxval; contiene la gestione della forzatura dello zero mediante la variabile "Include0".
Essa prende in considerazione le prime 5 cifre significative di MinVal e MaxVal; e' richiesto che questi ultimi differiscano di qualcosa nelle prime quattro cifre  significative, pertanto prima di chiamare BeginRounding gestisco a parte i casi speciali.
*/

    int delta_ie;
   float Ratio;
   char buffermin[12]="           ", buffermax[12]="           ";

   /* Gestione della forzatura della inclusione dello 0 nella scala sulla base
      del contenuto di "Include0". Essa ha senso solo con MinVal e MaxVal dello
      stesso segno */
   if( MinVal*MaxVal>0 ){
     if(MaxVal>0) {
         Ratio=MinVal/MaxVal;     //Ratio e' la percentuale di vuoti
         if( Ratio <= float(Include0)/100.f) MinVal=0.f;
     }else {
       Ratio=MaxVal/MinVal;
       if( Ratio <= float(Include0)/100.f) MaxVal=0.f;
     }
   }
   Min.Value=MinVal;
   Max.Value=MaxVal;

   /* Compilazione interi delle cifre e segno di Min e Max: */
   sprintf(buffermax,"%+10.3e",double(Max.Value)); //scrivo Max e Min nel formato:
//   sprintf(buffermax,"%+10.3e",Max.Value); //scrivo Max e Min nel formato:
   sprintf(buffermin,"%+10.3e",double(Min.Value)); //       s#.###es##   s: segno

   /* Analizzo le due stringhe e compilo le corrispondenti grandezze: */
   Max.Sign=*buffermax;
   sscanf( buffermax+1, "%1u", &Max.i1 );
   sscanf( buffermax+3, "%1u", &Max.i2 );
   sscanf( buffermax+4, "%1u", &Max.i3 );
   sscanf( buffermax+5, "%1u", &Max.i4 );
   sscanf( buffermax+7, "%u", &Max.ie );

   Min.Sign=*buffermin;
   sscanf( buffermin+1, "%1u", &Min.i1 );
   sscanf( buffermin+3, "%1u", &Min.i2 );
   sscanf( buffermin+4, "%1u", &Min.i3 );
   sscanf( buffermin+5, "%1u", &Min.i4 );
   sscanf( buffermin+7, "%u", &Min.ie );

   /* Qualora i due estremi abbiano ordine di grandezza differente,
      le approssimazioni vanno fatte con riferimento al numero di modulo
      maggiore. */
   delta_ie=Max.ie-Min.ie;
   switch(delta_ie) {
     case -3:
        if(Min.i1==0)break;
        Max.ie=Min.ie; Max.i4=Max.i1; Max.i3=0;
        Max.i2=0;      Max.i1=0;      break;
     case -2:
        if(Min.i1==0)break;
        Max.ie=Min.ie; Max.i4=Max.i2; Max.i3=Max.i1;
        Max.i2=0;      Max.i1=0;      break;
     case -1:
        if(Min.i1==0)break;
        Max.ie=Min.ie; Max.i4=Max.i3; Max.i3=Max.i2;
        Max.i2=Max.i1; Max.i1=0;      break;
     case 0:
        break;
     case 1:
        if(Max.i1==0)break;
        Min.ie=Max.ie; Min.i4=Min.i3; Min.i3=Min.i2;
        Min.i2=Min.i1; Min.i1=0;      break;
     case 2:
        if(Max.i1==0)break;
        Min.ie=Max.ie; Min.i4=Min.i2; Min.i3=Min.i1;
        Min.i2=0;      Min.i1=0;      break;
     case 3:
        if(Max.i1==0)break;
        Min.ie=Max.ie; Min.i4=Min.i1;
        Min.i3=0;      Min.i2=0;      Min.i1=0;    break;
     default:
     /* Se gli ordini di differenza sono piu' di tre il numero di modulo minimo viene posto pari a 0 (con effetto Max minore  dell'1%. sul riempimento del grafico) */
        float AbsMin=Min.Value, AbsMax=Max.Value;
        if(AbsMin<0)AbsMin=-AbsMin;
        if(AbsMax<0)AbsMax=-AbsMax;
        if(AbsMin<AbsMax){
           Min.ie=Max.ie;
           /* (istruz.necessaria per avere all'esterno in Max.ie=Min.ie
             sempre l'esponente dell'estremo di massimo modulo */
           Min.i4=0;   Min.i3=0;   Min.i2=0;   Min.i1=0;
        }else{
           Max.ie=Min.ie;                   // come sopra
           Max.i4=0;   Max.i3=0;   Max.i2=0;   Max.i1=0;
        }
    }                                         //fine switch delta_ie
}

void CLineChart::checkTooltip(){
    bool testBool=QToolTip::isVisible();
    if(!testBool){
        rectTTVisible=false;
        update();
        tooltipTimer->stop();
    }

}


//---------------------------------------------------------------------------
/*
int CLineChart::computeDecimalsOLD(float scaleMin_, float ticInterval, bool halfTicNum_){
    //Calcolo dei decimali da usare per i numeri sulle tacche degli assi.
    //Scrivo le prime due tacche con 4 decimali ed individuo quanti posso ometterne
    //perché nulli. Per far ciò devo trovare, partendo da destra, il primo carattere
    //diverso da '0' e anche la posizione del carattere '.'
    char num[20];
    QString numStr;
    int i, ret, temp1, temp2;
    sprintf(num,"%+.4f",scaleMin_+ticInterval*(1+halfTicNum_));
    numStr=QString::number(scaleMin_+ticInterval*(1+halfTicNum_),'f',4);
    num=numStr.data();
    i=strlen(num);
    do{	 i--; } while(num[i]=='0' && i>0);
    if(num[i]=='.') //Se trovo il puntino prima di altri caratteri non nulli ho 0 decimali
        ret=0;
    else{
        temp1=i; //Ora che ho trovato un carattere diverso da '0' cerco il '.'
        do{	 i--; } while(num[i]!='.' && i>0);
        ret=temp1-i;
        //Il numero di cifre significative non deve comunque superare 5:
        if(temp1>6)ret=min(ret,2);
    }
    sprintf(num,"%+.4f",scaleMin_+2*ticInterval*(1+halfTicNum_));
    numStr=QString::number(scaleMin_+2*ticInterval*(1+halfTicNum_),'f',4);
    num=numStr.data();
    i=strlen(num);
    do{	 i--; } while(num[i]=='0' && i>0);
    if(num[i]=='.')
        temp2=0;
    else{
        temp1=i;
        do{	 i--; } while(num[i]!='.' && i>0);
        temp2=temp1-i;
        //Il numero di cifre significative non deve comunque superare 5:
        if(temp1>6)temp2=min(temp2,2);
    }
    ret=max(temp2,ret);
    return ret;
}
*/

int CLineChart::computeDecimals(float scaleMin_, float ticInterval, bool halfTicNum_){
    //Calcolo dei decimali da usare per i numeri sulle tacche degli assi.
    //Scrivo le prime due tacche con 4 decimali ed individuo quanti posso ometterne
    //perché nulli. Per far ciò devo trovare, partendo da destra, il primo carattere
    //diverso da '0' e anche la posizione del carattere '.'
    QString numStr;
    int i, ret, temp1, temp2;
    numStr=QString::number(double(scaleMin_+ticInterval*(1+halfTicNum_)),'f',4);
    i=numStr.length();
    do{	 i--; } while(numStr[i]=='0' && i>0);
    if(numStr[i]=='.') //Se trovo il puntino prima di altri caratteri non nulli ho 0 decimali
        ret=0;
    else{
        temp1=i; //Ora che ho trovato un carattere diverso da '0' cerco il '.'
        do{	 i--; } while(numStr[i]!='.' && i>0);
        ret=temp1-i;
        //Il numero di cifre significative non deve comunque superare 5:
        if(temp1>6)ret=qMin(ret,2);
    }
    numStr=QString::number(double(scaleMin_+2*ticInterval*(1+halfTicNum_)),'f',4);
    i=numStr.length();
    do{	 i--; } while(numStr[i]=='0' && i>0);
    if(numStr[i]=='.')
        temp2=0;
    else{
        temp1=i;
        do{	 i--; } while(numStr[i]!='.' && i>0);
        temp2=temp1-i;
        //Il numero di cifre significative non deve comunque superare 5:
        if(temp1>6)temp2=qMin(temp2,2);
    }
    ret=qMax(temp2,ret);
    return ret;
}


int CLineChart::computeTic(float minRoundValue_, float maxRoundValue_, int minTic){
/*   Calcolo del numero di tacche da mettere sopra un asse.
 * FUNZIONE PURA: a pari input la funzione ritorna il medesimo output. Non scrive in
 * nessuna variabile diversa da quelle internamente generate.
  Se MinTic>=0, il risultato è sempre fra MinTic e MinTic+3.
  Se MinTic<0. per convenzione il risultato è 0.
  Esemplifico la logica operativa nel caso più importante, ovvero con MinTic=4:
-> divido "idelta" per 5, 6, 7, 8 e scrivo i corrispondenti 4 numeri risultanti su una
   stringa (DeltaTicStr) di formato: ##.###. Il minimo numero di decimali ottenibile sarà
   memorizzato in MinDecimals
-> trovo i numeri di tacche che mi danno sulle tacche un numero di decimali pari a
   MinDecimals. Nel caso in cui il campo numerico da considerare contiene lo 0, verifico
   se fra tali numeri ne esistono alcuni che contemplino la presenza di una tacca in
   corrispondenza dello 0. SOLO NEL CASO DI RISPOSTA NEGATIVA procedo come segue:
     - trovo i numeri di tacche che mi danno sulle tacche un numero di decimali pari a
       MinDecimals+1 o MinDecimals+2. Verifico se fra tali numeri ne esistono alcuni che
       contemplino la presenza di una tacca in corrispondenza dello 0. Se questa verifica
       ha esito affermativo il numero di tacche sarà prescelto fra quelli che comportano
       sulle tacche un numero di decimali pari al più a MinDecimals+1 e consentono di avere
       una tacca in corrispondenza dello 0; in caso contrario fra quelli che comportano sulle
       tacche un numero di decimali pari a MinDecimals.
->  A questo punto ho individuato un set di numeri di tacche ammissibili (SelectedSet).
    Fra di essi scelgo secondo la seguente lista di priorità:
    - fra i numeri selezionati scelgo quello che ha l'ultimo digit più "semplice".
      Uso il seguente ordine si semplicit‡ decrescente:
      . digit '0'
      . digit'5'
      . cifre pari
      . altro
  - A parità di semplicità scelgo il numero di tacche più elevato.

Se minTic è diverso da 4, ad es. è 3, il numero di tacche consentito andrà da 3 a 6, e quindi la divisione iniziale si farà per 4, 5, 6, 7 anzicché 5, 6, 7, 8.

***** Nel seguente codice il numero costante 4 sta per numero di possibilità per il numero di tacche. Ad es. per grafici standard potrà avere 4, 5, 6, o 7 tacche; per grafici piccoli, ad es. 2, 3, 4, o 5.****/

  int i, //indice generico
    id1, id2,  idelta,
    j,  //contatore di caratteri
    ret=0, //codice di ritorno
    Decimals[4], //numero di cifre decimali non nulle
    MinDecimals; //numero minimo di cifre decimali non nulle
  QSet <int> Empty, work, hasMinDecimals, hasMinDecimals_, SAllowsZero, selectedSet;

  char **deltaTicStr;
  char buffer[12];
  deltaTicStr=CreateCMatrix(4,8); //8 E' il numero di spazi necessari per scrivere i numeri su stringa
  if(minTic<0){
    ret=0;
    goto Return;
  }
  if(deltaTicStr==nullptr){
    QMessageBox::critical(this, tr("TestLineChart"),
      tr("Error Allocating Space in \"computeTic\" within \"CLineChart\""), QMessageBox::Ok);
    ret=minTic;
    goto Return;
  }

  //  sprintf(buffer,"%+10.3e",maxRoundValue_-minRoundValue_);
  sprintf(buffer,"%+10.3e",double(maxRoundValue_-minRoundValue_));

// static float fff=-1.0;
// sprintf(buffer,"%+10.3e",fff);

  sscanf(buffer+1, "%1u", &id1);
  sscanf(buffer+3, "%1u", &id2);
  idelta=id1*10+id2;

  MinDecimals=5;
  //Calcolo i numeri di decimali e ne individuo il numero minimo:
  for(i=0; i<4; i ++){

    //    sprintf(deltaTicStr[i],"%7.4f",(float)idelta/(i+minTic+1));
    sprintf(deltaTicStr[i],"%7.4f",double(idelta)/double(i+minTic+1));

    j=7;
    if(allowsZero(minRoundValue_,maxRoundValue_,i+minTic))
      SAllowsZero<<i;
    else
      SAllowsZero.remove(i);
    do{j--;} while(deltaTicStr[i][j]=='0' && j>0);
    Decimals[i]=j-2;
    MinDecimals=qMin(MinDecimals,Decimals[i]);
  }
  //Individuo le intertacche con numero minimo e subminimo di decimali:
  for(i=0; i<4; i++){
    if(Decimals[i]==MinDecimals)
      hasMinDecimals<<i;
    else
      hasMinDecimals.remove(i);
    if(Decimals[i]==MinDecimals+1 || Decimals[i]==MinDecimals+2)
      hasMinDecimals_<<i;
    else
      hasMinDecimals_.remove(i);
  }

  //Ore definisco il SelectedSet che contiene i numeri di tacche che rispettano i criteri
  // prescelti, e fra cui devo poi ritrovare un unico numero da scegliere
  if(minRoundValue_*maxRoundValue_>=0)
    selectedSet=hasMinDecimals;
  else{
    work=hasMinDecimals&SAllowsZero;
    if(work!=Empty)
       selectedSet=work;
    else{
      work=hasMinDecimals_&SAllowsZero;
      if(work!=Empty)
        selectedSet=work;
      else
        selectedSet=hasMinDecimals;
    }
  }
  //Verifico se fra le intertacche selezionate ce n'è qualcuna
  //la cui ultima cifra è lo '0':
  for(i=3; i>=0; i--)
    if(selectedSet.contains(i))
      if(deltaTicStr[i][Decimals[i]+1]=='0'){
        ret=i+minTic;
          goto Return;
      }
    //Verifico se fra le intertacche a numero minimo di decimali ce ne è qualcuna la cui ultima cifra è il '5':
  for(i=3; i>=0; i--)
    if(selectedSet.contains(i))
      if(deltaTicStr[i][Decimals[i]+1]=='5'){
        ret=i+minTic;
        goto Return;
      }
  //Verifico se fra le intertacche a numero minimo di decimali ce ne è qualcuna
  //la cui ultima cifra è pari:
  for(i=3; i>=0; i--)
    if(selectedSet.contains(i)){
      char c=deltaTicStr[i][Decimals[i]+1];
      if(c=='2'||c=='4'||c=='6'||c=='8'){
        ret=i+minTic;
        goto Return;
      }
    }
  //A questo punto ritorno il più grande numero di tacche a minimo numero di decimali
  for(i=3; i>=0; i--)
    if(selectedSet.contains(i)){
      ret=i+minTic;
      goto Return;
    }
    //Riga che serve solo per evitare un warning:
Return:
  CLineChart::DeleteCMatrix(deltaTicStr);
  return ret;
}


char **CLineChart::CreateCMatrix(long NumRows, long NumCols){
    long i;
    char **Matrix;
    //Allocaz. vettore puntatori alle righe:
    Matrix=new char*[NumRows];
    if(Matrix==nullptr)
        return nullptr;
  //Allocaz. matrice:
    Matrix[0]=new char[NumRows*NumCols];
    for(i=1; i<NumRows; i++)
        Matrix[i]=Matrix[0]+i*NumCols;
    return Matrix;
}


float **CLineChart::CreateFMatrix(long NumRows, long NumCols){
    long i;
    float **Matrix;
    //Allocaz. vettore puntatori alle righe:
    Matrix=new float*[NumRows];
    if(Matrix==nullptr)
        return nullptr;
    if(Matrix==nullptr)
        return nullptr;
  //Allocaz. matrice:
    Matrix[0]=new float[NumRows*NumCols];
    if(Matrix[0]==nullptr)
        return nullptr;
    for(i=1; i<NumRows; i++)
        Matrix[i]=Matrix[0]+i*NumCols;
    return Matrix;
}

int **CLineChart::CreateIMatrix(long NumRows, long NumCols){
    long i;
    int  **Matrix;
    //Allocaz. vettore puntatori alle righe:
    Matrix=new int *[NumRows];
    if(Matrix==nullptr)
        return nullptr;
  //Allocaz. matrice:
    Matrix[0]=new int[NumRows*NumCols];
    for(i=1; i<NumRows; i++)
        Matrix[i]=Matrix[0]+i*NumCols;
    return Matrix;
}

void CLineChart::DeleteCMatrix(char **Matrix){
    if(Matrix==nullptr)
        return;
    delete[] Matrix[0];
    delete[] Matrix;
}

int CLineChart::DeleteFMatrix(float **Matrix){
    if(Matrix==nullptr)
        return 1;
    delete[] Matrix[0];
    delete[] Matrix;
    return 0;
}

void CLineChart::DeleteIMatrix(int  **Matrix){
    if(Matrix==nullptr)
        return;
    delete[] Matrix[0];
    delete[] Matrix;
}

//---------------------------------------------------------------------------
void CLineChart::designPlot(void){
 /* Routine per il calcolo delle dimensioni principali del grafico.
Oltre che al momento della costruzione di CLineChart, e ad ogni suo ridimensionamento (comando resizeStopped() andrà utilizzato prima e dopo un eventuale comando di print(), il quale fa riferimento al numero di pixel della stampante, di regola molto maggiore di quello del grafico a schermo.
*/

  int numWidth,maxTic;
  xAxis.ticPixWidth=qMax(4,int(0.015f*plotRect.height()));
  yAxis.ticPixWidth=xAxis.ticPixWidth;
  ryAxis.ticPixWidth=xAxis.ticPixWidth;
  svgOffset= qMax(6,plotRect.width()/100);

  //Stabilisco i tre font del grafico (baseFont, expFont, legendFont) ed attribuisco il baseFont al myPainter
  //Uso una logica DPI-aware
  QScreen *screen=QGuiApplication::primaryScreen();
  myDPI=int(screen->logicalDotsPerInch());
  if(myDPI>100)
    generalFontPx=qMax(qMin(int(0.014f*(plotRect.height()+plotRect.width())),28),12);
  else
     generalFontPx=qMax(qMin(int(0.012f*(plotRect.height()+plotRect.width())),20),10);
  onePixDPI=myDPI/float(96);
  int fontPxSize=generalFontPx;
  if(fontSizeType==fsFixed)
    fontPxSize=fixedFontPx;
  //Nelle fasi iniziali del programma fontPxSize è 0 e l'assegnazione di questo valore crea un messaggio d'errore nella finestra di uscita del programma. Il seguente if consente di evitarlo:
 if(fontPxSize>0){
   numFont. setPixelSize(fontPxSize);
   baseFont.setPixelSize(fontPxSize);
   expFont.setPixelSize(int(EXPFRACTION*fontPxSize));
   lgdFont.setPixelSize(fontPxSize);
  }
  QFontMetrics fm(baseFont);
  float fSmallHSpace=0.6f*fm.horizontalAdvance("a");
  smallHSpace=int(fSmallHSpace);
  if(fSmallHSpace<=2.0f)
    markHalfWidth=1.5f*fSmallHSpace;
  else
    markHalfWidth=qMax(1.5f*2.0f,1.00f*fSmallHSpace);

  numWidth=fm.horizontalAdvance("+0.000");
  textHeight=fm.height();
  maxTic=int(plotRect.height()/(1.2f*textHeight)/2.f)-1;
  minYTic=qMin(4,maxTic-3);
  maxTic=int(plotRect.width()/(1.2f*numWidth))-1;
  minXTic=qMin(4,maxTic-3);

}
void CLineChart::drawBars(void){
/* Routine per il tracciamento di diagrammi a barre.  Per il momento consente di tracciare diagrammi relativi ad un'unica grandezza
Pertanto l'indice di file è sempre 0 e l'indice della variabile è sempre 0.
*/
  int i,barLeft,barCenter,
          barY, //coordinata in pixel Y del lato orizzontale del rettangolo rappreentativo dell'ordinata da rappreentare con la barra
          barWidth,barRight,
          yZero; //Coordinata pixel Y della retta y=0 (scale lineari) e logy=0 (scale log)
  float minXStep, fAux;
  QBrush redBrush, grayBrush;

  plotPen.setColor(Qt::black);
  myPainter->setPen(plotPen);
  redBrush.setColor(Qt::red);
  redBrush.setStyle(Qt::SolidPattern);
  grayBrush.setColor(Qt::gray);
  grayBrush.setStyle(Qt::SolidPattern);
  myPainter->setBrush(redBrush);
  if(yAxis.scaleType==stLin)
    yZero=Y1+int(yAxis.minF*yAxis.pixPerValue );
  else
    yZero=Y1+int(yAxis.eMin *yAxis.pixPerValue );

  if(xAxis.scaleType==stLin){
     xStartIndex[0]=X0+margin + NearInt((px[0][startIndex[0]]-xAxis.minF)*xAxis.pixPerValue );
     xStopIndex[0] =X0+margin + NearInt((px[0][stopIndex[0]] -xAxis.minF)*xAxis.pixPerValue );
     minXStep=px[0][stopIndex[0]]-px[0][startIndex[0]];
     for(i=startIndex[0]+1; i<=stopIndex[0]; i++)
       if(px[0][i]-px[0][i-1]<minXStep) minXStep=px[0][i]-px[0][i-1];
//     barWidth=max(2,0.7*(X1-X0)*minXStep/(px[0][stopIndex[0]]-px[0][startIndex[0]]));
     barWidth=qMax(2,int(0.7f*(X1-X0)*minXStep/(px[0][stopIndex[0]]-px[0][startIndex[0]])));
  }else{
    xStartIndex[0]=X0+margin + NearInt(log10f(px[0][startIndex[0]]-xAxis.eMin)*xAxis.pixPerValue );
    xStopIndex[0] =X0+margin + NearInt(log10f(px[0][stopIndex[0]] -xAxis.eMin)*xAxis.pixPerValue );
    minXStep=log10f(px[0][stopIndex[0]]/px[0][startIndex[0]]);
    for(i=startIndex[0]+1; i<=stopIndex[0]; i++){
      fAux=log10f(px[0][i]/px[0][i-1]);
      if(fAux<minXStep) minXStep=fAux;
    }
    barWidth=qMax(2,int(0.7f*(X1-X0)*minXStep/(log10f(px[0][stopIndex[0]]/px[0][startIndex[0]]))));
  }
  // Nel caso in cui il numero di barre è molto modesto, la loro larghezza può risultare eccessiva rispetto al font usato per tracciare i numeri sugli assi. Faccio la correzione:
  if (barWidth>1.5f*generalFontPx)
      barWidth=int(1.5f*generalFontPx);
// Se le barre sono larghe, la larghezza del cursore va aumentata rispetto al valore default:
  dataCurs.setWidth(qMax(dataCurs.width(),barWidth/2));
  // per centrare sempre il cursore dentro una bar, la differenza fra la larghezza della barra e quella del cursore dev'essere divisibile per due, ma comunque mai inferiore ad uno:
  if((barWidth-dataCurs.width())/2*2!= barWidth-dataCurs.width() )
      dataCurs.setWidth(qMax(dataCurs.width()-1,1));

  // ***NOTA. In analogia con dataCurs.width() la barWidth è calcolata includendo primo e ultimo pixel. Se ad es. un dataCurs ha x1=135 e x2=137  la sua width è 3.
  plotPen.setColor(Qt::black);
  //Tracciamento barre:
  for(i=startIndex[0]; i<=stopIndex[0]; i++){
    myPainter->setBrush(redBrush);
    if(yAxis.scaleType==stLin){
       barY=int(Y1-(py[0][0][i]-yAxis.minF)*yAxis.pixPerValue );
    }else{
       barY=int(Y1-log10f(py[0][0][i]/yAxis.scaleMin)*yAxis.pixPerValue );
    }
    if(xAxis.scaleType==stLin){
       fAux=px[0][i] - xAxis.minF;
    }else{
      fAux=log10f(px[0][i]) - xAxis.eMin;
    }
    if(fAux<0)fAux=0;
    barCenter=X0+margin + NearInt(fAux*xAxis.pixPerValue );
    barLeft=int(barCenter-barWidth/2.f+0.5f);
    barLeft=qMax(X0,barLeft);
    barRight=barLeft+barWidth-1;
    //La seguente condizione non dovrebbe mai accadere. Siccome però accade e per ora non ho il tempo di vedere perché, la gestisco in modo semplice:
    if(barLeft>=X1)continue;
    barRight=qMin(X1,barRight);
    if(zoomed){
      if(barY>Y1){
        myPainter->setBrush(grayBrush);
        if(yZero<Y0)
          myPainter->drawRect(barLeft,Y0,barRight-barLeft,Y1-Y0+1);
        else if (yZero>Y1)
          ;
        else
          myPainter->drawRect(barLeft,yZero,barRight-barLeft,Y1-yZero+1);
        continue;
      }else if (barY<Y0){
        myPainter->setBrush(grayBrush);
         if(yZero<Y0)
           ;
         else if (yZero>Y1)
           myPainter->drawRect(barLeft,Y0,barRight-barLeft,Y1-Y0+1);
         else
           myPainter->drawRect(barLeft,Y0,barRight-barLeft,yZero-Y0+1);
         continue;
      }
    }
    if(yZero<Y0)
      myPainter->drawRect(barLeft,Y0,barRight-barLeft,barY-Y0);
    else if (yZero>Y1)
      myPainter->drawRect(barLeft,Y1,barRight-barLeft,barY-Y1);
    else
      // E' stato visto che se larghezza o altezza del rettangolo sono negativi, il rettangolo viene sì tracciato, ma l'interno deborda di un pixel, con uno sgradevole effetto di imprecisione. Pertanto nel caso di altezze negative devo manualmente rigirare il rettangolo:
      if(barY-yZero>1){ //se la differenza è 0 o 1 non traccio nulla
        myPainter->drawRect(barLeft,yZero,barRight-barLeft,barY-yZero);
      }else if (barY-yZero<-1){  //se la differenza è 0 o 1 non traccio nulla
        myPainter->drawRect(barLeft,barY,barRight-barLeft,yZero-barY);
    }
  }

  //Piazzo il cursore in corrispondenza della barra più vicina alla metà del grafico:
  /*
  i=(startIndex[0]+stopIndex[0])/2.+0.5;
  static SXYValues values;
  int rWidth=dataRect.width(); //va calcolata volta per volta perché nel caso di ptBar la larghezza è calcolata dinamicamente
  int nearX;
  values=giveValues(px[0][i], FLinearInterpolation, nearX, false, false);
  if(nearX<X0)nearX=X0;
  if(nearX>X1) nearX=X1;
  dataRect.moveLeft(nearX-rWidth/2);
  //Mando fuori i valori:
  emit valuesChanged(values,false,false);
*/
}


int CLineChart::drawCurves(bool noCurves){
 /* Funzione per il tracciamento delle curve su grafici di linea.  Contiene al suo interno
  *  un algoritmo per l'eliminazione automatica dei punti visualmente ridondanti e del
  *  taglio delle curve all'esterno del rettangolo di visualizzazione.
  *  Essendo stata realizzata con grande cura ed essendo intrinsecamente complessa è
  *  sconsigliato modificarla se non strettamente necessario.
*/
  int iTotPlot=-1;
  int pointsDrawn0;
  float   sxmin, //valore corrispondente al lato sinistro dell'asse x
          symin, //valore corrispondente al lato basso dell'asse y
          xf, yf, x1f, y1f, yRatio;
  float x,y,x1,y1; //valori arrotondati di xf, yf, x1f, y1f
  FC.getRect(X0,Y0,X1,Y1);
  if(xAxis.scaleType==stLin)
    sxmin=xAxis.minF;
  else
    sxmin=xAxis.eMin;
  pointsDrawn=0;
  if(xAxis.scaleType==stLin){
    for(int i=0; i<nFiles; i++){
      xStartIndex[i]=X0+NearInt(xAxis.pixPerValue  * (px[i][startIndex[i]] - sxmin));
      xStopIndex[i] =X0+NearInt(xAxis.pixPerValue  * (px[i][stopIndex[i]] - sxmin));
    }
  }else{
    for(int i=0; i<nFiles; i++){
      xStartIndex[i]=X0+ NearInt((log10f(px[i][startIndex[i]])-xAxis.eMin)*xAxis.pixPerValue );
      xStopIndex[i] =X0+ NearInt((log10f(px[i][stopIndex[i]]) -xAxis.eMin)*xAxis.pixPerValue );
    }
  }
  if(noCurves)
    return 0;
  for(int iFile=0; iFile<nFiles; iFile++){
    for(int iPlot=0; iPlot<nPlots[iFile]; iPlot++)	{
      bool wasInRect=false;
      iTotPlot++;
      QPainterPath path;
      if(blackWhite)
        plotPen.setColor(Qt::black);
      else
        plotPen.setColor(curveParamLst[iTotPlot].color);
      plotPen.setStyle(curveParamLst[iTotPlot].style);
      myPainter->setPen(plotPen);
      // Calcolo yRatio e symin, valutando se sono relativi alla scala di sinistra o a quella eventuale di destra:
      if(curveParamLst[iTotPlot].rightScale){
        yRatio=ryAxis.pixPerValue ;
        if(yAxis.scaleType==stLin)
          symin=ryAxis.minF;
        else
          symin=ryAxis.eMin;
      }else{
        yRatio=yAxis.pixPerValue ;
        if(yAxis.scaleType==stLin)
          symin=yAxis.minF;
        else
          symin=yAxis.eMin;
      }
      CFilterClip::FloatPoint P1,P2;
      pointsDrawn0=0;
      if(xAxis.scaleType==stLin)
        x1f=xAxis.pixPerValue  * (px[iFile][startIndex[iFile]]-sxmin)+X0;
      else
        x1f=xAxis.pixPerValue  * (log10f(px[iFile][startIndex[iFile]])-sxmin)+X0;
      if(yAxis.scaleType==stLin)
        y1f=yAxis.widthPix-yRatio * (py[iFile][iPlot][startIndex[iFile]]-symin)+Y0;
      else
        y1f=yAxis.widthPix-yRatio * (log10f(py[iFile][iPlot][startIndex[iFile]])-symin)+Y0;
      x1=NearInt(x1f);
      y1=NearInt(y1f);
      if(FC.isInRect(x1,y1))
          wasInRect=true;
      // Se wasInRect=false, il primo punto da graficare sarà l'intersezione col rettangolo del primo e secondo punto.
      // Siccome finora (20/11/15) quest'intersezione non è stata mai cercata e il comportamento di CLineChart è risultato accettabile, aggiungo una modifica al codice usato finora solo nel caso in cui il punto successivo è nel rettangolo.
      if(!wasInRect){
        int startIndexPlus=startIndex[iFile]+1;
        //Le seguenti due righe sono state commentate il 3/4/2018 e sostituita con l'if. Si tratta di righe "core", quindi la correzione va accuratamente validata. Sembra però che avessero ben due problemi:
        // 1) usavano entrambe la formula lineare anche in caso di scale logaritmiche
        // 2) la seconda usava startIndex[iFile] invece di startIndexPlus
//        float xfplus=xAxis.pixPerValue  * (px[iFile][startIndexPlus]-sxmin)+X0;
//        float yfplus=yAxis.width-yRatio * (py[iFile][iPlot][startIndex[iFile]]-symin)+Y0;

        float xfplus, yfplus;
//        qDebug()<<"px[iFile][startIndexPlus]: "<<px[iFile][startIndexPlus];
        if(xAxis.scaleType==stLin)
          xfplus=xAxis.pixPerValue  * (px[iFile][startIndexPlus]-sxmin)+X0;
        else
          xfplus=xAxis.pixPerValue  * (log10f(px[iFile][startIndexPlus])-sxmin)+X0;

        if(yAxis.scaleType==stLin)
          yfplus=yAxis.widthPix-yRatio * (py[iFile][iPlot][startIndexPlus]-symin)+Y0;
        else
          yfplus=yAxis.widthPix-yRatio * (log10f(py[iFile][iPlot][startIndexPlus])-symin)+Y0;

        int xPlus=NearInt(xfplus);
        int yPlus=NearInt(yfplus);
        CLineChart::CFilterClip::FloatPoint I1, I2;
        if(FC.isInRect(xPlus,yPlus)){
          FC.getLine(x1,y1,xPlus,yPlus);
          FC.giveRectIntersect(I1,I2);
          path.moveTo(NearInt(I1.X), NearInt(I1.Y));
        }
      }else{
        path.moveTo(double(x1),double(y1));
      }

      // Ora che ho posto il puntatore del path al primo punto, calcolo la retta congiungente il primo col secondo punto, per innescare il successivo loop per tutti i punti dal secondo in poi.
      int secondIndex=startIndex[iFile]+1;
      // il seguente check dovrebbe essere sempre passato in quanto la verifica che gli indici sono almeno due è fatta quando sono determinati startIndex[i] e stopIndex[i] in goPlot(). Comunque male non fa:
      if(secondIndex>stopIndex[iFile])
        return 1;
      if(xAxis.scaleType==stLin)
        xf=xAxis.pixPerValue  * (px[iFile][secondIndex]-sxmin)+X0;
      else
        xf=xAxis.pixPerValue  * (log10f(px[iFile][secondIndex])-sxmin)+X0;
      if(yAxis.scaleType==stLin)
        yf=yAxis.widthPix-yRatio * (py[iFile][iPlot][secondIndex]-symin)+Y0;
      else
        yf=yAxis.widthPix-yRatio * (log10f(py[iFile][iPlot][secondIndex])-symin)+Y0;
      x=NearInt(xf);
      y=NearInt(yf);
      // Se i due punti coincidono la retta è indeterminata. Però non crea difficoltà al tracciamento in quanto FC gestisce internamente tale situazione.
      FC.getLine(x1,y1,x,y);

      //Grafico fino al penultimo punto, con filtraggio e "Clippaggio". L'ultimo punto
      //lo traccio fuori del loop per essere certo che venga comunque tracciato, anche
      //se è nel prolungamento della retta congiungente i due punti precedenti.
      //qDebug()<<"start: "<<startIndex[iFile]+1<<"  stop: "<<stopIndex[iFile];

      for(int iPoint=startIndex[iFile]+1; iPoint<stopIndex[iFile]; iPoint++)	{
        if(xAxis.scaleType==stLin)
          xf=xAxis.pixPerValue  * (px[iFile][iPoint] - sxmin) +X0;
        else
          xf=xAxis.pixPerValue  * (log10f(px[iFile][iPoint]) - sxmin) +X0;
        if(yAxis.scaleType==stLin)
          yf=yAxis.widthPix-yRatio*(py[iFile][iPlot][iPoint] - symin) +Y0;
        else
          yf=yAxis.widthPix-yRatio*(log10f(py[iFile][iPlot][iPoint]) - symin) +Y0;
        x=xf+0.5f;
        y=yf+0.5f;

        //Il seguente if, che serve per debuggare un problema lo fa scomparire! Il problema si osserva quando si traccia la variable del file Energie_Nied.adf, ma solo in release.
        if (x<X0)
          qDebug()<<"x, X0: "<<x<<X0;

        if(wasInRect){
          if(FC.isInRect(x,y)){ //Vecchio e nuovo punto dentro il rettangolo
            if(!FC.isRedundant(x,y)){
              FC.getLine(x1,y1,x,y);
              path.lineTo(qreal(x1),qreal(y1));
              pointsDrawn0++;
            }
          }else{ //Il vecchio punto era nel rettangolo il nuovo no
             wasInRect=false;
             FC.getLine(x1,y1,x,y);
             path.lineTo(qreal(x1),qreal(y1));
             if(FC.giveRectIntersect(P1,P2)==1)
                 path.lineTo(qreal(P1.X),qreal(P1.Y));
             pointsDrawn0++;
           }
         }else{ //Il punto precedente non era dentro il rettangolo
           if(FC.isInRect(x,y)){ //Il vecchio punto era fuori, il nuovo dentro il rettangolo
             FC.getLine(x1,y1,x,y);
             wasInRect=true;
             FC.giveRectIntersect(P1,P2);
             path.moveTo(qreal(P1.X),qreal(P1.Y));
             path.lineTo(qreal(x),qreal(y));
             pointsDrawn0++;
           }else{ //Vecchio e nuovo punto fuori del rettangolo
             FC.getLine(x1,y1,x,y);
             if(FC.giveRectIntersect(P1,P2)){ //anche se i due punti sono fuori la loro congiungente interseca il rettangolo
               path.moveTo(qreal(P1.X),qreal(P1.Y));
               path.lineTo(qreal(P2.X),qreal(P2.Y));
               pointsDrawn0+=2;
            } //Fine ricerca prima intersezione
          } //Fine vecchio e nuovo punto fuori dal rettangolo
        } //Fine punto vecchio fuori dal rettangolo
        x1=x;
        y1=y;
      } //Fine ciclo for tracciamento curve
      // qDebug()<<"PointsDrawn"<<pointsDrawn0;
      //Tracciamento ultimo punto della curva:
      if(xAxis.scaleType==stLin)
        xf=xAxis.pixPerValue  * (px[iFile][stopIndex[iFile]] - sxmin) +X0;
      else
        xf=xAxis.pixPerValue  * (log10f(px[iFile][stopIndex[iFile]]) - sxmin) +X0;
      if(yAxis.scaleType==stLin)
        yf=yAxis.widthPix-yRatio*(py[iFile][iPlot][stopIndex[iFile]] - symin) +Y0;
      else
        yf=yAxis.widthPix-yRatio*(log10f(py[iFile][iPlot][stopIndex[iFile]]) - symin) +Y0;
      x=xf+0.5f;
      y=yf+0.5f;

      /**********************
/Per ragioni sconosciute talvolta l'ultimo punto non è tracciato, MA SOLO IN RELEASE.
Per questa ragione sono state aggiunte le seguenti righe qDebug() per cercare di tracciare il problema.
Ma con qDebug() il problema non si presenta nemmeno in relase mode!
Per ora pertanto si lascia il codice con queste righe, riducendone al minimo le funzioni, in attesa che prima o poi il vero problema venga evidenziato.
*/
      static int iWasInRect=0;
//      qDebug()<<"x,x1,y,y1:"<<x<<x1<<y<<y1;
      //L'ultimo punto lo traccio solo se il penultimo era nel rettangolo:
      if(wasInRect){
        iWasInRect++;
//        qDebug()<<"wasInRect, i:"<<wasInRect<<iWasInRect;
        if(FC.isInRect(x,y)){
          path.lineTo(qreal(x1),qreal(y1));
          path.lineTo(qreal(x),qreal(y));
        }else{
          FC.getLine(x1,y1,x,y);
          path.lineTo(qreal(x1),qreal(y1));
          if(FC.giveRectIntersect(P1,P2)==1)
            path.lineTo(qreal(P1.X),qreal(P1.Y));
          else if(FC.giveRectIntersect(P1,P2)==2)
            path.lineTo(qreal(P2.X),qreal(P2.Y));
        }
        pointsDrawn0++;
      }
      pointsDrawn=qMax(pointsDrawn,pointsDrawn0);
      QElapsedTimer timer;
      timer.start();

//      QFile file("pathdata.dat");
//      if(!file.open(QIODevice::WriteOnly))
//          return;
//      QDataStream out(&file);
//      out<<path;
//      file.close();
      if(makingSVG){
        myPainter->drawPath(path);
//        qDebug() << "drawpath operation took" << timer.elapsed() << "milliseconds";
      }else{
// Qui uso la sintassi che mi è stata suggerita da Samuel Rodal, ma è superflua l'iterazione fra i poligoni, visto che le mie curve sono composte tutte da un unico poligono. Notare l'uso di foreach(), estensione di Qt al C++ (significato accessibile via help).
        int i;
        qDebug()<<"r-g-b: "<<myPainter->pen().color().red()<< myPainter->pen().color().green()<<myPainter->pen().color().blue();
        qDebug()<<"width: "<<myPainter->pen().width();
        foreach(QPolygonF poly, path.toSubpathPolygons())
          for(i=0; i<poly.size()-1; i++)
            myPainter->drawLine(poly.at(i),poly.at(i+1));
//          qDebug() << "foreach operation took" << timer.elapsed() << "milliseconds";
      }
    } //Fine tracciamento varie curve relative ad un medesimo file
  } //Fine ciclo for fra i vari files
 return 0;
}

int CLineChart::drawCurvesD(bool noCurves){
 /* Funzione per il tracciamento delle curve su grafici di linea.  Contiene al suo interno
  * un algoritmo per l'eliminazione automatica dei punti visualmente ridondanti e del
  * taglio delle curve all'esterno del rettangolo di visualizzazione.
  * Essendo stata realizzata con grande cura ed essendo intrinsecamente complessa è
  * sconsigliato modificarla se non strettamente necessario.
*/
  int iTotPlot=-1;
  int pointsDrawn0;
  float   sxmin, //valore corrispondente al lato sinistro dell'asse x
          symin, //valore corrispondente al lato basso dell'asse y
          yRatio;
  double   xf, yf, x1f, y1f;
  double  x,y,x1,y1; //valori arrotondati di xf, yf, x1f, y1f
  FCd.getRect(X0,Y0,X1,Y1);
  if(xAxis.scaleType==stLin)
    sxmin=xAxis.minF;
  else
    sxmin=xAxis.eMin;
  pointsDrawn=0;
  if(xAxis.scaleType==stLin){
    for(int i=0; i<nFiles; i++){
      xStartIndex[i]=X0+NearInt(xAxis.pixPerValue  * (px[i][startIndex[i]] - sxmin));
      xStopIndex[i] =X0+NearInt(xAxis.pixPerValue  * (px[i][stopIndex[i]] - sxmin));
    }
  }else{
    for(int i=0; i<nFiles; i++){
      xStartIndex[i]=X0+ NearInt((log10f(px[i][startIndex[i]])-xAxis.eMin)*xAxis.pixPerValue );
      xStopIndex[i] =X0+ NearInt((log10f(px[i][stopIndex[i]]) -xAxis.eMin)*xAxis.pixPerValue );
    }
  }
  if(noCurves)
    return 0;
  for(int iFile=0; iFile<nFiles; iFile++){
    for(int iPlot=0; iPlot<nPlots[iFile]; iPlot++)	{
      bool wasInRect=false;
      iTotPlot++;
      QPainterPath path;
      if(blackWhite)
        plotPen.setColor(Qt::black);
      else
        plotPen.setColor(curveParamLst[iTotPlot].color);
      plotPen.setStyle(curveParamLst[iTotPlot].style);
      myPainter->setPen(plotPen);
      // Calcolo yRatio e symin, valutando se sono relativi alla scala di sinistra o a quella eventuale di destra:
      if(curveParamLst[iTotPlot].rightScale){
        yRatio=ryAxis.pixPerValue ;
        if(yAxis.scaleType==stLin)
          symin=ryAxis.minF;
        else
          symin=ryAxis.eMin;
      }else{
        yRatio=yAxis.pixPerValue ;
        if(yAxis.scaleType==stLin)
          symin=yAxis.minF;
        else
          symin=yAxis.eMin;
      }
      CFilterClipD::DoublePoint P1,P2;
      pointsDrawn0=0;
      if(xAxis.scaleType==stLin)
        x1f=double(xAxis.pixPerValue  * (px[iFile][startIndex[iFile]]-sxmin)+X0);
      else
        x1f=double(xAxis.pixPerValue  * (log10f(px[iFile][startIndex[iFile]])-sxmin)+X0);
      if(yAxis.scaleType==stLin)
        y1f=double(yAxis.widthPix-yRatio * (py[iFile][iPlot][startIndex[iFile]]-symin)+Y0);
      else
        y1f=double(yAxis.widthPix-yRatio * (log10f(py[iFile][iPlot][startIndex[iFile]])-symin)+Y0);
      x1=NearIntD(x1f);
      y1=NearIntD(y1f);
      if(FCd.isInRect(x1,y1))
          wasInRect=true;
      // Se wasInRect=false, il primo punto da graficare sarà l'intersezione col rettangolo del primo e secondo punto.
      // Siccome finora (20/11/15) quest'intersezione non è stata mai cercata e il comportamento di CLineChart è risultato accettabile, aggiungo una modifica al codice usato finora solo nel caso in cui il punto successivo è nel rettangolo.
      if(!wasInRect){
        int startIndexPlus=startIndex[iFile]+1;
        //Le seguenti due righe sono state commentate il 3/4/2018 e sostituita con l'if. Si tratta di righe "core", quindi la correzione va accuratamente validata. Sembra però che avessero ben due problemi:
        // 1) usavano entrambe la formula lineare anche in caso di scale logaritmiche
        // 2) la seconda usava startIndex[iFile] invece di startIndexPlus
//        float xfplus=xAxis.pixPerValue  * (px[iFile][startIndexPlus]-sxmin)+X0;
//        float yfplus=yAxis.width-yRatio * (py[iFile][iPlot][startIndex[iFile]]-symin)+Y0;

        float xfplus, yfplus;
        qDebug()<<"px[iFile][startIndexPlus]: "<<px[iFile][startIndexPlus];
        if(xAxis.scaleType==stLin)
          xfplus=xAxis.pixPerValue  * (px[iFile][startIndexPlus]-sxmin)+X0;
        else
          xfplus=xAxis.pixPerValue  * (log10f(px[iFile][startIndexPlus])-sxmin)+X0;

        if(yAxis.scaleType==stLin)
          yfplus=yAxis.widthPix-yRatio * (py[iFile][iPlot][startIndexPlus]-symin)+Y0;
        else
          yfplus=yAxis.widthPix-yRatio * (log10f(py[iFile][iPlot][startIndexPlus])-symin)+Y0;

        int xPlus=NearInt(xfplus);
        int yPlus=NearInt(yfplus);
        CLineChart::CFilterClipD::DoublePoint I1, I2;
        if(FCd.isInRect(xPlus,yPlus)){
          FCd.getLine(x1,y1,xPlus,yPlus);
          FCd.giveRectIntersect(I1,I2);
          path.moveTo(NearIntD(I1.X), NearIntD(I1.Y));
        }
      }else{
        path.moveTo(x1,y1);
      }

      // Ora che ho posto il puntatore del path al primo punto, calcolo la retta congiungente il primo col secondo punto, per innescare il successivo loop per tutti i punti dal secondo in poi.
      int secondIndex=startIndex[iFile]+1;
      // il seguente check dovrebbe essere sempre passato in quanto la verifica che gli indici sono almeno due è fatta quando sono determinati startIndex[i] e stopIndex[i] in goPlot(). Comunque male non fa:
      if(secondIndex>stopIndex[iFile])
        return 1;
      if(xAxis.scaleType==stLin)
        xf=double(xAxis.pixPerValue  * (px[iFile][secondIndex]-sxmin)+X0);
      else
        xf=double(xAxis.pixPerValue  * (log10f(px[iFile][secondIndex])-sxmin)+X0);
      if(yAxis.scaleType==stLin)
        yf=double(yAxis.widthPix-yRatio * (py[iFile][iPlot][secondIndex]-symin)+Y0);
      else
        yf=double(yAxis.widthPix-yRatio * (log10f(py[iFile][iPlot][secondIndex])-symin)+Y0);
      x=NearIntD(xf);
      y=NearIntD(yf);
      // Se i due punti coincidono la retta è indeterminata. Però non crea difficoltà al tracciamento in quanto FCd gestisce internamente tale situazione.
      FCd.getLine(x1,y1,x,y);
//      bool lineDefined=FCd.getLine(x1,y1,x,y);
//      qDebug()<<"x1: "<<x1<<"  y1: "<<y1<<"  x:"<<x<<"  y: "<<y;
//      qDebug()<<"lineDefined: "<<lineDefined;

      //Grafico fino al penultimo punto, con filtraggio e "Clippaggio". L'ultimo punto
      //lo traccio fuori del loop per essere certo che venga comunque tracciato, anche
      //se è nel prolungamento della retta congiungente i due punti precedenti.
      //qDebug()<<"start: "<<startIndex[iFile]+1<<"  stop: "<<stopIndex[iFile];

      for(int iPoint=startIndex[iFile]+1; iPoint<stopIndex[iFile]; iPoint++)	{
        if(xAxis.scaleType==stLin)
          xf=double(xAxis.pixPerValue  * (px[iFile][iPoint] - sxmin) +X0);
        else
          xf=double(xAxis.pixPerValue  * (log10f(px[iFile][iPoint]) - sxmin) +X0);
        if(yAxis.scaleType==stLin)
          yf=double(yAxis.widthPix-yRatio*(py[iFile][iPlot][iPoint] - symin) +Y0);
        else
          yf=double(yAxis.widthPix-yRatio*(log10f(py[iFile][iPlot][iPoint]) - symin) +Y0);
        x=xf+0.5;
        y=yf+0.5;

        //Il seguente if, che serve per debuggare un problema lo fa scomparire! Il problema si osserva quando si traccia la variable del file Energie_Nied.adf, ma solo in release.
        if (x<X0)
          qDebug()<<"x, X0: "<<x<<X0;

        if(wasInRect){
          if(FCd.isInRect(x,y)){ //Vecchio e nuovo punto dentro il rettangolo
            if(!FCd.isRedundant(x,y)){
              FCd.getLine(x1,y1,x,y);
              path.lineTo(x1,y1);
              pointsDrawn0++;
            }
          }else{ //Il vecchio punto era nel rettangolo il nuovo no
             wasInRect=false;
             FCd.getLine(x1,y1,x,y);
             path.lineTo(x1,y1);
             if(FCd.giveRectIntersect(P1,P2)==1)
                 path.lineTo(P1.X,P1.Y);
             pointsDrawn0++;
           }
         }else{ //Il punto precedente non era dentro il rettangolo
           if(FCd.isInRect(x,y)){ //Il vecchio punto era fuori, il nuovo dentro il rettangolo
             FCd.getLine(x1,y1,x,y);
             wasInRect=true;
             FCd.giveRectIntersect(P1,P2);
             path.moveTo(P1.X,P1.Y);
             path.lineTo(x,y);
             pointsDrawn0++;
           }else{ //Vecchio e nuovo punto fuori del rettangolo
             FCd.getLine(x1,y1,x,y);
             if(FCd.giveRectIntersect(P1,P2)){ //anche se i due punti sono fuori la loro congiungente interseca il rettangolo
               path.moveTo(P1.X,P1.Y);
               path.lineTo(P2.X,P2.Y);
               pointsDrawn0+=2;
            } //Fine ricerca prima intersezione
          } //Fine vecchio e nuovo punto fuori dal rettangolo
        } //Fine punto vecchio fuori dal rettangolo
        x1=x;
        y1=y;
      } //Fine ciclo for tracciamento curve
      qDebug()<<"PointsDrawn"<<pointsDrawn0;
      //Tracciamento ultimo punto della curva:
      if(xAxis.scaleType==stLin)
        xf=double(xAxis.pixPerValue  * (px[iFile][stopIndex[iFile]] - sxmin) +X0);
      else
        xf=double(xAxis.pixPerValue  * (log10f(px[iFile][stopIndex[iFile]]) - sxmin) +X0);
      if(yAxis.scaleType==stLin)
        yf=double(yAxis.widthPix-yRatio*(py[iFile][iPlot][stopIndex[iFile]] - symin) +Y0);
      else
        yf=double(yAxis.widthPix-yRatio*(log10f(py[iFile][iPlot][stopIndex[iFile]]) - symin) +Y0);
      x=xf+0.5;
      y=yf+0.5;

      /**********************
/Per ragioni sconosciute talvolta l'ultimo punto non è tracciato, MA SOLO IN RELEASE.
Per questa ragione sono state aggiunte le seguenti righe qDebug() per cercare di tracciare il problema.
Ma con qDebug() il problema non si presenta nemmeno in relase mode!
Per ora pertanto si lascia il codice con queste righe, riducendone al minimo le funzioni, in attesa che prima o poi il vero problema venga evidenziato.
*/
      static int iWasInRect=0;
//      qDebug()<<"x,x1,y,y1:"<<x<<x1<<y<<y1;
      //L'ultimo punto lo traccio solo se il penultimo era nel rettangolo:
      if(wasInRect){
        iWasInRect++;
        if(FCd.isInRect(x,y)){
          path.lineTo(x1,y1);
          path.lineTo(x,y);
        }else{
          FCd.getLine(x1,y1,x,y);
          path.lineTo(x1,y1);
          if(FCd.giveRectIntersect(P1,P2)==1)
            path.lineTo(P1.X,P1.Y);
          else if(FCd.giveRectIntersect(P1,P2)==2)
            path.lineTo(P2.X,P2.Y);
        }
        pointsDrawn0++;
      }
      pointsDrawn=qMax(pointsDrawn,pointsDrawn0);
      QElapsedTimer timer;
      timer.start();

//      QFile file("pathdata.dat");
//      if(!file.open(QIODevice::WriteOnly))
//          return;
//      QDataStream out(&file);
//      out<<path;
//      file.close();
      if(makingSVG){
          myPainter->drawPath(path);
//          qDebug() << "drawpath operation took" << timer.elapsed() << "milliseconds";
      }else{
// Qui uso la sintassi che mi è stata suggerita da Samuel Rodal, ma è superflua l'iterazione fra i poligoni, visto che le mie curve sono composte tutte da un unico poligono. Notare l'uso di foreach(), estensione di Qt al C++ (significato accessibile via help).
          foreach(QPolygonF poly, path.toSubpathPolygons())
              for(int i=0; i<poly.size()-1; i++)
                  myPainter->drawLine(poly.at(i),poly.at(i+1));
      }
    } //Fine tracciamento varie curve relative a un medesimo file
  } //Fine ciclo for fra i vari files
 return 0;
}



void CLineChart::drawCurvesPoly(bool NoCurves){
/*Funzione per il traccamento di curve su grafici di linea.
A differenza delle altre versioni di "drawCurves" fa uso di QPolygon invece di QPath.
Questo in via sperimentale in quanto si è visto che tutte le versioni di questa funzione che fanno uso di drawPath hanno un problema: in taluni casi non tracciano uno dei segmenti di curva previsti. Il fatto è ben visibile con la curva vDcloadDcmeno del file "rad2.mat"
Naturalmente rinunciare a drawpath ha i seguenti inconvenienti:
1- quando si esporta il grafico come SVG il grafico non è oggetto unico; invece, ognuno dei segmenti costituenti lo è
2- ci si aspetta una minore efficenza, ma questo è da verificare.
La presente funzione serve quindi a verificare i cambiamenti di efficienza che si hanno se si elimina il drawPath; se essi sono trascurabili e scompare l'errore di visualizzazione sopra citato, si potrebbe usare all'uso di drawPath almeno nella visualizzazione a schermo, magari lascandolo invece attivo nella scrittura su svg.
*/

    int i, iPlot=0, icount, igraf, iTotPlot=-1;
    int pointsDrawn0;
    float sxmin, symin, xf, yf, x1f, y1f, yRatio;
    QPolygon poly;
    if(xAxis.scaleType==stLin)
      sxmin=xAxis.minF;
    else
      sxmin=xAxis.eMin;
    pointsDrawn=0;
    for(i=0; i<nFiles; i++){
      xStartIndex[i]=NearInt(xAxis.pixPerValue  * (px[i][startIndex[i]] - sxmin))+X0;
      xStopIndex[i]=NearInt(xAxis.pixPerValue  * (px[i][stopIndex[i]] - sxmin))+X0;
    }
    if(NoCurves)return;
    for(i=0; i<nFiles; i++){
      for(igraf=0; igraf<nPlots[i]; igraf++)	{
        if(blackWhite)
          plotPen.setColor(Qt::black);
        else
          plotPen.setColor(curveParamLst[igraf].color);
        myPainter->setPen(plotPen);
        iTotPlot++;
        // Calcolo YRatio e symin, valutando se sono relativi alla scala di sinistra o
        //a quella eventuale di destra:
        if(curveParamLst[iTotPlot].rightScale){
          yRatio=ryAxis.pixPerValue ;
          if(yAxis.scaleType==stLin)
            symin=ryAxis.minF;
          else
            symin=ryAxis.eMin;
        }else{
          yRatio=yAxis.pixPerValue ;
          if(yAxis.scaleType==stLin)
          symin=yAxis.minF;
        else
          symin=yAxis.eMin;
        }
        pointsDrawn0=0;
        if(xAxis.scaleType==stLin)
          x1f=xAxis.pixPerValue  * (px[i][startIndex[i]]-sxmin)+X0;
        else
          x1f=xAxis.pixPerValue  * (log10f(px[i][startIndex[i]])-sxmin)+X0;
        if(yAxis.scaleType==stLin)
          y1f=yAxis.widthPix-yRatio * (py[i][igraf][startIndex[i]]-symin)+Y0;
        else
          y1f=yAxis.widthPix-yRatio * (log10f(py[i][igraf][startIndex[i]])-symin)+Y0;
        poly << QPoint(int(x1f),int(y1f));
        //Tracciamento grafico
        for(icount=startIndex[i]+1; icount<=stopIndex[i]; icount++)	{
          if(xAxis.scaleType==stLin)
            xf=xAxis.pixPerValue  * (px[i][icount] - sxmin) +X0;
          else
            xf=xAxis.pixPerValue  * (log10f(px[i][icount]) - sxmin) +X0;
          if(yAxis.scaleType==stLin)
            yf=yAxis.widthPix-yRatio*(py[i][igraf][icount] - symin) +Y0;
          else
            yf=yAxis.widthPix-yRatio*(log10f(py[i][igraf][icount]) - symin) +Y0;
          poly << QPoint(int(xf),int(yf));
          pointsDrawn0++;
        } //Fine ciclo for tracciamento curve
        pointsDrawn=qMax(pointsDrawn,pointsDrawn0);
        iPlot++;
      } //Fine tracciamento varie curve relative ad un medesimo file
    } //Fine ciclo for fra i vari files
  //  Return:
    myPainter->drawPolyline(poly);


}

void CLineChart::drawCurvesQtF(bool NoCurves){
/* Funzione per il tracciamento delle curve su grafici di linea.
Nelle versioni QtI e QtF è stato soppresso il fltraggio in quanto si presume che per velocizzare sia sufficiente la separazione fra preparazione del path e successiva generazione della Pixmap a partire da esso.
I test effettuati hanno in effetti mostrato che con QtF si hanno tempi eccessivi, mentre con QtI  i tempi sono paragonabili a quelli ottenibili FC (cioè la mia versione di DrawCurves che fa uso di FIlterClip) mentre con il BCB senza filterClip i tempi erano enormemente superiori.
   Ecco un resoconto in cui si sono mediati i risultati ottenuti in tutti i casi con più prove consecutive effettuate sul medesimo PC:
   Punti      100 000    100 000
   Linea       thin       thick
   FC/ms         8          8
   QtF/ms       15         95
   QtI/ms        8          9
Occorre ricordare che  al momento non è chiaro come si comportano le due funzioni quando viene effettuata l'operazione di Clip.
Si osserva che Qti opera SENZA simplified(), in quanto se metto simplified() i tempi si allungano e il grafico viene richiuso da una linea che invece non dovrebbe esserci.
L'unica differenza fra QtF e QtI sta nella linea "lineTo", la quint'ultima di codice, che nel caso di QtF traccia fra valori float, mentre QtI traccia fra valori preventivamente convertiti da float a int.
   */

  int i, iPlot=0, icount, igraf, iTotPlot=-1;
  int pointsDrawn0;
  float sxmin, symin, xf, yf, x1f, y1f, yRatio;
  QPainterPath path;
  if(xAxis.scaleType==stLin)
    sxmin=xAxis.minF;
  else
    sxmin=xAxis.eMin;
  pointsDrawn=0;
  for(i=0; i<nFiles; i++){
    xStartIndex[i]=NearInt(xAxis.pixPerValue  * (px[i][startIndex[i]] - sxmin))+X0;
    xStopIndex[i]=NearInt(xAxis.pixPerValue  * (px[i][stopIndex[i]] - sxmin))+X0;
  }
  if(NoCurves)return;
  for(i=0; i<nFiles; i++){
    for(igraf=0; igraf<nPlots[i]; igraf++)	{
      if(blackWhite)
        plotPen.setColor(Qt::black);
      else
        plotPen.setColor(curveParamLst[igraf].color);
      myPainter->setPen(plotPen);
      iTotPlot++;
      // Calcolo YRatio e symin, valutando se sono relativi alla scala di sinistra o
      //a quella eventuale di destra:
      if(curveParamLst[iTotPlot].rightScale){
        yRatio=ryAxis.pixPerValue ;
        if(yAxis.scaleType==stLin)
          symin=ryAxis.minF;
        else
          symin=ryAxis.eMin;
      }else{
        yRatio=yAxis.pixPerValue ;
        if(yAxis.scaleType==stLin)
        symin=yAxis.minF;
      else
        symin=yAxis.eMin;
      }
      pointsDrawn0=0;
      if(xAxis.scaleType==stLin)
        x1f=xAxis.pixPerValue  * (px[i][startIndex[i]]-sxmin)+X0;
      else
        x1f=xAxis.pixPerValue  * (log10f(px[i][startIndex[i]])-sxmin)+X0;
      if(yAxis.scaleType==stLin)
        y1f=yAxis.widthPix-yRatio * (py[i][igraf][startIndex[i]]-symin)+Y0;
      else
        y1f=yAxis.widthPix-yRatio * (log10f(py[i][igraf][startIndex[i]])-symin)+Y0;
      path.moveTo(double(x1f),double(y1f));
      //Tracciamento grafico
      for(icount=startIndex[i]+1; icount<=stopIndex[i]; icount++)	{
        if(xAxis.scaleType==stLin)
          xf=xAxis.pixPerValue  * (px[i][icount] - sxmin) +X0;
        else
          xf=xAxis.pixPerValue  * (log10f(px[i][icount]) - sxmin) +X0;
        if(yAxis.scaleType==stLin)
          yf=yAxis.widthPix-yRatio*(py[i][igraf][icount] - symin) +Y0;
        else
          yf=yAxis.widthPix-yRatio*(log10f(py[i][igraf][icount]) - symin) +Y0;
        path.lineTo(double(xf),double(yf));
        pointsDrawn0++;
      } //Fine ciclo for tracciamento curve
      pointsDrawn=qMax(pointsDrawn,pointsDrawn0);
      iPlot++;
    } //Fine tracciamento varie curve relative ad un medesimo file
  } //Fine ciclo for fra i vari files
//  Return:
  myPainter->drawPath(path);
}

void CLineChart::drawCurvesQtI(bool NoCurves){
 /* Funzione per il tracciamento delle curve su grafici di linea.
Per la spiegazione vedere il commento alla funzione drawCurvesQtF.
*/

  int i, iPlot=0, icount, igraf, iTotPlot=-1;
  int PointsDrawn0;
  float sxmin, symin, xf, yf, x1f, y1f, YRatio;
  QPainterPath path;
  if(xAxis.scaleType==stLin)
    sxmin=xAxis.minF;
  else
    sxmin=xAxis.eMin;
  pointsDrawn=0;
  for(i=0; i<nFiles; i++){
    xStartIndex[i]=NearInt(xAxis.pixPerValue  * (px[i][startIndex[i]] - sxmin))+X0;
    xStopIndex[i]=NearInt(xAxis.pixPerValue  * (px[i][stopIndex[i]] - sxmin))+X0;
  }
  if(NoCurves)return;
  for(i=0; i<nFiles; i++){
    for(igraf=0; igraf<nPlots[i]; igraf++)	{
      if(blackWhite)
        plotPen.setColor(Qt::black);
      else
        plotPen.setColor(curveParamLst[igraf].color);
      myPainter->setPen(plotPen);
      iTotPlot++;
      // Calcolo YRatio e symin, valutando se sono relativi alla scala di sinistra o
      //a quella eventuale di destra:
      if(curveParamLst[iTotPlot].rightScale){
        YRatio=ryAxis.pixPerValue ;
        if(yAxis.scaleType==stLin)
          symin=ryAxis.minF;
        else
          symin=ryAxis.eMin;
      }else{
        YRatio=yAxis.pixPerValue ;
        if(yAxis.scaleType==stLin)
        symin=yAxis.minF;
      else
        symin=yAxis.eMin;
      }
      PointsDrawn0=0;
      if(xAxis.scaleType==stLin)
        x1f=xAxis.pixPerValue  * (px[i][startIndex[i]]-sxmin)+X0;
      else
        x1f=xAxis.pixPerValue  * (log10f(px[i][startIndex[i]])-sxmin)+X0;
      if(yAxis.scaleType==stLin)
        y1f=yAxis.widthPix-YRatio * (py[i][igraf][startIndex[i]]-symin)+Y0;
      else
        y1f=yAxis.widthPix-YRatio * (log10f(py[i][igraf][startIndex[i]])-symin)+Y0;
      path.moveTo(double(x1f),double(y1f));
      //Tracciamento grafico
      for(icount=startIndex[i]+1; icount<=stopIndex[i]; icount++)	{
        if(xAxis.scaleType==stLin)
          xf=xAxis.pixPerValue  * (px[i][icount] - sxmin) +X0;
        else
          xf=xAxis.pixPerValue  * (log10f(px[i][icount]) - sxmin) +X0;
        if(yAxis.scaleType==stLin)
          yf=yAxis.widthPix-YRatio*(py[i][igraf][icount] - symin) +Y0;
        else
          yf=yAxis.widthPix-YRatio*(log10f(py[i][igraf][icount]) - symin) +Y0;
        path.lineTo(double(xf),double(yf));
        PointsDrawn0++;
      } //Fine ciclo for tracciamento curve
      pointsDrawn=qMax(pointsDrawn,PointsDrawn0);
      iPlot++;
    } //Fine tracciamento varie curve relative ad un medesimo file
  } //Fine ciclo for fra i vari files
//  Return:
  myPainter->drawPath(path);
}

void  CLineChart::drawMark(float X, float Y, int mark, bool markName){
  /* Funzione per il tracciamento dei vari simboli identificativi delle varie curve.
   X e Y sono le coordinate del centro; larghezza e altezza sono "markWidth/2".
    Se markName è true sto marchiando i nomi delle variabili e non devo fare il check per vedere se il punto passato è dentro il rettangolo del grafico o meno
  */
  QPainterPath path;
  QPolygon polygon;
  myPainter->setPen(Qt::black);
  if(!markName && (X<X0 || X>X1 || Y<Y0 || Y>Y1))return;
  switch(mark){
    case 0:
      //Cerchietto:
      myPainter->drawEllipse(int(X-markHalfWidth),int(Y-markHalfWidth),
                             int(2*markHalfWidth),int(2*markHalfWidth));
      break;
    case 1:
      //Quadratino:
      myPainter->drawRect(int(X-markHalfWidth),int(Y-markHalfWidth),
                          int(2*markHalfWidth),int(2*markHalfWidth));
      break;
    case 2:
      //Triangolino:
      polygon<<QPoint(int(X-markHalfWidth),int(Y+markHalfWidth));
      polygon<<QPoint(int(X+markHalfWidth),int(Y+markHalfWidth));
      polygon<<QPoint(int(X)              ,int(Y-markHalfWidth));
      myPainter->drawPolygon(polygon);
      break;
    case 3:
       //Crocetta
       path.moveTo(int(X-markHalfWidth),int(Y-markHalfWidth));
       path.lineTo(int(X+markHalfWidth),int(Y+markHalfWidth));
       path.moveTo(int(X-markHalfWidth),int(Y+markHalfWidth));
       path.lineTo(int(X+markHalfWidth),int(Y-markHalfWidth));
       myPainter->drawPath(path);
       break;
    case 4:
       //Cerchietto pieno:
        myPainter->setBrush(Qt::black);
        myPainter->drawEllipse(int(X-markHalfWidth),int(Y-markHalfWidth),
                               int(2*markHalfWidth),int(2*markHalfWidth));
        break;
     case 5:
        //Quadratino pieno:
        myPainter->setBrush(Qt::black);
        myPainter->drawRect(int(X-markHalfWidth),int(Y-markHalfWidth),
                            int(markHalfWidth),int(markHalfWidth));
        break;
      case 6:
        //Triangolino pieno:
        myPainter->setBrush(Qt::black);
        polygon<<QPoint(int(X-markHalfWidth),int(Y+markHalfWidth));
        polygon<<QPoint(int(X+markHalfWidth),int(Y+markHalfWidth));
        polygon<<QPoint(int(X)              ,int(Y-markHalfWidth));
        myPainter->drawPolygon(polygon);
        break;
      case 7:
        //Crocetta dentro quadratino
        path.moveTo(int(X-markHalfWidth),int(Y-markHalfWidth));
        path.lineTo(int(X+markHalfWidth),int(Y+markHalfWidth));
        path.moveTo(int(X-markHalfWidth),int(Y+markHalfWidth));
        path.lineTo(int(X+markHalfWidth),int(Y-markHalfWidth));
        path.addRect(QRectF(double(X-markHalfWidth), double(Y-markHalfWidth),
                            double(2*markHalfWidth), double(2*markHalfWidth)));
        myPainter->drawPath(path);
        break;
    }
  myPainter->setBrush(Qt::white);
}


void CLineChart::drawSwarm(void){
/* Funzione per il tracciamento di uno sciame di punti, non collegati da alcuna linea. I punti possono essere minimi (pari ad un pixel) o medi (pari ad un quadratino 3x3  il cui centro è la coordinata del punto desiderato).
 */
  int i, iPlot=0, icount, igraf, iTotPlot=-1;
  int pointsDrawn0, ptRadius=swarmPointWidth/2;
  float sxmin, symin, xf, yf, x1f, y1f, yRatio;
  float x,y,x1,y1; //valori arrotondati di xf, yf, x1f, y1f
//  CursorShape->Width=1;
  ticPen.setWidth(1);
  plotPen.setWidth(1);
  framePen.setWidth(1);

  FC.getRect(X0,Y0,X1,Y1);

  if(xAxis.scaleType==stLin)
    sxmin=xAxis.minF;
  else
    sxmin=xAxis.eMin;
  pointsDrawn=0;
  for(i=0; i<nFiles; i++){
    xStartIndex[i]=NearInt(xAxis.pixPerValue  * (px[i][startIndex[i]] - sxmin))+X0;
    xStopIndex[i]=NearInt(xAxis.pixPerValue  * (px[i][stopIndex[i]] - sxmin))+X0;
  }
  for(i=0; i<nFiles; i++){
    for(igraf=0; igraf<nPlots[i]; igraf++) {
      iTotPlot++;
      if(blackWhite)
        plotPen.setColor(Qt::black);
      else
        plotPen.setColor(curveParamLst[iTotPlot].color);
      myPainter->setPen(plotPen);
// Calcolo YRatio e symin, valutando se sono relativi alla scala di sinistra o a quella eventuale di destra:
      if(curveParamLst[iTotPlot].rightScale){
        yRatio=ryAxis.pixPerValue ;
        if(yAxis.scaleType==stLin)
          symin=ryAxis.minF;
        else
          symin=ryAxis.eMin;
      }else{
         yRatio=yAxis.pixPerValue ;
         if(yAxis.scaleType==stLin)
           symin=yAxis.minF;
         else
           symin=yAxis.eMin;
      }
      pointsDrawn0=0;
      if(xAxis.scaleType==stLin)
        x1f=xAxis.pixPerValue  * (px[i][startIndex[i]]-sxmin)+X0;
      else
        x1f=xAxis.pixPerValue  * (log10f(px[i][startIndex[i]])-sxmin)+X0;
      if(yAxis.scaleType==stLin)
        y1f=yAxis.widthPix-yRatio * (py[i][igraf][startIndex[i]]-symin)+Y0;
      else
        y1f=yAxis.widthPix-yRatio * (log10f(py[i][igraf][startIndex[i]])-symin)+Y0;
      x1=NearInt(x1f);
      y1=NearInt(y1f);
      if(xAxis.scaleType==stLin)
        xf=xAxis.pixPerValue  * (px[i][startIndex[i]]-sxmin)+X0;
      else
        xf=xAxis.pixPerValue  * (log10f(px[i][startIndex[i]])-sxmin)+X0;
      if(yAxis.scaleType==stLin)
        yf=yAxis.widthPix-yRatio * (py[i][igraf][startIndex[i]]-symin)+Y0;
      else
        yf=yAxis.widthPix-yRatio * (log10f(py[i][igraf][startIndex[i]])-symin)+Y0;
      x=NearInt(xf);
      y=NearInt(yf);
      FC.getLine(x1,y1,x,y);
//Grafico fino al penultimo punto, con filtraggio e "Clippaggio". Il primo e l'ultimo punto li traccio fuori del loop: il primo altrimenti non verrebbe tracciato, l'ultimo per essere certo che venga comunque tracciato, anche se è nel prolungamento della retta congiungente i due punti precedenti.
      if(xAxis.scaleType==stLin)
        xf=xAxis.pixPerValue  * (px[i][startIndex[i]] - sxmin) +X0;
      else
        xf=xAxis.pixPerValue  * (log10f(px[i][startIndex[i]]) - sxmin) +X0;
      if(yAxis.scaleType==stLin)
        yf=yAxis.widthPix-yRatio*(py[i][igraf][startIndex[i]] - symin) +Y0;
       else
        yf=yAxis.widthPix-yRatio*(log10f(py[i][igraf][startIndex[i]]) - symin) +Y0;
      x=xf+0.5f;
      y=yf+0.5f;
      if(swarmPointSize==ssPixel)
        myPainter->drawPoint(QPointF(double(xf),double(yf)));
      else
        myPainter->drawRect(int(x-ptRadius),int(y-ptRadius),
                            swarmPointWidth,swarmPointWidth);
      pointsDrawn0++;
      for(icount=startIndex[i]+1; icount<stopIndex[i]; icount++)	{
        if(xAxis.scaleType==stLin)
          xf=xAxis.pixPerValue  * (px[i][icount] - sxmin) +X0;
        else
          xf=xAxis.pixPerValue  * (log10f(px[i][icount]) - sxmin) +X0;
        if(yAxis.scaleType==stLin)
          yf=yAxis.widthPix-yRatio*(py[i][igraf][icount] - symin) +Y0;
        else
          yf=yAxis.widthPix-yRatio*(log10f(py[i][igraf][icount]) - symin) +Y0;
        x=xf+0.5f;
        y=yf+0.5f;
        if(FC.isInRect(x,y)){ //Traccio il punto
          if(swarmPointSize==ssPixel)
            myPainter->drawPoint(QPointF(double(xf),double(yf)));
          else
            myPainter->drawRect(int(x-ptRadius),int(y-ptRadius),
                                swarmPointWidth,swarmPointWidth);
          pointsDrawn0++;
        }
      } //Fine ciclo for tracciamento curve
      //Tracciamento ultimo punto della curva:
      if(xAxis.scaleType==stLin)
        xf=xAxis.pixPerValue  * (px[i][stopIndex[i]] - sxmin) +X0;
      else
        xf=xAxis.pixPerValue  * (log10f(px[i][stopIndex[i]]) - sxmin) +X0;
      if(yAxis.scaleType==stLin)
        yf=yAxis.widthPix-yRatio*(py[i][igraf][stopIndex[i]] - symin) +Y0;
      else
        yf=yAxis.widthPix-yRatio*(log10f(py[i][igraf][stopIndex[i]]) - symin) +Y0;
      x=xf+0.5f;
      y=yf+0.5f;
      //Traccio l'ultimo punto :
      if(FC.isInRect(x,y)){ //Traccio il punto
        if(swarmPointSize==ssPixel)
          myPainter->drawPoint(QPointF(double(xf),double(yf)));
        else
          myPainter->drawRect(int(x-ptRadius),int(y-ptRadius),
                              swarmPointWidth,swarmPointWidth);
        pointsDrawn0++;
      }
      pointsDrawn=qMax(pointsDrawn,pointsDrawn0);
      if(swarmPointSize!=ssPixel)
      iPlot++;
    } //Fine tracciamento varie curve relative ad un medesimo file
  } //Fine ciclo for fra i vari files
}

int CLineChart::smartWriteUnit(QPainter * myPainter, QFont baseFont, int X, int Y, EadjustType hAdjust, EadjustType vAdjust, QString text,  bool addBrackets, bool Virtual ){
  /*
   * ***
   * FUNZIONE PURA
   * Questa è una funzione pura nel senso di Modelica: non usa alcuna variabile dell'
   * oggetto a cui appartiene, che non sia passata fra i parametri.
   * In tal modo la riutilizzabilità in CLineChart è grandemente facilitata.
   * ***
   *
   * Function che serve per mettere su una canvas il testo text, considerando gli
   * allineamenti orizzontale e verticale richiesti. X e Y sono quindi il punto iniziale,
   * finale o centrale del testo da scrivere a seconda se l'allineamento è atLeft,
   * atRight, atCenter.
   * In particolare se vAdjust è atLeft Y è il punto più alto del testo.
   * Se richiesto aggiunge all'inizio e alla fine del testo delle parentesi.
   *
   * Se smart è true, il comportamento è speciale e adatto a stringhe descriventi UNITA'
   * DI MISURA:
   * - i digit vengono presi come esponenti,
   * - la lettera 'u' come mu minuscolo (simbolo del micro)
   * - la lettera 'w' come omega maiuscolo (simbolo degli ohm)
   * - il carattere '.' è spostato più in alto ad indicare il prodotto
   *
   * In tal modo si possono scrivere in maniera professionale  praticamente tutte
   * le unità di misura dell'SI!
   *
   * Vi è infine il caso speciale delle potenze di 10, automaticamente attivo se la unit
   * comincia per "*10"
   *
   * Se  Virtual=true, la stringa non è scritta, e questa funzione ha il solo scopo del
   * calcolo della larghezza (come valore di ritorno)
   *
   * Il font di painter è temporaneamente modificato sia per fare l'esponente che per
   * l'eventuale omega in stile symbol, e poi rimesso al valore del widget in uscita.
   *
   * VALORE DI RITORNO: la lunghezza in pixel della stringa composita, considerando anche
   * le eventuali parentesi.
   */

   bool specialCase=false;  // caso speciale delle potenze di 10
   QFont   wFont; //work font
   if(text=="")
       return 0;
//   baseFont.setPixelSize(12);
   int expSize=int(0.7f*baseFont.pixelSize());
   int expOffs=int(0.4f*baseFont.pixelSize());

   // Fase0: caso speciale delle potenze di 10.
   if(text.mid(0,3)=="*10")
     specialCase=true;

   //Se non siamo in special case, il primo carattere deve essere letterale.
   //Però disattivo questo check in quanto non voglio fare un check ricco di validità, né voglio emettere un messaggeBox in questo caso. Meglio è quindi lasciare la label errata in quanto l'utente vedrà il problema e si regolerà di conseguenza.
  //   if(!specialCase && !text[0].isLetter())
  //       return 0;

   // Fase 1: procedo col calcolo della lunghezza in pixel della stringa.
  int len=0;
  for(int i=0; i<text.size(); i++){
    wFont=baseFont;
    QChar c=text[i];
    if(useSmartUnits){
      if(c=='w') c=QChar(0x03A9); //unicode per omega maiuscolo
      if(c=='u') c=QChar(0x03BC); //unicode per mu minuscolo
	}  
    if(useSmartUnits && (c.isDigit()||c=='-'||c=='+'||c=='.')){  //Vedo se questi caratteri vanno trattati come esponenti
      wFont=baseFont;
      if(specialCase && i>2)
          wFont.setPixelSize(expSize);
      if(!specialCase && i>0)
          wFont.setPixelSize(expSize);
    } else{
        wFont=baseFont;
    }
    myPainter->setFont(wFont);
    len+=myPainter->fontMetrics().horizontalAdvance(c);
  }
  myPainter->setFont(baseFont);
  if(addBrackets)
    len+=myPainter->fontMetrics().horizontalAdvance("()");
  if(Virtual)
  return len;
  //Il calcolo preliminare appena fatto mi serve per la gestione del punto di inizio della scrittura tenendo conto dei vari allineamenti previsti. Per ora però non lo uso e passo alla scrittura.
  int wX=X, wY=Y; //ascisse x e y di lavoro ("work")
  if(hAdjust==atRight)
    wX-=len;
  if(hAdjust==atCenter)
    wX-=len/2;
  if(vAdjust==atLeft)
    wY+=myPainter->fontMetrics().height()-1;
  if(vAdjust==atCenter)
    wY+=myPainter->fontMetrics().height()/2-1;
  if(addBrackets){
    myPainter->drawText(wX,wY,"(");
    wX+=myPainter->fontMetrics().horizontalAdvance("(");
  }
    for(int i=0; i<text.size(); i++){
      wFont=baseFont;
      QChar c=text[i];
      if(useSmartUnits){
        if(c=='w') c=QChar(0x03A9); //unicode per omega maiuscolo
        if(c=='u') c=QChar(0x03BC); //unicode per mu minuscolo
      }
      if(useSmartUnits && (c.isDigit()||c=='-'||c=='+'||c=='.')){
        wFont=baseFont;
        if( (specialCase && i>2) || (!specialCase && i>0)){
          wFont.setPixelSize(expSize);
          qDebug()<<"expSize: "<< expSize << "; expOffs: " << expOffs;
          myPainter->setFont(wFont);
          myPainter->drawText(wX,wY-expOffs,QString(c));
        }else{
          myPainter->setFont(wFont);
          myPainter->drawText(wX,wY,QString(c));
        }
        wX+=myPainter->fontMetrics().horizontalAdvance(c);
      } else{
        myPainter->setFont(baseFont);
        myPainter->drawText(wX,wY,QString(c));
        wX+=myPainter->fontMetrics().horizontalAdvance(c);
      }
    }

  if(addBrackets){
    myPainter->setFont(baseFont);
    myPainter->drawText(wX,wY,")");
  }
  return len;
}



int CLineChart::writeText2(QPainter * myPainter, int X, int Y, EadjustType hAdjust, EadjustType vAdjust, QString msg1, QString msg2, bool addBrackets, bool Virtual){
/*
 *    * ***
   * FUNZIONE PURA
   * Questa è una funzione pura nel senso di Modelica: non usa alcuna variabile dell'
   * oggetto a cui appartiene, che non sia passata fra i parametri.
   * In tal modo la riutilizzabilità in CLineChart è grandemente facilitata.
   * ***
   *
   * Function che serve per mettere su una canvas il testo msg1, seguito dal testo  msg2,
 * come esponente (più piccolo e allineato in alto). Gestisce gli allineamenti (orizzontale
 *  e verticale) specificati, ritorna la larghezza in pixel della stringa completa.
 *
 * Notare che:
 * - se  Virtual=true, la stringa non è scritta, e questa funzione ha il solo scopo del
 *   calcolo della larghezza (come valore di ritorno) MA AL MOMENTO (SETT 2015) NON E'
 *  MAI RICHIAMATA CON L'ULTIMO PARAMETRO TRUE!.
 *
 * Note Qt:
 * - All'ingresso in questa funzione "painter" ha il valore default del widget CLineChart.
 *   Quindi painter>fontMetrics() coincide con fontMetrics() e con this->fontMetrics().
 * - Il font di painter è temporaneamente modificato sia per fare l'esponente che per
 *   l'eventuale omega in stile symbol, e poi rimesso al valore del widget in uscita.
 *
 *  VALORE DI RITORNO: la lunghezza in pixel della stringa composita, considerando sia
 * msg1 che msg2 che le eventuali parentesi.
*/
  int len=0; //Lunghezza senza hOffset;
  int ret; //valore di ritorno: lunghezza compreso hOffset
  int xPosition=X,
      width1, width2, wBracket, //larghezza testo msg1, msg2 e di una parentesi
      H1, H2; //Altezza testo msg1 e msg2.
  int hOffset, vOffset; //un certo numero di pixel da lasciare fra il testo e le coordinate passate. Non li metto da fuori perché se li gestisco qui dentro li posso esprimere in funzione del fontSize. Questo è particolarmente importante in quanto sulla stampante ho un effetto completamente differente, a parità di pixel, che sullo schermo per via della grande differenza sulla dimensione del pixel stesso.

/* La Y passata a questa routine è il pixel superiore in cui scrivere.
In Qt la posizione verticale da passare per il tracciamento non è il margine superiore del testo come in BCB ma il margine inferiore
Questo ha comportato nel seguente if la sostituzione di "atLeft" con "atRight" e lo 0.55 con 0.45 (poi trasformato in 0.35)  ***/
// int aaa=smallHSpace;
  if(msg1+msg2=="")
      return 0;
  myPainter->setFont(baseFont);
  width1=myPainter->fontMetrics().horizontalAdvance(msg1);
  H1=myPainter->fontMetrics().height();
  myPainter->setFont(expFont);
  width2=myPainter->fontMetrics().horizontalAdvance(msg2);
  H2=myPainter->fontMetrics().height();

  if(addBrackets)
    wBracket=myPainter->fontMetrics().horizontalAdvance("(");
  else
    wBracket=0;

  if(msg1=="")return 0;
  if(vAdjust==atRight)
    vOffset=int(0.1f*H1);
  else if(vAdjust==atCenter)
    vOffset=int(0.35f*H1);
  else{ //atLeft
    vOffset=int(1.0f*H1);
}
//Determinazione del pixel più a sinistra del testo da scrivere xPosition ritoccando il valore di partenza pari all'"X" ricevuto dalla funzione
  myPainter->setFont(baseFont);
  if(hAdjust==atLeft){
    hOffset=2*smallHSpace;
    xPosition+=hOffset;
  }else if(hAdjust==atCenter){
      xPosition-=(width1+width2)/2 +wBracket*int(addBrackets);
   }else{ //hAdjust=atRight
      hOffset=int(1.5f*smallHSpace);
      if(msg2!="")
        hOffset=smallHSpace;
      xPosition-= width1+width2+2*wBracket*int(addBrackets)+hOffset;
  }

  //Scrittura parentesi sinistra:
   if(!Virtual && addBrackets && msg1!="")
     myPainter->drawText(xPosition,Y+vOffset,"(");
   if(addBrackets && msg1!="")
       len+=wBracket;

   //Scrittura del testo msg1
   if(!Virtual)
      myPainter->drawText(xPosition+len,Y+vOffset,msg1);
  len+=width1;


  //Scrittura del testo msg2 :
  myPainter->setFont(expFont);
  if(!Virtual && width2>0)
      myPainter->drawText(xPosition+len,int(Y+vOffset-0.4*H2),msg2);
  len+=width2;
  myPainter->setFont(baseFont);

  //Scrittura parentesi destra:
  if(!Virtual && addBrackets && msg1!="")
    myPainter->drawText(xPosition+len,Y+vOffset,")");
  if(addBrackets && msg1!="")
      len+=wBracket;

  return ret=len;
}

void CLineChart::disableTitle(){
  writeTitle1=false;
  myPainter->drawRect(0,0,geometry().width(),titleHeight);
  resizeStopped();
}

void CLineChart::enableTitle(){
  writeTitle1=true;
  resizeStopped();
}


bool CLineChart::event(QEvent *event){
    /* Function per la gestione del tootip di visualizzazione dei valori numerici dei dati*/
    if(!enableTooltip) return true;
    if (event->type() == QEvent::ToolTip) {
      if(!xVarParam.isMonotonic)return true;
      QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
      // QHelpEvent means that a tooltip was requested

      // Se sono fuori del rettangolo X0-Y0/X1-Y1 faccio solo la gestione del tooltip delle variabili.
      QPoint pos=helpEvent->pos();
      if( pos.x()<X0 || pos.x()>X1  || pos.y()<Y0 || pos.y()>Y1){
          setCursor(Qt::ArrowCursor);
          setToolTip("");
          foreach(SHoveringData hovData, hovDataLst){
            if(hovData.rect.contains(pos)){
              if(!curveParamLst[hovData.iTotPlot].isFunction)break;
              hovVarRect= hovData.rect;
//              QString str=curveParamLst[hovData.iTotPlot].fullName;
              QToolTip::showText(helpEvent->globalPos(), curveParamLst[hovData.iTotPlot].fullName);
//              setToolTip(lCurveParam[hovData.iTotPlot].fullName);
            }
          }
          return true;
      }

      QPoint nearP;
      QPointF valueP;
      int ttType=giveNearValue(helpEvent->pos(),nearP,valueP);
      qDebug()<<"nearValue x: nearX: :"<<helpEvent->pos().x()<<nearP.x();
      if(ttType==0){
          QToolTip::hideText();
          return true;
      }
      QString  sX=QString::number(valueP.x());
      QString  sY=QString::number(valueP.y());

      if(ttType==1)
        QToolTip::showText(helpEvent->globalPos(), "x: "+sX+"\nry: "+sY);
      else //ttType=-1
        QToolTip::showText(helpEvent->globalPos(), "x: "+sX+"\ny: "+sY);
      // attivo il seguente timer che serve per vedere quando i tooltip è scomparso, e di conseguenza cancellare anche il quadratino rosso.
      tooltipTimer->start(200);
      bool thick=plotPen.width()>1;
      tooltipRect.moveTo(nearP-QPoint(2+thick,2+thick));
      rectTTVisible=true;
      update();
      return true;
    }
    if (event->type() == QEvent::ToolTipChange) {
      rectTTVisible=false;
      update();
      return true;
    }
  return QWidget::event(event);

}


bool  CLineChart::fillPixelToIndex(int **pixelToIndexDX){
  //Funzione che, nel caso di file a passo variabile, dà per ogni pixel l'indice del punto del file tracciato a destra del pixel considerato, più prossimo al pixel stesso.
  //Ritorna true se il lavoro è stato eseguito, false se ci sono stati errori.
  //I pixel si incominciano a contare dal rettangolo del grafico, cioè da X0. Quindi quando l'indice di pixel è 0 mi riferisco al segmento verticale di sinistra
    // I valori vengono attribuiti all'array passato; in realtà lineChart contiene un array privato di nome proprio pixelToIndexDX; quindi si poteva anche evitare di passare questo argomento.
  if(numOfVSFiles==0)
    return false;
  int VSFile=0, iFile, iPixel, index;
  float t, tStart, tStep;

  tStart=xAxis.scaleMin * xAxis.scaleFactor;
  tStep=(xAxis.scaleMax*xAxis.scaleFactor - tStart)/(X1-X0);
  for(iFile=0; iFile<nFiles; iFile++)
    if(filesInfo[iFile].variableStep){
      // t=tStart;
      //Il punto corrispondente a X0 è ovviamente il primo punto:
      index=startIndex[iFile];
      pixelToIndexDX[VSFile][0]=index;
      t=tStart;
      for(iPixel=1; iPixel<=X1-X0; iPixel++){
//        t=tStart+iPixel*tStep;
        t+=tStep;
        while (px[iFile][index]<t && index<filesInfo[iFile].numOfPoints-1 )
            index++;
        pixelToIndexDX[VSFile][iPixel]=index;
      }
      VSFile++;
    }
  return true;
}


bool  CLineChart::fillPixelToIndexLog(int **pixelToIndexDX){
  //Versione di fillPixelToIndex per scale stLog e stDB.
  int iPix, index, iFile;
  float val0;
  for(iFile=0; iFile<nFiles; iFile++){
    index=startIndex[iFile];
    pixelToIndexDX[iFile][0]=index;
    val0=log10f(px[iFile][index]/xAxis.scaleMin);
    for(iPix=1; iPix<=X1-X0; iPix++){
      while (log10f(px[iFile][index]/xAxis.scaleMin)<iPix/xAxis.pixPerValue +val0 &&
             index<filesInfo[iFile].numOfPoints-1)
          index++;
      pixelToIndexDX[iFile][iPix]=index;
    }
  }
 return true;
}


struct CLineChart::SMinMax CLineChart::findMinMax(float *vect, int dimens)
{
   struct SMinMax vmM;

   vmM.Min=*vect;
   vmM.Max=*vect;

//   float v[5];
//   for (unsigned i=1; i<min(5,dimens); i++) v[i] =*(vect+i);

   for (int icount=1; icount<dimens; icount++)
   {
      vmM.Min=qMin(vmM.Min, *(vect+icount));
      vmM.Max=qMax(vmM.Max, *(vect+icount));
   }
   return(vmM);
}


void CLineChart::getData(float *px_,float **py_, int nPoints_, int nPlots_){
   /*funzione  per l'utilizzo elementare di lineChart: associabile naturalmente a plot() (senza parametri passati).*/
   int i;
   SFileInfo FI;
   //Attribuisco valori default standard alle variabili necessarie al getData per singole variabili asse x,
   FI.name="fileName";
   FI.numOfPoints=nPoints_;
   FI.variableStep=true;
   xVarParam.name="x";
   xVarParam.isMonotonic=true;
   xVarParam.isVariableStep=true;
   curveParamLst.clear();
   SCurveParam param;
   for(i=0; i<nPlots_; i++){
     if(i==0)param.color=Qt::red;
     if(i==1)param.color=Qt::green;
     if(i==2)param.color=Qt::blue;
     if(i==3)param.color=Qt::gray;
     if(i>3)param.color=Qt::black;
     param.name="var";
     param.rightScale=false;
     param.unitS="";
     param.style=Qt::SolidLine;
     curveParamLst.append(param);
    }

   getData(FI,  nPlots_, xVarParam, curveParamLst, px_, py_);

}
void CLineChart::getData(SFileInfo FI, int nPlots_,  SXVarParam xVarParam_, QList <SCurveParam> &lCurveParam_,float *px_,float **py_){
    /*getData per il caso di singole variabili asse x*/
    filesInfo.clear();
    nPlots.clear();
    filesInfo.append(FI);

    //L'uso delle seguenti variabili con "1" in fondo evita di dover fare delle chiamate a new per un unico elemento dei vettori, però non è una tecnica sicura. Funziona con molti compilatori, ma con il MingW (e Qt 5.0.2) dà segmetation fault. La abbandonerò progressivamente.
    //Per ora ho eliminato variableStep, avendola sostituita con filesInfo[iFile].variableStep.
    variableStep1=FI.variableStep;    //variableStep=&variableStep1;
    nPlots1=nPlots_;
    nPlots.append(nPlots_);

    xVarParam=xVarParam_;
    curveParamLst=lCurveParam_;

    nFiles=1;
    delete[] px;
    delete[] py;
    px= new float*[1];
    py= new float **[1];
    px[0]=px_;
    py[0]=py_;
    numOfTotPlots=nPlots[0];
    if(variableStep1)numOfVSFiles=1;
    dataGot=true;
}

void CLineChart::getData(QList <SFileInfo> FIlist, const QVector <int> &nPlots_,  SXVarParam xVarParam_, QList <SCurveParam> lCurveParam_,float **px_,float ***py_){
    /*getData per il caso di multiple variabili asse x*/

    filesInfo=FIlist;
    nFiles=FIlist.count();
    nPlots=nPlots_;
    xVarParam=xVarParam_;
    curveParamLst=lCurveParam_;
    px=px_;
    py=py_;
    numOfTotPlots=0;
    numOfVSFiles=0;
    for(int i=0; i<nFiles; i++){
        numOfTotPlots+=nPlots[i];
        if(FIlist[i].variableStep)numOfVSFiles++;
    }
    dataGot=true;
}

void CLineChart::getUserUnits(QString xUnit, QString yUnit, QString ryUnit){
    userUnits.x=xUnit;
    userUnits.y=yUnit;
    userUnits.ry=ryUnit;
}


int CLineChart::giveActiveDataCurs(){
    if(dataCurs2Visible) return 2;
    if(dataCursVisible) return 1;
    return 0;
}

SFloatRect2 CLineChart::giveDispRect(){
    return dispRect;
}


QString CLineChart::givexUnit(){
   return xVarParam.unitS;
}

SFloatRect2 CLineChart::giveFullLimits(){

  //float Left,Right,LTop,LBottom,RTop,RBottom;
  SFloatRect2 fullLimits;
  fullLimits.Left=xAxis.minVal;
  fullLimits.Right=xAxis.maxVal;
  fullLimits.LTop=yAxis.maxVal;
  fullLimits.LBottom=yAxis.minVal;
  fullLimits.RTop=ryAxis.maxVal;
  fullLimits.RBottom=ryAxis.minVal;
  return fullLimits;
}


EPlotPenWidth CLineChart::givePlotPenWidth() const {
    return PPlotPenWidth;
}


 SXYValues CLineChart::giveValues(int cursorX, bool interpolation,  bool Xdiff, bool Ydiff){
    int nearX;
    return giveValues(cursorX, interpolation, nearX, Xdiff, Ydiff);
}


SXYValues CLineChart::giveValues(int cursorX, bool interpolation, int &nearX, bool xDiff, bool yDiff){
/*  Function che dà i valori delle variabili visualizzate in corrispondenza della posizione, passata come parametro, della retta di cursore.
 Nel vettore di uscita si troverà prima il valore della variabile sull'asse x,  poi quello delle variabili sull'asse y.
 NearX ha un significato pregnante soltanto se non sono in multifile o se in multifile le varie variabili X coincidono. In questo caso dà la posizione in pixel del punto rispetto a cui vengono dati i valori. Essa verrà sfruttata per muovere "a scatto" il cursorShape nel punto giusto.
Nel caso di multifile con files aventi tempi di campionamento diversi, i vari valori si riferiscono in generale ad ascisse differenti, e quindi questa logica del movimento "a scatto" ha poco senso. In questo caso nearX contiene l'ascissa del valore calcolato rispetto all'ultimo file considerato.
Argomenti:
- cursorX: valore del pixel orizzontale del mouse
- interpolation: se true si fa l'interpolazione lineare fra le asciss e i relativi valori
- nearX: ritorna il valore del pixel orizzontale del punto più vicino a cursorC
- xDiff e yDiff se true danno le differenze rispetto ai valori del precedente cursore
*/

  /******************************************************************
  Questa funzione dà per scontato che sia già stato effettuato un plot, e non fa alcun
  check a riguardo. E' responsabilità del chiamante richiamare questa funzione soltanto
  se esiste un plot effettuato in LineChart.
  ****************************************************************/

  int iFile, iVar,
    netCursorX,//valore del cursorX passato valutato entro il rettangolo (X0, Y0, X1, Y1)
    iVSFile=0, //indice del file VariableStep
    indexSX, //Indice del punto a SX del cursore
    index;//l'indice del punto più vicino al cursore
  float fCursorX,//valore numerico sull'asse x corrispondente alla posizione del cursore
        deltaX, slope;
  SXYValues ret;
  netCursorX=qMin(qMax(cursorX-X0-margin,0),X1-X0-2*margin);
  if(xAxis.scaleType==stLin)
    fCursorX=xAxis.scaleMin*xAxis.scaleFactor+netCursorX/xAxis.pixPerValue ;
  else{
    //In questo caso devo tener conto del logaritmo per la costruzione del grafico,
    // e posso omettere ScaleFactor che è sempre unitario:
    fCursorX=powf(10,xAxis.eMin+netCursorX/xAxis.pixPerValue );
  }
  //Può capitare che il cursore sia fuori della zona in cui ci sono grafici.
  //In tal caso riporto dentro il valore di fCusorX:
 //    fCursorX=min(fCursorX,xmM.Max);
 //   fCursorX=max(fCursorX,xmM.Min);
  fCursorX=qMin(fCursorX,dispRect.Right);
  fCursorX=qMax(fCursorX,dispRect.Left);
  if(xDiff)
    lastXCurs[1]=fCursorX;
  else if(yDiff)
    lastXCurs[2]=fCursorX;
  else
    lastXCurs[0]=fCursorX;


  if(!xVarParam.isMonotonic){
     QMessageBox::critical(this,"MC's PlotXY","X variable here must be monotonic",QMessageBox::Ok);
//      qApp->closeAllWindows();
     //Le seguenti due righe solo per evitare un Warning:
     ret.X=cursorXValues;
     ret.Y=cursorYValues;
     return ret;
  }
  for(iFile=0; iFile<nFiles; iFile++){
    int nPoints=filesInfo[iFile].numOfPoints;
/***Per ogni grafico scelgo la coppia di valori più vicina alla riga-cursore visualizzata:     ***/
    if(filesInfo[iFile].variableStep){
      indexSX=qMax(0,pixelToIndexDX[iVSFile][netCursorX]-1);
      deltaX=px[iFile][indexSX+1]-px[iFile][indexSX];
      if(fCursorX-px[iFile][indexSX]<px[iFile][indexSX+1]-fCursorX)
        index=indexSX;
      else
        index=indexSX+1;
    }else{
      deltaX=px[iFile][1]-px[iFile][0];
      indexSX=int((fCursorX-px[iFile][0])/deltaX);

      index=NearInt((fCursorX-px[iFile][0])/deltaX);
      //Può accadere che fra l'ultimo e il penultimo punto non si abbia la stessa distanza
      //che fra gli altri. Questo perché se Tmax supera TOld+DeltaT, vengono registrati
      //i valori a Tmax e non a TOld+DeltaT. La seguente riga considera questa eventualità:
      if(  fabsf(px[iFile][nPoints-1]-fCursorX)  <  fabsf(px[iFile][nPoints-2]-fCursorX)
         &&  index<nPoints-1    ) index++;
    }
    if(interpolation){
      for(iVar=0; iVar<nPlots[iFile]; iVar++){
        slope=(py[iFile][iVar][indexSX+1]-py[iFile][iVar][indexSX])/deltaX;
        cursorXValues[iFile]=fCursorX-int(xDiff*cursorXValBkp[iFile]);
        cursorYValues[iFile][iVar]=py[iFile][iVar][indexSX]+
        /*Slope=DeltaY/DeltaX:*/ slope*(fCursorX-px[iFile][indexSX]) -int(yDiff)* cursorYValBkp[iFile][iVar];
        if(!yDiff)cursorYValBkp[iFile][iVar]=cursorYValues[iFile][iVar];
      }
    }else{
      /*Occorre tener conto della possibilità che non tutti i grafici hanno piena
      lunghezza. Dove ho un valore non inizializzato, per convenzione (simile a
      quella di MODELS) gli dò il valore 8888.8
      */
      for(iVar=0; iVar<nPlots[iFile]; iVar++){
        if(fCursorX<=px[iFile][nPoints-1]){
          cursorXValues[iFile]=px[iFile][index]-int(xDiff)*cursorXValBkp[iFile];
          cursorYValues[iFile][iVar]=py[iFile][iVar][index] -int(yDiff)* cursorYValBkp[iFile][iVar];
          if(!yDiff)cursorYValBkp[iFile][iVar]=cursorYValues[iFile][iVar];
        }else{
          cursorXValues[iFile]=8888.8f;
          cursorYValues[iFile][iVar]=8888.8f;
        }
      }
    }
    if(filesInfo[iFile].variableStep)
      iVSFile++;
    if(interpolation||nFiles>1)
      nearX=cursorX;
    else{
      if(cursorXValues[iFile]==8888.8f)
        nearX=X0;
      else{
        if(xAxis.scaleType==stLin)
          nearX=X0+margin+int(xAxis.pixPerValue * (cursorXValues[iFile]+xDiff*
                cursorXValBkp[iFile] -xAxis.scaleMin*xAxis.scaleFactor));
        else
          nearX=X0+margin+int(xAxis.pixPerValue *log10f(cursorXValues[iFile]/xAxis.scaleMin));
      }
    }
    if(!xDiff)
      cursorXValBkp[iFile]=cursorXValues[iFile];
  }
  ret.X=cursorXValues;
  ret.Y=cursorYValues;
  return ret;
}

int CLineChart::giveNearValue(QPoint mouseP , QPoint &nearP, QPointF &valueP){
/*  Function che dà il valore della variabile visualizzata più vicina al punto
 * in cui è situato il cursore del mouse.
 * cursor.x() e cursor.y() danno la posizione del cursore;
 * in near.x() e near.y() vengono salvati i relativi pixel del punto più vicino
 * della curva più vicina al cursore, i corrispondenti valori sono riportati
 * in value.x(), value.y().
 * Se però il cursore è troppo lontano da tutte le curve, nearX viene
 *  posto = -1, e il valore di ritorno è da considerarsi invalido.
 *
 * Il valore di ritorno è
 * 0 se il cursore non è sufficientemente vicino ad alcuna curva
 * -1 se il cursore è sufficientemente vicino a una curva con scala sull'asse ly
 * +1 se il cursore è sufficientemente vicino a una curva con scala sull'asse ry
*/

  /******************************************************************
  Questa funzione dà per scontato che sia già stato effettuato un plot, e non fa alcun
  check a riguardo. E' responsabilità del chiamante richiamare questa funzione soltanto
  se esiste un plot effettuato in LineChart.
  ****************************************************************/

  bool rightScale=false;
  int iFile, nearX, nearY,
     VSFile=0, //indice del file VariableStep
     indexSX, //Indice del punto a SX del cursore
     index; //l'indice del punto più vicino al cursore
  float fMouseX, //valore numerico sull'asse x corrispondente alla posizione del mouse
        deltaX,
        symin;

  QPoint netMouseP; //valore del mouseP passato valutato entro il rettangolo (X0, Y0, X1, Y1)
  int ret=0;
  if(plotType==ptBar)
      return ret;

//  netMouseP.rx()=min(max(mouseP.x()-X0,0),X1-X0);
  netMouseP.rx()=qMin(qMax(mouseP.x()-xStartIndex[0],0),X1-X0);
  netMouseP.ry()=qMin(qMax(mouseP.y()-Y0,0),Y1-Y0);

  if(xAxis.scaleType==stLin)
      fMouseX=xAxis.scaleMin*xAxis.scaleFactor+netMouseP.x()/xAxis.pixPerValue ;
  else
    //In questo caso devo tener conto del logaritmo per la costruzione del grafico,
    // e posso omettere ScaleFactor che è sempre unitario:
    fMouseX=powf(10,xAxis.eMin+netMouseP.x()/xAxis.pixPerValue );
  //Può capitare che il cursore del mouse sia fuori della zona in cui ci sono grafici.
  //In tal caso riporto dentro il valore di fCusorX:
//  fMouseX=min(fMouseX,xmM.Max);
  fMouseX=qMin(fMouseX,dispRect.Right);

  if(!xVarParam.isMonotonic){
    QMessageBox::critical(this,tr("MC's PlotXY"),tr("X variable here must be monotonic"),QMessageBox::Ok);
      return false;
  }

  /* La ricerca dei dati da fornire in output, cioè nearP e valueP avviene così:
   * 1) prima cerco il punto visualizzato sulla scheda del plot più vicino al cursore
   *    a prescindere dalla vicinanza stessa
   * 2) poi verifico se la vicinanza è sufficiente a visualizzare il punto o no.
   *
   * Il punto 1) a sua volta si esegue come segue:
   *   1a) ipotizzo che il punto più vicino sia il primo punto della prima curva
   *       del primo file
   *   1b)poi in un loop, ogni qual volta trovo un punto migliore effettuo la
   *      sostituzione
 */

   //fase 1a):
  if(xAxis.scaleType==stLin)
    nearX=int(X0+xAxis.pixPerValue *(px[0][0] - xAxis.scaleMin* xAxis.scaleFactor));
  else
    nearX=int(X0+xAxis.pixPerValue *log10f(px[0][0]/xAxis.scaleMin));
  if(yAxis.scaleType==stLin)
    nearY=int(Y1-yAxis.pixPerValue *(py[0][0][0] - yAxis.scaleMin* yAxis.scaleFactor));
  else
    nearY=int(Y1-yAxis.pixPerValue *log10f(py[0][0][0]/yAxis.scaleMin));

  nearP=QPoint(nearX,nearY);
  valueP=QPoint(int(px[0][0]), int(py[0][0][0]));
  rightScale=curveParamLst[0].rightScale;

  //fase 1b):
  int iTotPlot=-1;
  for(iFile=0; iFile<nFiles; iFile++){
    int nPoints=filesInfo[iFile].numOfPoints;
    /* Prima di tutto trovo index che è l'indice del valore sull'asse x più vicino al cursore*/
    if(filesInfo[iFile].variableStep){
      indexSX=qMax(0,pixelToIndexDX[VSFile][netMouseP.x()]-1);
      if(fMouseX-px[iFile][indexSX]<px[iFile][indexSX+1]-fMouseX)
        index=indexSX;
      else
        index=indexSX+1;
    }else{
      deltaX=px[iFile][1]-px[iFile][0];
      index=NearInt((fMouseX-px[iFile][0])/deltaX);
      //Può accadere che fra l'ultimo e il penultimo punto non si abbia la stessa distanza
      //che fra gli altri. Questo perché se Tmax supera TOld+DeltaT, vengono registrati
      //i valori a Tmax e non a TOld+DeltaT. La seguente riga considera questa eventualità:
      if(  fabsf(px[iFile][nPoints-1]-fMouseX)   <   fabsf(px[iFile][nPoints-2]-fMouseX)
        &&  index<nPoints-1 )
          index++;
    }
    // index trovato!

    //La seguente condizione non si dovrebbe mai verificare. Ma siccome è stato visto che questo in rarissimi casi accade, al fine di facilitare il debug metto il seguente check, comodo per mettere un breakpoiont su badIndex=true;
    bool badIndex=false;
    if(index>=nPoints)
        badIndex=true;

    //Ora scelgo per il file corrente il punto più vicino verticalmente al cursore
    if(xAxis.scaleType==stLin)
      nearX=X0+margin+int(xAxis.pixPerValue *(px[iFile][index] - xAxis.scaleMin* xAxis.scaleFactor));
    else
      nearX=X0+margin+int(xAxis.pixPerValue *log10f(px[iFile][index]/xAxis.scaleMin));

    for (int iVar=0; iVar<nPlots[iFile] ; iVar++){
      iTotPlot++;
      float vAxisRatio;
      if(curveParamLst[iTotPlot].rightScale){
          vAxisRatio= ryAxis.pixPerValue ;
      }else{
          vAxisRatio= yAxis.pixPerValue ;
      }

      if(yAxis.scaleType==stLin &&curveParamLst[iTotPlot].rightScale){
        symin=ryAxis.minF;
      }else if(yAxis.scaleType!=stLin &&  curveParamLst[iTotPlot].rightScale){
        symin=ryAxis.eMin;
      }else if(yAxis.scaleType==stLin && !curveParamLst[iTotPlot].rightScale){
        symin=yAxis.minF;
      }else{
        symin=yAxis.eMin;
      }

      if(yAxis.scaleType==stLin)
        nearY=NearInt(Y1-vAxisRatio * (py[iFile][iVar][index]-symin));
      else
        nearY=NearInt(Y1-vAxisRatio * (log10f(py[iFile][iVar][index])-symin));
      QPointF tempValueP=QPointF(double(px[iFile][index]), double(py[iFile][iVar][index]));
      if( abs(nearY-mouseP.y()) < abs(nearP.y()-mouseP.y()) ){
         valueP=tempValueP;
         nearP=QPoint(nearX,nearY);
         rightScale=curveParamLst[iTotPlot].rightScale;
      }
    }

  }
  //fase 2):
  if(abs(nearP.x()-mouseP.x())<tooltipMargin/2 ||
     abs(nearP.y()-mouseP.y())<tooltipMargin          )
     ret=-1;
  if(ret==-1 && rightScale) ret=1;
  //Il punto è considderato vicino se son vicine anche solo la x o la y. Però il punto deve essere visualizzato, altrimenti va considerato lontano. Faccio questa correzione qui:
  if(nearP.x()<X0 ||nearP.x()>X1)ret=0;
  if(nearP.y()<Y0 ||nearP.y()>Y1)ret=0;
  if(ret==0)
     qDebug()<<"nearP:"<<nearP<<"mouseP:"<<mouseP<<"ret:"<<ret;
  return ret;
}


SFloatRect2 CLineChart::giveZoomRect(int StartSelX, int StartSelY, int X, int Y){
    SFloatRect2 R;

    if(xAxis.scaleType==stLin){
        R.Left=xAxis.scaleMin*xAxis.scaleFactor+qMax(StartSelX-X0,0)/xAxis.pixPerValue ;
        R.Right=xAxis.scaleMin*xAxis.scaleFactor+qMin(X-X0,xAxis.widthPix)/xAxis.pixPerValue ;
    }else{
        //In questo caso devo tener conto del logaritmo per la costruzione del grafico,
        // e posso omettere ScaleFactor che è sempre unitario:
        R.Left= powf(10,xAxis.eMin+qMax(StartSelX-X0,0)/xAxis.pixPerValue );
        R.Right=powf(10,xAxis.eMin+qMin(X-X0,xAxis.widthPix)/xAxis.pixPerValue );
    }
    if(yAxis.scaleType==stLin){
        R.LBottom=yAxis.scaleMin*yAxis.scaleFactor+qMax(Y1-Y,0)/yAxis.pixPerValue ;
        R.LTop=   yAxis.scaleMin*yAxis.scaleFactor+qMin(Y1-StartSelY,yAxis.widthPix) /yAxis.pixPerValue ;
  }else{
        //In questo caso devo tener conto del logaritmo per la costruzione del grafico,
        // e posso omettere ScaleFactor che è sempre unitario:
        R.LBottom=powf(10,yAxis.eMin+qMax(Y1-Y,0)/yAxis.pixPerValue );
        R.LTop=   powf(10,yAxis.eMin+qMin(Y1-StartSelY,yAxis.widthPix)/yAxis.pixPerValue );
  }
    if(twinScale){
        R.RBottom=ryAxis.scaleMin*ryAxis.scaleFactor+qMax(Y0+yAxis.widthPix-Y,0)/ryAxis.pixPerValue ;
        R.RTop=ryAxis.scaleMin*ryAxis.scaleFactor+qMin(Y0+yAxis.widthPix-StartSelY,yAxis.widthPix) /ryAxis.pixPerValue ;
    }
    return R;
}


bool CLineChart::isZoomed(){
    return zoomed;
}

void CLineChart::keyPressEvent(QKeyEvent * event){
  /* In questa funzione implemento lo spostamento del cursore di visualizzazione dei
   * dei dati da tastiera.
   * Se è premuta una freccetta semplice sposto di un pixel; se è associato il tasto CTRL
   *  alla freccetta sposto di tre.
  */
  if(!dataCursVisible){
    QLabel::keyPressEvent(event);
    return;
  }
  int pixShift=1;
  if(event->modifiers()&Qt::ControlModifier)
      pixShift=3;

  if(event->key()==Qt::Key_Left)
    dataCurs.moveLeft(dataCurs.x()-pixShift);
  if(event->key()==Qt::Key_Right)
    dataCurs.moveLeft(dataCurs.x()+pixShift);

  //Calcolo i valori e li mando fuori:
  SXYValues values;

  //la seguente chiamata attribuisce un valore a nearX:
  int nearX;
  if(xAxis.scaleType==stLin)
    values=giveValues(dataCurs.x()-1, linearInterpolate, nearX, false, false);
  else{
    values=giveValues(dataCurs.x()-(xStartIndex[0]-X0), linearInterpolate, nearX, false, false);
    // La seguente riga non funziona in quanto la correzione ccon nearX nel caso di scale logaritmiche non è precise. In particolare l'errore dovuto alla correzione è siperiore al passo di 1 o  pixel dovuto alla keyboard, e il risultato finale con filterQS è che il cursore si spsota a destra qualunque sia il tasto che io ho premuto!
//    dataCurs.moveLeft(nearX-(dataCurs.width()-1)/2);
  }
qDebug()<<"x(): "<<dataCurs.x();
qDebug()<<"width:"<<dataCurs.width();
  emit valuesChanged(values,false,false);

  update();
  QLabel::keyPressEvent(event);
}

void CLineChart::leaveEvent(QEvent *){
    setCursor(Qt::ArrowCursor);
}


void CLineChart::mouseMoveEvent(QMouseEvent *event)
{
/* Questa routine ha tre sezioni:
1) nella prima gestisce l'eventuale visualizzazione di informazioni aggiuntive su una variabile quando si fa l'hovering sul nome. Al momento è implementata solo la visualizzazione del nome di funzione completo quando si fa l'hovering sul nome conciso normalmente visualizzato
2) nella seconda gestisce la selezione dell'area di zoom
3) nella terza gestisce i cursori di dati
*/
  int nearX;
  static SXYValues values;
  int posX=event->pos().x();
  if(dataCursDragging)
    hovVarRect=QRect(0,0,0,0);
    //La seguente sezione 1 è stata trasferita all'interno della funzione event (che cestisce anche lo snap to grid)
    /*
//1) visualizzazione informazioni aggiuntive quando si fa l'hovering sul nome di variabile
    setCursor(Qt::ArrowCursor);
    //1) verifico se sono all'interno di un'area contenente il nome di variabile.
    setToolTip("");
    foreach(SHoveringData hovData, hovDataLst){
      if(hovData.rect.contains(event->pos())){
        if(!lCurveParam[hovData.iTotPlot].isFunction)break;
        hovVarRect= hovData.rect;
        QString str=lCurveParam[hovData.iTotPlot].fullName;
        setToolTip(lCurveParam[hovData.iTotPlot].fullName);
      }
    }
 */

//2) selezione dell'area di zoom
  if(zoomSelecting){  //sono in fase di selezione del rettangolo di zoom
    //endPos è la posizione finale per il rettangolo delle zoomate
    endZoomRectPos=event->pos();
    update();
    return;
  }

//3) gestione dei cursori di dati
   //A questo punto non sto selezionando il rettangolo di zoom. Adesso se sono in un'azione di dataRectDragging aggiorno i valori numerici e la posizione del cursore dati; altrimenti valuto se il mouse è nel raggio di azione di uno dei tre cursori dati. La cosa viene ripetuta tre volte, una per ogni cursore dati.
    dataCursSelecting=0;
    if(dataCursVisible){
      if(dataCursDragging){
        int rWidth=dataCurs.width(); //va calcolata volta per volta perché nel caso di ptBar la larghezza è calcolata dinamicamente
        setCursor(Qt::SizeHorCursor);
        //la seguente chiamata attribuisce un valore a nearX. Probabilmente il primo argomento dovrebbe sempre essere posX, ma per ora lascio la differenza in funzione del tipo di scala perché così funziona anche se non è stato chiarito il motivo. Sembra che vi sia all'interno di giveValues un errore che si compensa passando questo valore modificato.
        if(xAxis.scaleType==stLin)
          values=giveValues(posX-1, linearInterpolate, nearX, false, false);
        else
          values=giveValues(posX-(xStartIndex[0]-X0), linearInterpolate, nearX, false, false);
        if(nearX<X0)nearX=X0;
        if(nearX>X1) nearX=X1;
        dataCurs.moveLeft(nearX-(rWidth-1)/2);

        //Mando fuori i valori:
        emit valuesChanged(values,false,false);
      }else{
  //Se sono a meno di 5 pixel dall'eventuale cursore dati, cambio poi il cursore mouse per indicare la trascinabilità del cursore dati:
        if(posX>dataCurs.x()-5 && posX<dataCurs.x()+dataCurs.width()+5){
          setCursor(Qt::SizeHorCursor);
          dataCursSelecting=1;
        }else
           setCursor(Qt::ArrowCursor);
      }
    }

    if(dataCurs2Visible){
      if(dataCurs2Dragging){
        setCursor(Qt::SizeHorCursor);
        values=giveValues(posX-1, linearInterpolate, nearX, true, true);
        dataCurs2.moveLeft(nearX-1);
        //Mando fuori i valori:
        emit valuesChanged(values,true,true);
      }else{
  //Se sono a meno di 5 pixel dall'eventuale cursore dati, setto il corrispodente flag affinché in fondo alla routine si cambi poi il cursore mouse:
        if(posX>dataCurs2.x()-5 && posX<dataCurs2.x()+dataCurs2.width()+5){
          setCursor(Qt::SizeHorCursor);
          dataCursSelecting=2;
        }
      }
    } else{

    }
    update();
}


void CLineChart::mouseDoubleClickEvent(QMouseEvent *event){
  /* Il double click serve solo per introdurre il testo del titolo.
   *Esso andrà effettuato quando il titolo è visibile e all'interno dell'area titolo
  */
  bool ok;
  //Se il doppioclick non è nel rettangolo del titolo esco:
  if(!titleRectF.contains(event->pos()))return;
  //Se il titolo non è visibile esco
  if(!writeTitle1)return;

  QString text = QInputDialog::getText(this, "plot Title",
                       "enter title text:", QLineEdit::Normal,  titleText, &ok);
  if(ok)
    titleText=text;

/***
 Il seguente if per funzionare bene dovrebbe anche emettere un segnale di ditolo disabilitato, il quale dovrebbe essere poi catturato dalla scheda di PlotWin e di conseguenza disattivato il bottone del titolo.
Se non faccio questo il comportamento è passabile ma imperfetto: il titolo è disabilitato ma il bottone rimane premuto.
Questo per me è inaccettabile. Meglio lasciare un titolo bianco, che è quello che ha fatto l'utente di una miglioramento a metà.

  if(text.simplified()==""){
    titleText=QString("Double-Click here to set the title text!");
    disableTitle();
  }
*/
  zoomSelecting=false;
  resizeStopped();
}


void CLineChart::mousePressEvent(QMouseEvent *event){
/* Quando mi sposto con il mouse, se entro nel raggio d'azione di un dataCursor il puntatore del mouse diviene la doppia freccetta orizontale ed un eventuale click aggancia il dataCursor attivo alla posizione del mouse.
Questo aggancio viene comandato nella funzione mouseMoveEvent(), attraverso la selezione del valore opportuno per la variabile dataCursSelecting, la quale vale 0 se quando non siamo nel raggio d'azione di alcun cursore dati, altrimenti vale il numero del cursore dati: 1 per quello base, 2 per quello con le differenze.
  Pertanto la routine ha un comportamento che va distinto in funzione del valore della variabile dataCursSelecting.
*/
    /* Il seguente emit serve per evitare che se l'utente clicca nell'area LineChart di una PlotWin, dopo i primi click, non avviene più lo switch automatico della tab. Infatti l'evento focusInEvent, si attiva solo se il click avviene sulla riga di intestazione della finestra plotWIN,  o la prima volta che si clicca sull'area di LineChart. Le volte successive non si attiva; per fare lo switch della tab anche in questo caso, catturo dentro LineChart il comando mousePressEvent():
*/

    emit chartClickedOn();
    int Ret=0, x1=event->pos().x()-1;

    if(dataCursSelecting>0){  //sono nel raggio d'azione di un qualche cursore dati
      if(event->buttons() & Qt::RightButton)return;
      switch(dataCursSelecting){
        case 1:
          dataCursDragging=true;
          lastDataCurs=1;
          dataCurs.moveLeft(x1);
          /* In conseguenza di un baco non identificato nell'evento paint arrivano spessissimo valori di dataRect errati.
In attesa di comprendere la causa del problema copio il rettangolo in una copia di riserva sperando di tamponare così il problema*/
          debugCurs=dataCurs;

          break;
        case 2:
          dataCurs2Dragging=true;
          lastDataCurs=2;
          dataCurs2.moveLeft(x1);
          break;
        default:
          ;
      }
      mouseMoveEvent(event);
      return;
    }

    //Se arrivo a questo punto il mouse non è nel raggio d'azione di alcun cursore dati e la pressione viene associata alle funzioni di zoom.
    if((event->buttons() & Qt::RightButton) && zoomed){ //tasto destro: menù contestuale per dezoom
      QMenu myMenu;

     // Se ho semplice pressione del tasto destro dezoomo di un livello; se la pressione è associata al tasto CTRL, dezoomo completamente:
     if(QApplication::keyboardModifiers()&Qt::ControlModifier){
       exactMatch=false;
       while(!plStack.isEmpty())
         dispRect=plStack.pop();
      }else
        if(!plStack.isEmpty())
          dispRect=plStack.pop();

/*
      QAction * zoombackAct, *unzoomAct, *myAct;
      zoombackAct=myMenu.addAction("Zoom Back");
      unzoomAct=myMenu.addAction("Unzoom");
      myAct=myMenu.exec(event->globalPos());
      if(myAct==zoombackAct)
        dispRect=plStack.pop();
      if(myAct==unzoomAct){
        exactMatch=false;
        while(!plStack.isEmpty())  dispRect=plStack.pop();
      }
*/
      if(plStack.isEmpty())
        zoomed=false;
      Ret=scaleXY(dispRect,false); //Ritocca il valore di DispRect
      if(Ret) return;
      goPlot(false,false);
      return;
  }
  if(event->buttons() & Qt::LeftButton){  //tasto sinistro: inizio zoomata
    //Qui è stato premuto il bottone sinistro:
    zoomSelecting=true;
    stZoomRectPos=event->pos();
    endZoomRectPos=event->pos();
  }
}


void CLineChart::mouseReleaseEvent(QMouseEvent *ev)
{
  if(ev->buttons() & Qt::RightButton)
    return;

  setCursor(Qt::ArrowCursor);
  if(dataCursDragging){
    dataCursDragging=false;
    return;
  }
  if(dataCurs2Dragging){
    dataCurs2Dragging=false;
    return;
  }

  if(zoomSelecting){
    SFloatRect2 oldDispRect=dispRect;
    if(ev->x()<=stZoomRectPos.x() || ev->y()<=stZoomRectPos.y())
      goto Return;
    zoomed=true;
    forceYZero=false;
    dispRect=giveZoomRect(int(stZoomRectPos.x()), int(stZoomRectPos.y()),
                          int(ev->x()), int(ev->y()));
    //Qui devo comandare il grafico con i nuovi estremi
    scaleXY(dispRect,false);

    dispRect.Left =xAxis.scaleMin*xAxis.scaleFactor;
    dispRect.Right=xAxis.scaleMax*xAxis.scaleFactor;
/*
Il seguente if sarebbe pensato per evitare di fare un diagramma a barre contenente meno di 3 barre,il quale infatti ha poco senso.
Ciononostante così com'è non funziona, in quanto può benissimo accadere che dispRect.Right-dispRect.Left<3 e che fra tali due estermi il numero di barre sia superiore a 3.
pertanto il codice per ora è commentato Eventualmente potrà essere reintrodotto in modo corretto in un secondo momento.
*/
/*  if(plotType==ptBar && dispRect.Right-dispRect.Left<3){
      dispRect=oldDispRect;
      scaleXY(dispRect,false);
      QMessageBox::warning(this,tr("MC's PlotXWin"),tr("Zoom Rectangle too small!"),QMessageBox::Ok);
      goto Return;
    }
*/
    myCursor=Qt::BusyCursor;
    if(xAxis.done==0 || yAxis.done==0){
      QMessageBox::warning(this,"MC's PlotXWin","Unable to zoom so deeply.",QMessageBox::Ok);
      dispRect=oldDispRect;
      scaleXY(dispRect,false);
    }else{
      plStack.push(oldDispRect);
      goPlot(false,false);
//          ResetMarkData();
    }
/*La seguente riga per notificare all'esterno il cambiamento di stato.
Potrà essere sostituita con l'emissione di un Signal()*/
//        if(OnZoomStateChange) OnZoomStateChange(Owner,FZoomed);
  }
  Return:
  endZoomRectPos=stZoomRectPos;
  zoomSelecting=false;
//      update();

}

void  CLineChart::drawAllLabelsAndGrid(SAxis axis){
  /* Traccia:
   - tacche
   - etichette numeriche ("numLabels")
   - etichette di asse ("axisLabel")
   - griglia
   Da Ago 2017 è presente una funzione che rinuove la penultima label numerica se non vi è sufficiente spazio per scrivere l'unità di misura o, più probabile,la potenza di 10. E'una rimozione che si fa solo se il problema si verifica lungo l'asse x. Quando si fa questo si fa removeOneLbl=true.

 */
  bool removeOneNumLbl=false;
  int x, y, ticCount, yy;
  float xf, yf;
  float auxF;
  //char num[10], * format="%.1f";
  QString numStr;
  QPainterPath ticPath, gridPath;

  if(axis.scaleType!=stLin){
    if(axis.scaleType==stDB)
      drawAllLabelsAndGridDB(axis);
    else
      drawAllLabelsAndGridLog(axis);
    return;
  }
  /* Ora che ho rimandato le scale logaritmiche ad altri files, qui di seguito tratto
     solo le scale lineari.
  */

  if(axis.addZeroLine){
    int zeroPosition, initialPix, finalPix;
    if(axis.type==atX){
      initialPix=Y0;
      finalPix=Y1;
    }else{
      initialPix=X0;
      finalPix=X1;
    }
    if(axis.scaleMin*axis.scaleMax>=0)
      zeroPosition=-1;
    else{
      zeroPosition=NearInt(axis.scaleMax/(axis.scaleMax-axis.scaleMin)*(Y1-Y0)+Y0);
    }
    ticPath.moveTo(initialPix,zeroPosition);
    ticPath.lineTo(finalPix,zeroPosition);
  }

  if(axis.type==atX){
    // Tacche e numeri.
    //Per prima cosa vedo se devo rimuovere una tacca.
    int axisLabelLen=writeAxisLabel(0,0,axis,true);
    //rimuovo euristicamente la tacca se la larghezza di ticinterval.x non è almeno pari alla larghezza in pixel della axislabel aumentata del 50%. Altrimento dovrei fare calcoli molto articolati per un fatto tutto sommato secondario.
    if(axisLabelLen*1.5f>axis.ticIntervalPix)
      removeOneNumLbl=true;

    myPainter->setPen(txtPen);
    ticCount=0;  //vale 0 per le tacche dotate di label numerica, 1 per le tacche prive di essa
    // Il seguente margin è diverso da 0 solo per i diagrammi a barre
    for(x=X0+margin, auxF=X0+margin, xf=xAxis.scaleMin; x<=X1-margin;
            auxF+=axis.ticIntervalPix, x=NearInt(auxF), xf+=xAxis.ticInterval) {
      ticPath.moveTo(x,yAxis.widthPix+Y0+1);
      ticPath.lineTo(x,yAxis.widthPix+Y0+xAxis.ticPixWidth+1);
      numStr=QString::number(double(xf),'g',4);
      if(ticCount/2*2==ticCount){
        //Scrivo la label numerica, omettendola se c'è poco spazio per la label di asse e sto scrivendo la penultima tacca.  Individuo che si tratta della penultima tacca con la condizione che la x è compresa fra X1-margin-1.5*aXis.ticIntervalPix.y e X1-Margin-0.5*aXis.ticIntervalPix.x
        if (!removeOneNumLbl)
          writeText2(myPainter,x,Y1+xAxis.ticPixWidth,atCenter,atLeft,numStr,"",false,false);
        if(removeOneNumLbl &&
              (x<X1-margin-1.5f*axis.ticIntervalPix || x>X1-margin-0.5f*axis.ticIntervalPix))
          writeText2(myPainter,x,Y1+xAxis.ticPixWidth,atCenter,atLeft,numStr,"",false,false);
      }
      if(xAxis.halfTicNum)ticCount++;
    }
    // ***
    // Ora scrittura della label di asse X
    //La seguente riga mette la label di asse al centro fra due tic, se non sono in halfTicNum, nel qual caso viene spostata un poco a sinistra ma non tanto da sembrare posta proprio in corrispondenza della "halftic".
    int xAxisLabelx=X1-int((0.5f+0.1f*xAxis.halfTicNum)*axis.ticIntervalPix);
    if(removeOneNumLbl)
        xAxisLabelx-=int(axis.ticIntervalPix/2.f);
    writeAxisLabel(xAxisLabelx, Y1+xAxis.ticPixWidth,xAxis,false);
    //Eventuale griglia:
    for(auxF=axis.ticIntervalPix, x=NearInt(auxF); x<xAxis.widthPix-1;
             auxF+=axis.ticIntervalPix, x=NearInt(auxF)) {
      if(xGrid) {
        gridPath.moveTo(x+X0,Y0);
        gridPath.lineTo(x+X0,Y0+yAxis.widthPix);
      }
    }
  } else {   // *******  Casi assi atYL e atYR
    // Tacche e label numeriche:
    ticCount=0;
     for(y=yAxis.widthPix+Y0, auxF=float(yAxis.widthPix+Y0),  yf=axis.scaleMin; y>=Y0-2;
               auxF-=axis.ticIntervalPix , y=NearInt(auxF), yf+=axis.ticInterval)   	  {
      if(axis.type==atYR){ //caso di scala destra
        ticPath.moveTo(X1+1,y);
        ticPath.lineTo(X1+1+yAxis.ticPixWidth,y);
      }else{
         ticPath.moveTo(X0-1,y);
         ticPath.lineTo(X0-1-yAxis.ticPixWidth,y);
      }
      if(fontSizeType==fsFixed) myPainter->setFont(QFont(baseFontName,fixedFontPx));
       numStr=QString::number(double(yf),'f',axis.ticDecimals);
       if(ticCount/2*2==ticCount){
        if(axis.type==atYR){
          writeText2(myPainter,X1+yAxis.ticPixWidth,y,atLeft,atCenter,numStr,"",false,false);
        }else{
          writeText2(myPainter,X0-yAxis.ticPixWidth+1,y,atRight,atCenter,numStr,"",false,false);
        }
      }
      if(axis.halfTicNum)ticCount++;
    }
    // ***
    // Labels di asse (YL e YR):
    if(axis.halfTicNum)
      yy=Y0+int(0.9f*axis.ticIntervalPix);
    else
      yy=Y0+int(0.5f*axis.ticIntervalPix);
//    if(axis.type==atYR)
//      writeAxisLabel(X1+0.5*(1+axis.halfTicNum)*yAxis.ticPixWidth ,yy, axis,false);
//    else{
//      writeAxisLabel(X0-0.5*(1+axis.halfTicNum)*yAxis.ticPixWidth , yy, axis,false);
//    }
    if(axis.type==atYR)
      writeAxisLabel((width()+X1)/2, yy, axis,false);
    else{
      writeAxisLabel(X0/2,yy, axis,false);
    }
    //eventuale griglia (solo per la scala di sinistra)
    if(yGrid && axis.type!=atYR){
      for(y=yAxis.widthPix+Y0, auxF=float(yAxis.widthPix+Y0),  yf=axis.scaleMin; y>=Y0;
      auxF-=axis.ticIntervalPix, y=NearInt(auxF), yf+=axis.ticInterval) {
        if(y!=yAxis.widthPix+Y0 && y!=Y0) {
          gridPath.moveTo(X0,y);
//          gridPath.lineTo(X0+xAxis.width-2,y);
          gridPath.lineTo(X1,y);
        }
      }
    }  //fine if(Axis.Grid)
  } //fine casi assi atYL e atYR
  myPainter->setPen(ticPen);
  myPainter->drawPath(ticPath);
  myPainter->setPen(gridPen);
  myPainter->drawPath(gridPath);
}

//---------------------------------------------------------------------------
void  CLineChart::drawAllLabelsAndGridDB(SAxis axis){
/* Versione della funzione LabelsAndGrid relativa a scale logaritmiche tarate in dB
 */
  int i, xx, yy,
    pos[MAXLOGTICS], //Posizioni lungo l'asse considerato delle varie tacche
    pos1, pos0, //sono X1-X0 ovvero Y1-Y0
    pos10, //=Pos1-Pos0
    numTics=0,
    numTicType; //Se è 1 metto solo tacche agli estremi della scala; se 2 ogni 20 db, se 3 ogni 10 db
  char num[10];
  float  DBStep=0.f,value0,value;     //distanza in stDB fra due tacche consecutive

  QPainterPath ticPath, gridPath;

  myPainter->setFont(QFont(baseFontName,generalFontPx));
  if(axis.type==atX){
    pos0=X0;
    pos1=X1;
  }else{
    pos0=Y0;
    pos1=Y1;
  }
  pos10=pos1-pos0;
  // Inizialmente ipotizzo di mettere tacche ogni 20 db:
  numTicType=2;
  //Se c'è poco spazio metto solo tacche agli estremi:
  if( pos10/(axis.eMax-axis.eMin) < 2*fontMetrics().height() ) numTicType=1;
  //Se c'è spazio metto tacche ogni 10 db, purché non siano in numero eccessivo:
  if(axis.eMax-axis.eMin <= MAXLOGTICS/3        &&
       pos10/(axis.eMax-axis.eMin) > 5.5*fontMetrics().height() ) numTicType=3;

  //Ora devo compilare il vettore con le posizioni delle tacche
  // (dall'alto verso il basso ovvero da sinistra a destra)
  pos[0]=pos0;
  switch(numTicType){
    case 1:
      DBStep=20.f*(axis.eMax-axis.eMin);
      axis.ticInterval=axis.widthPix;
      pos[1]=pos1;
      numTics=2;
      break;
    case 2:
      DBStep=20.f;
      axis.ticInterval= float(pos10+1)/(axis.eMax-axis.eMin);
      for(i=1; i<=axis.eMax-axis.eMin; i++){
        pos[i]=pos[i-1]+int(axis.ticInterval);
      }
      numTics=(axis.eMax-axis.eMin)+1;
      break;
    case 3:
      DBStep=10.f;
      axis.ticInterval= float(pos10+1)/(axis.eMax-axis.eMin)/2.f;
      for(i=1; i<=2*(axis.eMax-axis.eMin); i+=2){
        pos[i]=pos[0]+i*int(axis.ticInterval);
        pos[i+1]=pos[0]+(i+1)*int(axis.ticInterval);
      }
      //Per gli arrotondamenti ci può essere un errore massimo di 1 pixel sull'ultima
      //tacca e lo correggo:
      if(abs(pos[i-1]-pos1)==1)  pos[i-1]=pos1;
      numTics=2*(axis.eMax-axis.eMin)+1;
      break;
  }

  //Scrittura tacche e label numeriche:
  value0=20.f*axis.eMin;
  for(i=0; i<numTics; i++){
    // Tacche:
    if(axis.type==atX){
      ticPath.moveTo(pos[i],Y1+1);
      ticPath.lineTo(pos[i],Y1+xAxis.ticPixWidth);
    }else if(axis.type==atYL){
      ticPath.moveTo(X0,pos[i]);
      ticPath.lineTo(X0-yAxis.ticPixWidth,pos[i]);
    }else{
      ticPath.moveTo(X1,pos[i]);
      ticPath.lineTo(X1-yAxis.ticPixWidth,pos[i]);
    }
    value=value0+i*DBStep;
    sprintf(num,"%.0f",double(value));
    //Valore numerico:
    if(axis.type==atX){
        writeText2(myPainter,pos[i],Y1+xAxis.ticPixWidth,atCenter,atLeft,num,"",false,false);
    }else if(axis.type==atYL){
        writeText2(myPainter,X0-yAxis.ticPixWidth+1,pos[numTics-i-1],atRight,atCenter,num,"",false,false);
    }else{
        writeText2(myPainter,X1+smallHSpace,  pos[numTics-i-1],atLeft,atCenter,num,"",false,false);
    }
  }
  //eventuale griglia (asse y solo per la scala di sinistra)
  myPainter->setPen(gridPen);
  if(axis.type==atX){
    if(xGrid){
      for(i=1; i<numTics-1; i++){
        gridPath.moveTo(pos[i],Y0);
        gridPath.lineTo(pos[i],Y1);
      }
    }
  }else if(axis.type==atYL){
    if(yGrid){
       for(i=1; i<numTics-1; i++){
        gridPath.moveTo(X0,pos[i]);
        gridPath.lineTo(X1,pos[i]);
      }
    }
  }else
    goto Return;
  //Label "dB":
  if(axis.type==atX){
    xx=X1-int(axis.ticInterval/2.0f);
    yy=Y1+xAxis.ticPixWidth;
  }else{
    //Si ricordi che la label nel caso di assi sinistri (come questo) ha allineamento centrato.
    //L'eventualità che la label caschi in concidenza con un tick è trascurabile, quindi centro come se non vi fosse:
    //xx=(X0-yAxis.ticPixWidth)/2;
    xx=X0/2;
    yy=pos0+int(axis.ticInterval/2.0f);
    if(numTicType==1)
        yy=pos0+int(axis.ticInterval/3.5f);
  }
  writeAxisLabel(xx,yy, axis,false);

  Return:
  myPainter->setPen(gridPen);
  myPainter->drawPath(gridPath);
  myPainter->setPen(ticPen);
  myPainter->drawPath(ticPath);
}

//---------------------------------------------------------------------------
void CLineChart::drawAllLabelsAndGridLog(SAxis axis){
/* Versione della funzione LabelsAndGrid relativa a scale logaritmiche tarate in
 * valori effettivi della variabile prima dell'effettuazione del logaritmo
 * (scale "stLog")
 * Traccia:
 *  - tacche
 * - etichette numeriche ("numLabels")
 * - etichette di asse ("axisLabel")
 * - griglia
*/
  int i, dec, //indice di decade
    tic, //indice di tacca all'interno della decade
    pos[MAXLOGTICS],
    pos1, pos0, //sono X1-X0 ovvero Y1-Y0
    pos10, //=Pos1-Pos0
    numTicType; //Se è 1 metto solo tacche agli estremi della scala; se 2 ogni decade,
           //se 3 ogni due unità interne alla decade
  float decInterval=0; //spaziatura in pixel fra le tacche di due decadi consecutive
  char num[10];
  QPainterPath ticPath, gridPath;

  myPainter->setPen(ticPen);
  myPainter->setFont(QFont(baseFontName,generalFontPx));

  if(axis.type==atX){
    pos0=X0;
    pos1=X1;
  }else{
    pos0=Y0;
    pos1=Y1;
  }
  pos10=pos1-pos0;
  // Inizialmente ipotizzo di mettere tacche ogni decade:
  numTicType=2;
  //Se c'è poco spazio metto solo tacche agli estremi:
  if( pos10/(axis.eMax-axis.eMin) < 2*fontMetrics().height() ) numTicType=1;
  //Se c'è spazio metto tacche ogni due unità interne alla decade,
  //purché non si superino le 5 decadi:
  if(pos10/(axis.eMax-axis.eMin) > 5*fontMetrics().height() && axis.eMax-axis.eMin<6)
    numTicType=3;
  //Ora devo compilare il vettore "pos" con le posizioni delle tacche (dal basso verso l'alto, da sinistra a destra)
  pos[0]=pos1;  // nel caso di asse y la posizione di partenza è Y1
  if(axis.type==atX) pos[0]=pos0; // nel caso di asse x la posizione di partenza è X0
  switch(numTicType){
   case 1:
      pos[1]=pos0;
//      decInterval=pos[0]-pos[1];
      break;
    case 2:
      decInterval= float(pos10+1)/(axis.eMax-axis.eMin);
      for(dec=1; dec<axis.eMax-axis.eMin; dec++){
         pos[dec]=pos[0]+int((2*(axis.type==atX)-1)*dec*decInterval);
      }
      if(axis.type==atX)
        pos[axis.eMax-axis.eMin]=pos1;
      else
        pos[axis.eMax-axis.eMin]=pos0;
      break;
    case 3:
      decInterval= float(pos10+1)/(axis.eMax-axis.eMin);
      for(dec=0; dec<axis.eMax-axis.eMin; dec++){
        pos[5*dec]=pos[0]+int((2*(axis.type==atX)-1)*dec*decInterval);
        for(tic=1; tic<5; tic++){
//          pos[5*dec+tic]=pos[5*dec]+int((2*(axis.type==atX)-1)*log10f((float)2*tic)*decInterval);
          pos[5*dec+tic]=pos[5*dec]+int((2*(axis.type==atX)-1)*log10f(float(2*tic))*decInterval);
        }
      }
      pos[5*(axis.eMax-axis.eMin)]=pos0+(axis.type==atX)*(pos1-pos0);
      break;
  }

  //predisposizione path tacche, tracciamento relativi numeri, ed eventuale griglia
  switch(numTicType){
  case 1:
    for(i=0; i<2; i++){
      if(axis.type==atX){
        ticPath.moveTo(pos[i],Y1);
        ticPath.lineTo(pos[i],Y1+xAxis.ticPixWidth);
        writeText2(myPainter,pos[i],Y0-xAxis.ticPixWidth,atCenter,atLeft,"10",num,false,false);
      }else if(axis.type==atYL){
        ticPath.moveTo(X0-yAxis.ticPixWidth,pos[i]);
        ticPath.lineTo(X0+1,   pos[i]);
        sprintf(num,"%d",axis.eMin+i*(axis.eMax-axis.eMin));
        writeText2(myPainter,X0-yAxis.ticPixWidth,pos[i],atRight,atCenter,"10",num,false,false);
      }else{ //caso atYR
        ticPath.moveTo(X1-yAxis.ticPixWidth,pos[i]);
        ticPath.lineTo(X1,   pos[i]);
        sprintf(num,"%d",axis.eMin+i*(axis.eMax-axis.eMin));
        writeText2(myPainter,X1+smallHSpace,pos[i],atLeft,atCenter,"10",num,false,false);
      }
    }
    break;
  case 2:
    for(dec=0; dec<=axis.eMax-axis.eMin; dec++){
      if(axis.type==atX){
        ticPath.moveTo(pos[dec],Y1);
        ticPath.lineTo(pos[dec],Y1+xAxis.ticPixWidth);
        if(xGrid && dec>0 && dec<axis.eMax-axis.eMin){
          gridPath.moveTo(pos[dec],Y0);
          gridPath.lineTo(pos[dec],Y1);
        }
        ticPath.moveTo(pos[dec],Y1);
        ticPath.lineTo(pos[dec],Y1+xAxis.ticPixWidth);
        sprintf(num,"%d",axis.eMin+dec);
        writeText2(myPainter,pos[dec],Y1+xAxis.ticPixWidth,atCenter,atLeft,"10",num,false,false);
      }else if(axis.type==atYL){
        ticPath.moveTo(X0-yAxis.ticPixWidth,pos[dec]);
        ticPath.lineTo(X0+1,   pos[dec]);
        sprintf(num,"%d",axis.eMin+dec);
        writeText2(myPainter,X0-yAxis.ticPixWidth,pos[dec],atRight,atCenter,"10",num,false,false);
        if(yGrid && dec>0 && dec<axis.eMax-axis.eMin){
          //Non faccio la riga in corrispondenza della posizione 10^0 di diagrammi
          //a barre con scala verticale logaritmica per evitare che per errori di
          //arrotondamento restino visibili sia la riga continua di base delle barre
          //che la linea tratteggiata della griglia:
          if(!(plotType==ptBar && axis.scaleType!=stLin && axis.eMin+dec==0)){
            gridPath.moveTo(X0,pos[dec]);
            gridPath.lineTo(X1,pos[dec]);
          }
        }
      }else{ //caso atYR
        ticPath.moveTo(X0-yAxis.ticPixWidth,pos[dec]);
        ticPath.lineTo(X0+1,   pos[dec]);
        sprintf(num,"%d",axis.eMin+dec);
        writeText2(myPainter,X1+smallHSpace,pos[dec],atLeft,atCenter,"10",num,false,false);
      }
   }
   break;
  case 3:
   for(dec=0; dec<=axis.eMax-axis.eMin; dec++){
     sprintf(num,"%d",axis.eMin+dec);
     if(axis.type==atX){
       ticPath.moveTo(pos[5*dec],Y1);
       ticPath.lineTo(pos[5*dec],Y1+xAxis.ticPixWidth);
       writeText2(myPainter,pos[5*dec],Y1+xAxis.ticPixWidth,atCenter,atLeft,"10",num,false,false);
     }else if(axis.type==atYL){
       ticPath.moveTo(X0-yAxis.ticPixWidth,pos[5*dec]);
       ticPath.lineTo(X0+1,   pos[5*dec]);
       writeText2(myPainter,X0-yAxis.ticPixWidth,pos[5*dec],atRight,atCenter,"10",num,false,false);
     } else{ //case atYR
       ticPath.moveTo(X1-yAxis.ticPixWidth,pos[5*dec]);
       ticPath.lineTo(X1,   pos[5*dec]);
       writeText2(myPainter,X1+smallHSpace,pos[5*dec],atLeft,atCenter,"10",num,false,false);
     }
     for(tic=0; tic<5; tic++){
       if(dec==axis.eMax-axis.eMin)break;
       //Scrittura delle tacche intermedie fra le decadi intere ed eventuale griglia:
        if(axis.type==atX){
          if(xGrid){
             gridPath.moveTo(pos[5*dec+tic],Y0);
             gridPath.lineTo(pos[5*dec+tic],Y1);
           }else{
             ticPath.moveTo(pos[5*dec+tic],Y0);
             ticPath.lineTo(pos[5*dec+tic],Y0+xAxis.ticPixWidth);
             ticPath.moveTo(pos[5*dec+tic],Y1);
             ticPath.lineTo(pos[5*dec+tic],Y1-xAxis.ticPixWidth);
           }
         }else if(axis.type==atYL){
           //a barre con scala verticale logaritmica per evitare che per errori di
           //arrotondamento restino visibili sia la riga continua di base delle
           //barre che la linea tratteggiata della griglia:
           if(tic!=0 || !(plotType==ptBar && axis.eMin+dec==0)){
             if(yGrid){
               gridPath.moveTo(X0,pos[5*dec+tic]);
               gridPath.lineTo(X1,pos[5*dec+tic]);
             }else{
               ticPath.moveTo(X0,pos[5*dec+tic]);
               ticPath.lineTo(X0+yAxis.ticPixWidth,pos[5*dec+tic]);
               if(!twinScale){
                 ticPath.moveTo(X1,pos[5*dec+tic]);
                 ticPath.lineTo(X1-yAxis.ticPixWidth,pos[5*dec+tic]);
               }
             }
          }
        } else { //caso atYR
          if(tic!=0 || !(plotType==ptBar && axis.eMin+dec==0)){
            ticPath.moveTo(X1,pos[5*dec+tic]);
            ticPath.lineTo(X1-yAxis.ticPixWidth,pos[5*dec+tic]);
          }
        } //fine if(Axis.Type==atX)
      } //fine for(tic=0 ...
    } //fine for(dec=0 ...
    break;
  } //fine switch

  myPainter->setPen(gridPen);
  myPainter->drawPath(gridPath);
  myPainter->setPen(ticPen);
  myPainter->drawPath(ticPath);

  //Label di asse (da aggiungere)

}


void CLineChart::paintEvent(QPaintEvent *ev)
{
    QPainter painter(this);
    QRect dirtyRect = ev->rect();
    painter.drawImage(dirtyRect, *myImage);

    QColor  selCol(255,0,0,80),
            dataCol (100,100,100,120), // Primo cursore, bianco 100/255, opaco 120/255
            dataCol2(020,255,020,120); // Secondo cursore, verde, opaco 120/255
    if(plotType==ptBar)
        dataCol=QColor(100,100,100,180);  //Cursore bar, bianco 100/255, opaco 180/255
    QBrush selBrush(selCol), dataBrush(dataCol), dataBrush2(dataCol2);
    painter.setBrush(selBrush);
    if(zoomSelecting)
        painter.drawRect(int(stZoomRectPos.x()), int(stZoomRectPos.y()),
                         int(endZoomRectPos.x()-stZoomRectPos.x()),
                         int(endZoomRectPos.y()-stZoomRectPos.y()));
    if(dataCursVisible){
        painter.setBrush(dataBrush);
        painter.drawRect(dataCurs);
    }
    if(dataCurs2Visible){
        painter.setBrush(dataBrush2);
        painter.drawRect(dataCurs2);
    }
        painter.setBrush(QBrush(QColor(200,200,200,80)));
        painter.drawRect(hovVarRect);

        QBrush brush(QColor(250,0,0));
        tooltipRect.setWidth(5+2*(plotPen.width()>1));
        tooltipRect.setHeight(5+2*(plotPen.width()>1));
        painter.setBrush(brush);
        if(rectTTVisible)
          painter.drawRect(tooltipRect);
}

void CLineChart::copy(){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setImage(*myImage);
    if(showPlotCopiedDlg)
      QMessageBox::information(this,"CLineChart","plot copied as an image into the system clipboard.");
}

QImage *  CLineChart::giveImage(){
    //Questa funzione è utile per creare, ad esempio nella realizzazione di spettri di fourier, immagini contenenti più grafici.
    return myImage;
}


QString CLineChart::makePng(QString fullName, bool issueMsg){
    bool ok=false;
    QString ret=tr("Error");
    QPixmap pixmap;
    pixmap=pixmap.fromImage(*myImage);
    ok=pixmap.save(fullName);
    if(ok)ret="";
    if(issueMsg&&ok)
     QMessageBox::information(this,"CLineChart",tr("PNG image successfully created and saved into file\n")+fullName);
    return ret;
}

QString CLineChart::makeSvg(QString fullName, bool issueMsg){
    QString ret="";
    QSvgGenerator generator;

    generator.setFileName(fullName);
    //I seguenti fattori 0,85 non ci dovrebbero essere. Non è chiaro perché con essi il file SVG viene più centrato.
    generator.setSize(QSize(int(0.85f*geometry().width()), int(0.85f*geometry().height())));

      myPainter->end();
      myPainter->begin(&generator);

/* La seguente riga è sperimentale, e potrebbe anche essere inutile.
L'idea è di ritoccare dispRect in quanto il font SVG può essere diverso da quello a schermo
Ad es. a schermo uso un 25 point ma il mio paint device ne consente 24; oppure i points sono differenti ma le misure di larghezza di testo sono differenti a causa di diversa grafica del font.
*/
//    scaleXY(dispRect,false);

    goPlot(false,false);
    markAll();
    myPainter->begin(myImage);
    //Non so perché il grafico a questo punto è scomparso dallo schermo. Lo ritraccio:
    resizeStopped();
    if(issueMsg)
       QMessageBox::information(this,"CLineChart","SVG drawing successfully created and saved into file\n"+fullName);
    update();
    return ret;
}


void CLineChart::mark(void){
    /*Function per il tracciamento di marcatori in corrispondenza dei nomi delle
    variabili e sui grafici in corrispondenza, come ascissa, dell'ascissa di CursorShape.
    */
  mark(true);
}


//---------------------------------------------------------------------------
void CLineChart::mark(bool store){
  /*Function per il tracciamento di marcatori in corrispondenza dei nomi delle variabili e sui grafici in corrispondenza, come ascissa, dell'ascissa di dataCurs[0].
  */
  bool someMark=false;
  int X=dataCurs.x(),iFile,iPlot,iTotPlot=0,iVSFile=0;
  //Traccio i marcatori in corrispondenza dei nomi delle variabili:
  for(iFile=0; iFile<nFiles; iFile++)
  for(iPlot=0; iPlot<nPlots[iFile]; iPlot++) {
     drawMark(float(markPositions[iTotPlot].x()), float(markPositions[iTotPlot].y()), iTotPlot,true);
     iTotPlot++;
  }
  iTotPlot=0;
  if(store)
    if(++manuMarks.lastMark==MAXMANUMARKS){
      manuMarks.lastMark--;
      QApplication::beep();
      return;
    }
  //Traccio i marcatori sulle curve:
  for(iFile=0; iFile<nFiles; iFile++){
    for(iPlot=0; iPlot<nPlots[iFile]; iPlot++)	{
      if(X>=xStartIndex[iFile] && X<xStopIndex[iFile]){
        someMark=true;
        markSingle(iFile, iVSFile, iPlot, iTotPlot, store);
      }else if (store){
        manuMarks.indexSX[iFile][manuMarks.lastMark]=-1;
      }
      iTotPlot++;
    }
    if(filesInfo[iFile].variableStep) iVSFile++;
  }
  if(!someMark)  QApplication::beep();
  update();

}


//---------------------------------------------------------------------------
void CLineChart::markSingle(int iFile, int iVSFile, int iPlot, int iTotPlot, bool store){
    /*Function per il tracciamento di un unico marcatore in corrispondenza, come ascissa,  dell'ascissa di dataCursor.
  E' in sostanza una versione con funzioni più limitate di Mark(bool), ma che consente un accesso più diretto alle operazioni; è più scomoda da usare di Mark(bool) per via dei numerosi parametri passati.
  Parametri passati:
  - iFile è l'indice del file correntemente considerato
  - iVSFile è l'indice del file, se a spaziatura variabile, all'interno dell'elenco dei files a spaziatura variabile
  - iPlot è l'indice del grafico all'interno dell'elenco dei grafici del file iFile
  - iTotPlot è l'indice generale del grafico e serve per scegliere l'aspetto del marcatore
  - Se store è true la posizione del marcatore tracciato è memorizzata in modo da facilitare il ritracciamento in caso, as es. di resize della finestra.
  La memorizzazione della posizione viene effettuata memorizzando l'indice del punto  immediatamente a sinistra del punto del cursore, e poi la frazione dell'intervallo  fra esso e l'indice immediatamente a destra. In tal modo la memorizzazione è indipendente dal Paint del supporto di visualizzazione, e quindi può essere utilizzata senza conversioni per tracciare a stampa i marcatori.
    */
    int cursorX=dataCurs.x(), Y, indexSX, netCursorX;
    float yMinF, yValue, yRatio,
            fCursorX, //valore numerico float della variabile x corrispondente alla posizione del cursore (usato solo per scale stLin).
            slope;

  /* NOTA IMPORTANTE
   * E' stato visto per tentaticvi che nel caso di scale logaritmiche per far funzionare
   * bene il markSingle occorre togliere a cursorX xStartIndex[0]-X0 per calcolare
   * indexSX, e poi riaggiungerlo sufccessivamente all'atto della chiamata a drawMark.
   * Questo evidentemente è dovuto ad un problema di fillPixelToIndexLog(), che però
   * non sono riuscito ad individuare.
   * Una volta risolto quel problema dovrò anche togliere quest'artificio da questa
   * chiamata a funzione.
   * Al momento non funziona bene lo spostamento del cursore con la tastiera nel caso
   * di scale logaritmiche e inizio della curva a destra del lato sinistro del rettangolo.
   * Anche lì dovrò usare un arfiticio.
   * Riassumento il problema non risolto di fillPixelToIndexLog() ha generato i
   * seguenti artifici:
   * 1) l'artificio appena descritto nella presente routine
   * 2) analogo offset nella chiamata a giveValues all'interno della routine mouse
   *    MoveEvent nel caso di scale non lineari
   * 3) analogo artificio nella chiamata a giveValues in keyPressEvent().
   * A QUESTO PUNTO S'E' CAPITO  che il fillPixelToIndexLog ha una traslazione di
   * xStartIndex[0]-X0. Basterà correggere questo problema là, ed elimiare i tre artifici
   * sopra elencati.
   */

   if(xAxis.scaleType!=stLin)
      cursorX-=xStartIndex[iVSFile]-X0;

    netCursorX=qMin(qMax(cursorX-X0,0),X1-X0);

    fCursorX=xAxis.scaleMin*xAxis.scaleFactor+netCursorX/xAxis.pixPerValue ;
  if(filesInfo[iVSFile].variableStep )
    indexSX=qMax(0,pixelToIndexDX[iVSFile][netCursorX]-1);
  else{
    indexSX=int((fCursorX-px[iFile][0])/(px[iFile][1]-px[iFile][0]));
  }

    if(cursorX<xStartIndex[iFile] || cursorX>=xStopIndex[iFile]){
    return;
  }
  if(curveParamLst[iTotPlot].rightScale){
    yMinF=ryAxis.minF;
    yRatio=ryAxis.pixPerValue ;
  }else{
    yMinF=yAxis.minF;
    yRatio=yAxis.pixPerValue ;
  }
  slope=(py[iFile][iPlot][indexSX+1]-py[iFile][iPlot][indexSX])/
                  (px[iFile][indexSX+1]-px[iFile][indexSX]); /*Slope=DeltaY/DeltaX:*/
  // nel caso di scala logaritmica la slope andrebbe valutata con formula differente. Per ora la elimino semplicemente:
  if(xAxis.scaleType!=stLin)
      slope=0;

  yValue=py[iFile][iPlot][indexSX]+ slope*(fCursorX-px[iFile][indexSX]);


  Y=Y0+yAxis.widthPix-NearInt(yRatio*(yValue - yMinF));
  if(store){
    manuMarks.indexSX[iFile][manuMarks.lastMark]=indexSX;
    manuMarks.fraction[iFile][manuMarks.lastMark]=
           (fCursorX-px[iFile][indexSX])/(px[iFile][indexSX+1]-px[iFile][indexSX]);
  }
  //Traccio il marcatore sul grafico:
  if(xAxis.scaleType==stLin)
    drawMark(cursorX+1,Y,iTotPlot,false);//il "+1" perché il dataCurs ha larghezza 3 e il valore è nel pixel centrale
  else
    drawMark(cursorX+xStartIndex[0]-X0,Y,iTotPlot,false);//il "+1" perché il dataCurs ha larghezza 3 e il valore è
}


void CLineChart::markAll(){
  /* Funzione utilizzata per Resize, Print e Copy, per mettere i marcatori sul grafico
     in tutte le posizioni in cui essi sono presenti a schermo.
  */
  int i, iPlot, iMark, iTotPlot=0, X, Y;
  float yMinF, yRatio;

  //La seguente riga serve per evitare che vengano messi i marcatori vicino ai nomi
  //delle variabili se non è da farsi marcatura né manuale né automatica
  if(!autoMark && manuMarks.lastMark==-1)return;
  if(autoMark){
    markAuto();
  }else{
    //Traccio i marcatori vicino ai nomi delle variabili:
    for(i=0; i<nFiles; i++)
      for(iPlot=0; iPlot<nPlots[i]; iPlot++){
        drawMark(float(markPositions[iTotPlot].x()), float(markPositions[iTotPlot].y()), iTotPlot,true);
        iTotPlot++;
      }
    iTotPlot=0;
  }
  yRatio=yAxis.pixPerValue ;
  for(i=0; i<nFiles; i++)
  for(iPlot=0; iPlot<nPlots[i]; iPlot++) {
    /* Ora traccio eventuali punti a posizione determinata manualmente.
  La memorizzazione della posizione dei punti è stata effettuata memorizzando l'indice
  del punto immediatamente a sinistra del punto del cursore, e poi la frazione
  dell'intervallo fra esso e l'indice immediatamente a destra. In tal modo la
  memorizzazione è indipendente dalla Canvas del supporto di visualizzazione, e
  quindi può essere utilizzata senza conversioni per tracciare a stampa i marcatori.
    */
    if(curveParamLst[iTotPlot].rightScale){
      yMinF=ryAxis.minF;
      yRatio=ryAxis.pixPerValue ;
    }else{
      yMinF=yAxis.minF;
    }
    if(manuMarks.lastMark>-1)
      for(iMark=0; iMark<=manuMarks.lastMark; iMark++){
        if(manuMarks.indexSX[i][iMark]<startIndex[i] || manuMarks.indexSX[i][iMark]>=stopIndex[i])  continue;
        if(manuMarks.indexSX[i][iMark]<0)  continue;
        int indexSX=manuMarks.indexSX[i][iMark];
        float Slope=(py[i][iPlot][indexSX+1]-py[i][iPlot][indexSX])/
                    (px[i][indexSX+1]-px[i][indexSX]); /*Slope=DeltaY/DeltaX:*/
        float yValue=py[i][iPlot][indexSX] + manuMarks.fraction[i][iMark]*Slope*(px[i][indexSX+1]-px[i][indexSX]);
        float xValue=px[i][indexSX] +
              manuMarks.fraction[i][iMark]*(px[i][indexSX+1]-px[i][indexSX]);
        X=X0+NearInt(xAxis.pixPerValue  * (xValue - xAxis.minF));
        Y=Y0+yAxis.widthPix-NearInt(yRatio*(yValue - yMinF));
        drawMark(X,Y,iTotPlot,false);
      }
        iTotPlot++;
    }
}

//---------------------------------------------------------------------------
void CLineChart::markAuto(){
  /* Funzione per mettere i marcatori in posizioni automaticamente fissate:
  Per una data ascissa media, i marcatori non li metto esattamente in corrispondenza
  della stessa ascissa, per evitare sovrapposizioni, ma ad ascisse separate
  orizzontalmente di circa 1.5*MarkWidth pixel.
  */
  int i, iPlot, iTotPlot=0, iVSFile=0, indexRange, index, deltaIndex, X, Y;
  float yMinF, yRatio;

  //La seguente riga serve per memorizzare che ci sono i marcatori nelle posizioni
  //automaticamente calcolate, per l'eventuale ritracciamento a seguito di comando
  //di Copy o Print.
  autoMark=true;

  float auxF=float(1./(MAXAUTOMARKS));
  for(i=0; i<nFiles; i++){
    for(iPlot=0; iPlot<nPlots[i]; iPlot++)	{
      //Traccio i marcatori vicino ai nomi delle variabili:
      drawMark(float(markPositions[iTotPlot].x()), float(markPositions[iTotPlot].y()), iTotPlot,true);
      indexRange=stopIndex[i]-startIndex[i];
      if(curveParamLst[iTotPlot].rightScale){
        yMinF=ryAxis.minF;
        yRatio=ryAxis.pixPerValue ;
      }else{
        yMinF=yAxis.minF;
        yRatio=yAxis.pixPerValue ;
      }
      //Traccio eventuali marcatori equispaziati in posizioni determinate automaticamente:
      if(filesInfo[i].variableStep  && xVarParam.isMonotonic){
        int xInitial, xFinal, stepX;
        xInitial=X0+NearInt(xAxis.pixPerValue  * (px[i][startIndex[i]] - xAxis.minF));
        xFinal  =X0+NearInt(xAxis.pixPerValue  * (px[i][stopIndex[i]]  - xAxis.minF));
        stepX=(xFinal-xInitial)/MAXAUTOMARKS;
        X=X0+stepX/2;
        for(int ii=1; ii<=MAXAUTOMARKS; ii++){
          dataCurs.moveLeft(X);
          markSingle(i,iVSFile,iPlot,iTotPlot,false);
          X+=stepX;
        }
      }else{
        for(int ii=1; ii<=MAXAUTOMARKS; ii++){
          if(xVarParam.isMonotonic)
            deltaIndex=qMax(1,int(1.8f*markHalfWidth/xAxis.pixPerValue /(px[i][1]-px[i][0])));
          else
            deltaIndex=1;
          index=startIndex[i]+int(auxF*(ii-0.5f)*indexRange+deltaIndex*(iTotPlot-nFiles/2));
          X=X0+NearInt(xAxis.pixPerValue  * (px[i][index] - xAxis.minF));
          Y=Y0+yAxis.widthPix-NearInt(yRatio*(py[i][iPlot][index] - yMinF));
          drawMark(X+1,Y,iTotPlot,false); //il "+1" perché il dataCurs ha larghezza 3 e il valore è nel pixel centrale
        }
      }
      iTotPlot++;
    }
    if(filesInfo[i].variableStep) iVSFile++;
  }
  update();
}


float CLineChart::minus(struct SDigits c, int icifra, int ifrac){
/* Per la spiegazione v. inizio funzione "plus"*/
   float xret;
   switch(icifra) {
      case 3:           //in questo caso ifrac è sempre 1
        break;          // nessuna operazione per questo caso
      case 2:
        switch(ifrac) {
          case 1:
            c.i3=0;
            break;
          case 2:
            if(c.i3<5) c.i3=0;  else     c.i3=5;
            break;
          case 5:
            if(!isEven(c.i3)) c.i3--;
            break;
          default:
            puts("Errore 1 in minus");
        }
        break;
      case 1:                   // arrotondamento con una cifra significativa
        c.i3=0;
        switch(ifrac) {
          case 1:
            c.i2=0;
            break;
          case 2:
            if(c.i2<5) c.i2=0;     else     c.i2=5;
          break;
          case 5:
            if(!isEven(c.i2)) c.i2--;
            break;
          default:
            puts("Errore 3 in minus");
        }	                              // fine switch di ifrac
        break;
      default:
        puts("Errore 2 in minus");
   }                                    // fine switch di ii
   /* A differenza di plus qui non è necessaria alcuna gestione dei riporti */
//   xret=( (float)c.i1+(float)c.i2/10.f+(float)c.i3/100.f)*powf(10.f,float(c.ie));
   xret=( float(c.i1)+float(c.i2/10.f)+float(c.i3/100.f))*powf(10.f,float(c.ie));
   if(c.Sign == '-') xret=-xret;
   return(xret);
}


QString CLineChart::plot(bool autoScale){
  /* Manages left and right scale, then calls the private goPlot()
     - if autoScale=false the plot  the previously sent dispRect (using setDispRect() is used.
     - if autoscale=true the display rectangle (axes min and max values) dispRect
       is computed here based on the previously sent data
  */
  int i,icount,iTotPlot, iRet;
  resetMarkData();
  designPlot();
  lastAutoScale=autoScale;
//  iRet=0;
  if(autoScale){
    bool someLeftScale=false, someRightScale=false;
    /*Gestione eventuale scala destra. La metto se sono verificate due condizioni:
       1. E' richiesto almeno un grafico sulla scala sinistra
       2. E' richiesto almeno un grafico sulla scala destra     */
    twinScale=false;
    iTotPlot=0;
    for(i=0; i<nFiles; i++)
    for(icount=0; icount<nPlots[i]; icount++){
      if(curveParamLst[iTotPlot].rightScale)
        someRightScale=true;
      else
        someLeftScale=true;
      iTotPlot++;
    }

    if(someRightScale){
      if(someLeftScale)
        twinScale=true;
      else{
     //Se sono state richieste solo scale destre, si forzano tutte a scale sinistre:
        iTotPlot=0;
        for(i=0; i<nFiles; i++)
        for(icount=0; icount<nPlots[i]; icount++){
          curveParamLst[iTotPlot].rightScale=false;
            iTotPlot++;
        }
      }
    }
    dispRect=setFullDispRect();
    exactMatch=false;
    iRet=scaleXY(dispRect,false); //Ritocca il valore di dispRect
  }else{
      iRet=scaleXY(dispRect,false);
  }

  if(iRet==1)
  //Se iRet è diverso da 0 le cose non sono andate tutte a posto. Se è 1 è stato già emesso un messaggio d'errore; se è 2 è windowIsCut=true;
    return "";

  plotDone=true;
  return goPlot(false,false);
}
//---------------------------------------------------------------------------
QString CLineChart::goPlot(bool Virtual, bool /*IncludeFO*/){
/* Funzione principale per l'esecuzione dei grafici, richiamata dalla funzione plot()..
 * Se il parametro Virtual=true si fanno tutti i calcoli, curve escluse, ma non si scrive
 * niente con myPainter.
 * Questo serve quando si fa una stampa: dopo di essa vanno ripristinati tutti i valori
 * corretti di X0, Y0, axisW, markPts, ratio, xStartIndex e xStopIndex, e forse anche
 * qualcos'altro. Tutte queste cose servono per poter fare poi le operazioni di lettura
 * valori tramite cursore, e di marcatura ("Mark").
 * Allora dopo la stampa è opportuno ripetere il Plot, ma per evitare un inutile
 * ritracciamento delle curve in quell'unico caso pongo noCurves a true. Ovviamente
 * se Virtual è true non andrà neanche effettuata la cancellazione del grafico preesistente
 * per evitare di cancellare le curve preesistenti.
 * Il parametro includeFO viene passato a writeLegend, la quale viene così
 * informata se deve includere o meno i Factors e Offsets nella leggenda.
 *
 * FASI:
 * 0) operazioni preliminari
 * 1) preparazione penne e font
 * 2) scrittura leggenda, definzione geometria (X0, X1, Y0, Y1) e cursori
 * 3) tracciamento rettangolo grafico e mappatura indici per zoomate
 * 4) preparazione assi con scrittura ticmarks e label di asse
 * 5) esecuzione dei grafici tramite richiamo a drawCurves()
 *
*/

  int MaxPlots;
  SFloatRect2 R=dispRect;

  //***
  // fase 0: operazioni  preliminari
  delete startIndex;
  delete stopIndex;
  startIndex=new int[nFiles];
  stopIndex=new int[nFiles];

  if(stopIndex==nullptr)
      return "Unable to allocate variables in CLineChart!";
  myImage->fill(Qt::white);
  if(copying)
    FC.strongFilter=strongFilter;
  else
    FC.strongFilter=false;
  //Valori validi nel caso in cui non si stia facendo una zoomata
  //o si fa una zoomata ma la variabile X non è il tempo:
  for(int iFile=0; iFile<nFiles; iFile++){
    startIndex[iFile]=0;
    stopIndex[iFile]=filesInfo[iFile].numOfPoints-1;
  }

  //***
  // fase 1: preparazione penne e font
  if(PPlotPenWidth!=pwAuto){
    ticPen.setWidth(PPlotPenWidth+1); //se pwThin uso 1 pixel, se pwThick 2
    plotPen.setWidth(PPlotPenWidth+1);
  }else{  //Ora sono in spessore penna automatico
    //Si ricordi che il risultato della seguente divisione è troncato, non arrotondato.
    ticPen.setWidth(qMax(1,(plotRect.width()+plotRect.height())/1000));
    plotPen.setWidth(ticPen.width());
  }
  if(cutsLimits){
    framePen.setWidth(qMax(plotPen.width(),3));
    framePen.setColor(Qt::lightGray);
  }else{
    framePen.setWidth(plotPen.width());
    framePen.setColor(Qt::black);
  }

  gridPen.setWidth(1);
  gridPen.setStyle(Qt::DotLine);
  //E' stato verificato che la seguente riga è indispensabile per la scrittura su SVG
  if(makingSVG) myPainter->setFont(QFont(baseFontName,generalFontPx));
  if(writeTitle1){
      myPainter->setFont(baseFont);
      titleHeight=myPainter->fontMetrics().height();
      Y0=int(1.4*titleHeight);
    QTextOption txtOpt;
    txtOpt.setAlignment(Qt::AlignCenter);
    int addSpace=0;
    if(printing) addSpace=5;
//    QRectF testR1=plotRect, testR2=rect();
    titleRectF=QRectF(0,2,plotRect.width(),titleHeight+addSpace);
//    titleRectF=QRectF(0,2,myImage->width(),titleHeight);
    myPainter->setPen(Qt::black);
    myPainter->drawText(titleRectF, titleText, txtOpt);
  }else{
    Y0=textHeight+1;
  }

  //***
  //Fase 2) scrittura leggenda, definizione geometria (X0, X1, Y0, Y1) e cursori
  if(addLegend)
    writeLegend(false);
  else
    legendHeight=0;

  myPainter->setPen(framePen);
  //Vertici rettangolo di visualizzazione (X0-X1-Y0-Y1):
  X0=yAxis.maxTextWidth+int(1.5f*yAxis.ticPixWidth)+smallHSpace;
  if(makingSVG)X0+=svgOffset;
  if(printing)X0+=plotRect.x();
  //note:
  //- con questo X0 si lascia un margine bianco fisso di smallHSpace.
  //- Y0 è già stato calcolato più sopra!

  //  X1=plotRect.width()-max(rYAxis.maxTextWidth+2.0*yAxis.ticPixWidth+1.2*smallHSpace, 0.65*xAxis.maxTextWidth);
  if(twinScale)
     X1=plotRect.width()-qMax(ryAxis.maxTextWidth+yAxis.ticPixWidth+3*smallHSpace, xAxis.maxTextWidth);
  else
     X1=plotRect.width()-qMax(yAxis.ticPixWidth+3*smallHSpace, xAxis.maxTextWidth/2);

  if(X1<=X0){
    QMessageBox::critical(this,"","Critical error \"X1<X0\" in LineChart.\nProgram will be stopped");
    qApp->closeAllWindows();
  }
  Y1=plotRect.height()-legendHeight-textHeight-xAxis.ticPixWidth-2; //2 pixel fra la tacca e il numero
  Y1-=int(smallHSpace*1.5); //un piccolo ulteriore spazio per allontanarmi dal bordo
  if(Y1<=Y0){
    QMessageBox::critical(this,"","Critical error \"Y1<Y0\" in LineChart.\nProgram will be stopped");
    qApp->closeAllWindows();
  }

  dataCurs.setRect((X0+X1)/2,Y0,3,Y1-Y0);
  dataCurs2.setRect(int((X0+X1)*0.6),Y0,3,Y1-Y0);
  debugCurs=dataCurs;

  /*
Tracciamento rettangolo del grafico. Lo faccio trasparente e non a fondo bianco,  perché in tal modo rendo più agevole l'eventuale editazione di un grafico esportato, senza particolari inconvenienti. Nel grafico esportato avrò quindi pieno solo il rettangolo complessivo del grafico, tracciato altrove, ma non quello "netto" contenente gli assi, cioè questo:
*/
  if(!Virtual){
    myPainter->setPen(framePen);
    myPainter->setBrush(Qt::NoBrush);
    myPainter->drawRect(X0,Y0,X1-X0+1,Y1-Y0+1);
  }



  /* Nel caso di zoomata, la generazione del grafico può essere grandemente velocizzata (soprattutto nel caso di molti punti, evidentemente) se, invece di tracciare tutto il grafico e lasciare l'effettuazione della zoomata al taglio che la ClipRgn (o la mia ClipRgn, molto più efficiente di quella di Windows, implementata in CFilterClip) fa delle parti di grafico fuori zoomata, si procede al tracciamento della sola parte di grafico interessata dalla zoomata.
Questo, che si ottiene calcolando i valori di startIndex e stopIndex relativi all'effettiva finestra del grafico da tracciare, risulta possibile soltanto se la variabile X è monotona crescente rispetto al suo indice, e questo è senz'altro verificato se si tratta della variabile tempo. Questa caratteristica è specificata in CLineChart tramite la variabile booleana xVarParam.isMonotonic.
Se X, oltre che monotona crescente è anche costituita da campioni equispaziati (come nel caso di ATP) il calcolo di startIndex e stopIndex risulta particolarmente veloce.
*/
  if(xVarParam.isMonotonic){
    for(int iFile=0; iFile<nFiles; iFile++){
      int nPoints=filesInfo[iFile].numOfPoints;
      if(filesInfo[iFile].variableStep){
      int iPoint=0;
      do
        iPoint++;
      while(px[iFile][iPoint]<R.Left && iPoint<nPoints-1);
      // Se ho passato la riga superiore iPoint è tale che ho scavalcato il bordo sinistro di R e sono entrato dentro. Posso anche essere esattamente sul bordo sinistro.
      startIndex[iFile]=iPoint-1;
// con questa scelta inizio il tracciamento subito a sinistra del rettangolo; penserà poi la funzione filterClip a tagliare all'ingresso. Questa scelta non va però bene nel caso di scale logaritmiche in quanto andare di un punto a sinistra può implicare di scegliere un valore nullo dell'ascissa, con errore di dominio. Questo accade per esempio con il file filterQs.mat usato a marzo/aprile 2018. In tal caso il primo punto su file era x=0 il secondo x=1. Con la riga così com'è fatta sopra se scelgo come finestra di tracciamento da 1 a 1000, anche con l'exactMastch, il primo punto viene considerato a sinistra di 1, quindi il punto di valore 0, che genera il domain error
      if(xAxis.scaleType!=stLin)
          startIndex[iFile]=iPoint;

      do
        iPoint++;
      while(px[iFile][iPoint]<R.Right && iPoint<nPoints-1);
      stopIndex[iFile]=iPoint;
      }else{
        startIndex[iFile]=int((xAxis.scaleFactor*xAxis.scaleMin-px[iFile][0])/
                                 (px[iFile][1]-px[iFile][0]));
        startIndex[iFile]=qMax(0,startIndex[iFile]);
        startIndex[iFile]=qMin(nPoints,startIndex[iFile]);
        stopIndex[iFile]= NearInt((xAxis.scaleFactor*xAxis.scaleMax-px[iFile][0])/ (px[iFile][1]-px[iFile][0]))+1;
        stopIndex[iFile]=qMax(0,stopIndex[iFile]);
        stopIndex[iFile]=qMin(nPoints-1,stopIndex[iFile]);
      }
      if(startIndex[iFile]==stopIndex[iFile])
        return "Start and end indexes equal to each other in plot";
    }
  }

  //***
  //Fase 4) preparazione assi con scrittura ticmarks e label di asse
  if(plotType==ptBar)
    margin=qMax(1,int(0.42f*(X1-X0)/(stopIndex[0]-startIndex[0]+0.84f)));
  else
    margin=0;
  // ampiezza assi e intertacche
  xAxis.widthPix=X1-X0-2*margin;
  yAxis.widthPix=Y1-Y0;
  if(xAxis.scaleType==stLin)
    xAxis.pixPerValue= float(xAxis.widthPix) / (xAxis.scaleMax-xAxis.scaleMin);
  else
    xAxis.pixPerValue= float(xAxis.widthPix) / (xAxis.eMax-xAxis.eMin);
  if(yAxis.scaleType==stLin)
    yAxis.pixPerValue= float(yAxis.widthPix) / (yAxis.scaleMax-yAxis.scaleMin);
  else
    yAxis.pixPerValue= float(yAxis.widthPix) / (yAxis.eMax-yAxis.eMin);
  xAxis.ticIntervalPix=   xAxis.ticInterval * xAxis.pixPerValue;
  yAxis.ticIntervalPix=   yAxis.ticInterval * yAxis.pixPerValue;
  xAxis.pixPerValue /= xAxis.scaleFactor;
  yAxis.pixPerValue /= yAxis.scaleFactor;
  if(twinScale){
    if(ryAxis.scaleType==stLin)
      ryAxis.pixPerValue= float(yAxis.widthPix) / (ryAxis.scaleMax-ryAxis.scaleMin);
    else
      ryAxis.pixPerValue= float(yAxis.widthPix) / (ryAxis.eMax-ryAxis.eMin);
    ryAxis.ticIntervalPix=  ryAxis.ticInterval * ryAxis.pixPerValue;
    ryAxis.pixPerValue /= ryAxis.scaleFactor;
  }
  if(numOfVSFiles>0){
    CLineChart::DeleteIMatrix(pixelToIndexDX);
    pixelToIndexDX=CLineChart::CreateIMatrix(numOfVSFiles,X1-X0+1);
    if(xAxis.scaleType==stLin)
      fillPixelToIndex(pixelToIndexDX);
    else
      fillPixelToIndexLog(pixelToIndexDX);
  }
  // Tacche, numeri ed eventuale griglia Asse(i) y:
  // iii=myPainter->fontMetrics().width("-0.0008");
  if(!Virtual){
    drawAllLabelsAndGrid(xAxis);
    drawAllLabelsAndGrid(yAxis);
    if(twinScale)  drawAllLabelsAndGrid(ryAxis);
  }

  // Fase 5) esecuzione dei grafici tramite richiamo a drawCurves()

  QElapsedTimer timer;
  timer.start();
  switch (plotType){
    case ptLine:
      if(drawType==dtMC)
        drawCurves(Virtual);
      else if(drawType==dtMcD)
        drawCurvesD(Virtual);
      else if(drawType==dtQtF)
        drawCurvesQtF(Virtual);
      else if(drawType==dtQtI)
        drawCurvesQtI(Virtual);
      else
        drawCurvesPoly(Virtual);
    break;
    case ptBar:
      drawBars();
    break;
    case ptSwarm:
      drawSwarm();
    break;
  }
  drawTimeMs=int(timer.elapsed());
  drawTimeUs=int(timer.nsecsElapsed()/1000.f);
  //  qDebug() << "Drawing time: " << timer.elapsed() << "/ms";
  //Qui alloco lo spazio per i valori numerici da leggere in corrispondenza del cursore dati, anche se l'effettivo assegnamento dei valori avverrà nella routine "giveValues".
  delete cursorXValues;
  delete cursorXValBkp;
  DeleteFMatrix(cursorYValues);
  DeleteFMatrix(cursorYValBkp);
  cursorXValues=new float[nFiles];
  cursorXValBkp=new float[nFiles];

  MaxPlots=0;
  for(int i=0; i<nFiles; i++)
    MaxPlots=qMax(MaxPlots,nPlots[i]);
  cursorYValues=CLineChart::CreateFMatrix(nFiles,MaxPlots);
  cursorYValBkp=CLineChart::CreateFMatrix(nFiles,MaxPlots);
  //sPlotTime=str.number((clock()-t1)/(float)CLK_TCK,'g',3);
  update();
  return nullptr;
}


float CLineChart::plus(struct SDigits c, int icifra, int ifrac){
  /* Le funzioni plus e minus servono per trovare un numero "tondo" rispettivamente un po' più grande
  e un po' più piccolo del numero memorizzato dentro la struct cifre passata come primo
  parametro.
  "icifra" è il numero di cifre significative utilizzabili per rappresentare il numero. Più
           grande è icifra, più il risultato "tondo" si avvicinerà al numero di partenza.
  "ifrac" è una specie di indicazione frazionaria dei numero di cifre significative.
          Essa può valere solo 1 2 o 5.
          - Nel primo caso dopo "icifre" cifre significative il numero tondo avrà solo zeri.
          - Con ifrac=2 dopo icifre cifre significative si potrà avere un'unica cifra pari a 5,
            poi solo zeri
          - Con ifrac=5 dopo icifre cifre significative si potrà avere un'unica cifra
            che potrà assumere il valore di 2, 4, 6, o 8.
    */
  bool isEven(int);
  float xret;
  switch(icifra) {
    case 3:                           // in questo caso ifrac è sempre 1
      if(c.i4==0) goto exit;
      c.i3++;
      break;
    case 2:
      switch(ifrac) {
        case 1:
          if(c.i3+c.i4==0) goto exit;
          c.i3=0; c.i2++;
          break;
        case 2:
          if(c.i4==0 && (c.i3==5||c.i3==0)) goto exit;
          if(c.i3>=5) {
            c.i3=0; c.i2++;
          } else
              c.i3=5;
          break;
        case 5:
          if( c.i4==0 && isEven(c.i3) )
              goto exit;
          if(isEven(c.i3)) c.i3+=2;
            else c.i3++;
          break;
        default:
          ;//puts(tr"error 1 in plus"));
      }                      // fine switch di ifrac
            break;
    case 1:     // arrotondamento con una cifra significativa
                // c.i3 verra' posto a 0 alla fine di questo caso
      switch(ifrac) {
        case 1:
          if(c.i2+c.i3+c.i4==0) goto exit;
          c.i2=0; c.i1++;
          break;
        case 2:
          if(c.i3+c.i4==0 && (c.i2==5||c.i2==0) ) goto exit;
          if(c.i2>=5) {
            c.i2=0; c.i1++;
          } else c.i2=5;
          break;
       case 5:
         if(  c.i3+c.i4==0 && isEven(c.i2)  ) goto exit;
         if(isEven(c.i2)) c.i2+=2;
           else c.i2++;
         break;
       default:
         ;//puts(tr"Errore 2 in plus"));
     }                                  // fine switch di ifrac
     c.i3=0;
     break;
   default:
      ;//puts(tr"Errore 3 in plus"));
  }                                     // fine switch di icifra

  /* Ora si sistemano eventuali riporti: */
  if(c.i3==10) {
    c.i3=0; c.i2++;
    if(c.i2==10){
      c.i2=0; c.i1++;
      if(c.i1==10){
        c.i1=1; c.ie++;
      }
    }
  }
  exit:
  xret=( float(c.i1)+float(c.i2/10.f)+float(c.i3/100.f))*powf(10,float(c.ie));
  if(c.Sign=='-') xret=-xret;
  return(xret);
}

QString CLineChart::print(QPrinter * printer, bool thinLines){
  /* Funzione per la stampa del plot, tipicamente su supporto cartaceo.
 A differenza di BCB non ho una stampante generale di sistema. Viene quindi passato un puntatore a printer
*/
  //Per la stampa userò il normale comando plot(), passando la stampante come output device.
  QString ret="";
  myPainter->end();
  if(myPainter->begin(printer)==false){
    ret=tr("Unable to open the selected printer");
    QMessageBox::critical(this,"",ret);
    return ret;
  }
  QRect prnRect=printer->pageLayout().paintRectPixels(printer->resolution());

  if(prnRect.width()<20 || prnRect.height()<20 ){
    ret=tr("Unable to get valid information from the selected printer");
    QMessageBox::critical(this,"",ret);
    return ret;
  }
  //Riduco un poco per avere un certo margine:
  prnRect.setWidth(int(0.9f*prnRect.width()));
  prnRect.setHeight(int(0.9f*prnRect.height()));
  int xmargin=int(0.05f*prnRect.width()),
          ymargin=int(0.05f*prnRect.height());
  //Devo modificare printRect per mantenere l'aspect ratio di plotRect:
  if(prnRect.height()/prnRect.width() > aspectRatio){
    plotRect=QRect(xmargin,ymargin, prnRect.width(), int(aspectRatio*prnRect.width()));
  }else{
    if(prnRect.height()/aspectRatio>prnRect.width())
      plotRect=QRect(xmargin,ymargin, prnRect.width(), int(aspectRatio*prnRect.width()));
    else
      plotRect=QRect(xmargin,ymargin, int(prnRect.height()/aspectRatio), prnRect.height());
  }
  printing=true;
  designPlot();
  EPlotPenWidth oldPenWidth=PPlotPenWidth;
  if(thinLines)
    PPlotPenWidth=pwThin;
  else
    PPlotPenWidth=pwThick;
  goPlot(false,false);
  markAll();
  myPainter->end();
  PPlotPenWidth=oldPenWidth;
  plotRect=geometry();
  printing=false;
  blackWhite=false;
  //Non so perché il grafico a questo punto è scomparso dallo schermo. Lo ritraccio:
  resizeStopped();
  update();
  return ret;
}

void CLineChart::resetMarkData(){
    autoMark=false;
    manuMarks.lastMark=-1;
}

void CLineChart::resizeEvent(QResizeEvent * ){
/*La prima chiamata di resizeEvent avviene subito prima dello showEvent, e devo effettuare le operazioni usuali di PlotXY, ora trasferite in resizeStopped.
Nelle chiamate successive, durante un vero resize, occorre evitare che per grafici lunghi la reattività del programma sia insufficiente. Allora se i grafici sono lunghi l'aggiornamento del plot verrà fatto solo alla fine del resize, quando il timeout di myTimer attiverà resizeStopped.
Nella versione finale, pertanto, il timeout di myTimer sarà 0 per i grafici veloci (come verrà misurato alla prima esecuzione del comando plot()); sarà invece 100 ms per i grafici lenti, che verranno quindi tracciati solo alla fine del resize. */
  int timeout=100;
    if(plotType==ptBar)
      timeout=10;
    if(isVisible())
        myTimer->start(timeout);
    else
        resizeStopped();
}


void CLineChart::resizeStopped(){
  static QRect oldRect;
  plotRect=geometry();
  QRect r=plotRect;
  if(r.width()<50||r.height()<50)
    return;
  delete myPainter;
  delete myImage;
  aspectRatio=float(r.height())/float(r.width());

  myImage= new QImage(r.width(),r.height(),QImage::Format_RGB32);

  myPainter=new QPainter(myImage);
  txtPen=myPainter->pen();
//  if(dataGot){
  if(plotDone){
    designPlot();
    if(cutsLimits)
      dispRect=setFullDispRect();
    scaleXY(dispRect,false); //Ritocca il valore di DispRect appena fissato in seFullDispRect
    goPlot(false,false);
  }
  markAll();
  oldRect=r;
  chartResizeStopped();
}


int CLineChart::scaleAxis(SAxis &myAxis, float minVal, float maxVal, int minTic, unsigned include0, bool exactMatch)  {
/* Funzione per determinare la scala di un dato asse.
1) Nel caso di scala logaritmica la funzione determina soltanto i valori minimo e massimo
   delle potenze di 10 da includere nella scala.
2) Nel caso di scala lineare la funzione determina, in particolare:
   - il valori minimo e massimo dell'asse, scelti in modo che siano dei numeri "rotondi"
   - il numero di tacche da mettere e i valori numerici da mettere sulle tacche.
NOTE:
  . Se il grafico è costante e non nullo su tutto il range (minVal=maxVal!=0),
    lo 0 è forzato nella scala;
  . se minVal=maxVal!=0, la scala è convenzionalmente fissata fra -1 e 1.
  . per scale logaritmiche minVal e maxVal devono essere positivi

***
NOTA DICEMBRE 2018:
 Questa routine da risultati non del tutto soddisfacenti quando il riempimento è prossimo
 a RATIOLIM (attuamente fissato a 0.799f), anche perché questo è associato a forte
 asimmetria dei margini vuoti. Un caso molto significativo si ha facendo il grafico di
 1.1+-2*sin(x) che dà come estremi (-1,4), ma la curva ha un valore massimo poco superiore
 a 3.0: si ha uno spazio bianco sopra molto ampio, mentre sotto il grafico riempe quasi
 del tutto il rettangolo di visualizzazione.
 In questi casi ci starebbe bene una soluzione che eviti di mettere le tacche ai
 valori corrispondenti ai lati del rettangolo di visualizzazione. Nel caso citato, ad es.,
 si poteva mettere la tacca superiore a 3 e quella inferiore a -1, ma il lato
 orizzontale alto del rettangolo doveva essere più in alto della tacca superiore.
 Per evitare di cambiare radicalmente la maniera con cui sono calcolate le scale e le
 tacche, che nella stragrande maggioranzaa dei casi funziona molto bene, si potrebbe
 in un futuro prossimo aggiungere una funzione "enhanceAxisFill" che lavora solo nel caso
 in cui il calcolo standard (cioè quello attuale) delle scale porta dei vuoti di riempimento
 molto asimmetrici fara sopra e sotto, oppure, in alternativa, un riempimento fra 0.8 e
 0.9. Solo in questo caso specifico richiamerei "enhanceAxisFill" e sceglierei la
 soluzione con tacche esterne interne ai lati del rettangolo.
***

Significato delle variabili passate (solo fino a maxVal sono usate per scale logaritmiche):
- myAxis è l'asse di cui vanno determinati gli attributi
- minVal e MaxVal sono il valore minimo e massimo dei punti che devono essere raffigurati
  nella finestra individuata dalla scala che si vuole determinare.
  Il valore minimo della scala, scaleMin, sarà un numero "tondo" inferiore o uguale a
  minVal; analogamente scaleMax sarà un numero tondo superiore o uguale a MaxVal.
- minTic è il numero minimo di tacche sull'asse. Il corrispondente massimo è
  minTic+3. Ad es. all'avviamento il numero di tacche può essere compreso fra 4 e 7.
    Se il grafico diventa molto piccolo minTic si riduce di conseguenza.
- "include0" (intero senza segno) indica la percentuale di Include0zione
    richiesta della presenza dello 0 nella scala:
    - con include0 = 0 non ho alcuna forzatura;
    - con include0 = 100 lo zero è sempre in scala;
    - con include0 = p   lo zero è incluso in scala purché la percentuale di vuoti che ciò comporta non superi p
- exactMatch se è true disabilita la richiesta di estremi di scala "rotondi".
  Pertanto, in tal caso la funzione calcola solo le tacche e il fattore di scala senza
  preventivamente arrotondare minVal e maxVal.
****** CODICI DI RITORNO Ret
- ret=0 se è tutto OK
- ret=1 se è richiesta una scala log con valori della grandezza sul grafico non tutti positivi.
- ret=2 se è tutto ok ma è windowIsCut=true;
*/
  int i, aux, icifra,ifrac,ntic,
          iermx,   //digit di esponente del valore arrotondato di Max
          iermn,   //digit di esponente del valore arrotondato di Min
          irmn2;   //secondo digit del valore arrotondato Min
  float Ratio, scaleFactor, roundRange, minVal1, maxVal1, fAux;
  char buffer[12];
  QString msg;
  SDigits Min, Max;
  float (*pmax) (struct SDigits, int, int);
  float (*pmin)(struct SDigits, int, int);

  myAxis.minVal=minVal;
  myAxis.maxVal=maxVal;
  myAxis.done=0;
  /**** Parte relativa al caso di scale logaritmiche ****/
  if(myAxis.scaleType!=stLin){
    if(maxVal<=0 || minVal<=0){
      if(myAxis.type==atX)
        msg="Cannot create a log scale on the x-axis\n"
            "because the range contains null or negative values\n\n"
            "Please change the variable to be plot, or the x-axis range or choose a linear scale";
      else
        msg="Cannot create a log scale on the y-axis\n"
            "because the range contains null or negative values\n\n"
            "Please change the variable to be plot, or the y-axis range or choose a linear scale";
      QMessageBox::critical(this, "MC's PlotXWin",msg,QMessageBox::Ok);
      return 1;
    }
    sprintf(buffer,"%+10.3e",double(minVal));
    sscanf(buffer+7, "%u", &myAxis.eMin);
    sprintf(buffer,"%+10.3e",double(maxVal));
    sscanf(buffer+7, "%u", &aux);
    /* Se il numero MaxVal non è una potenza di 10 eMax=Aux+1, altrimenti eMax=Aux*/
    buffer[6]=0;
    if(strcmp(buffer,"+1.000")==0)
      myAxis.eMax=aux;
    else
      myAxis.eMax=aux+1;
    myAxis.scaleFactor=1.;
    myAxis.scaleMin=powf(10.,myAxis.eMin);
    myAxis.scaleMax=powf(10.,myAxis.eMax);
    if(myAxis.scaleType==stDB){
      sprintf(buffer,"%d",20*myAxis.eMin);
      myAxis.maxTextWidth=myPainter->fontMetrics().horizontalAdvance(buffer);
      sprintf(buffer,"%d",20*myAxis.eMax);
      myAxis.maxTextWidth=qMax(myAxis.maxTextWidth, myPainter->fontMetrics().horizontalAdvance(buffer));
      sprintf(buffer,"dB");
      if(useBrackets)
        sprintf(buffer,"(dB)");
      myAxis.maxTextWidth=qMax(myAxis.maxTextWidth, myPainter->fontMetrics().horizontalAdvance(buffer));
    }else{// myAxis.ScaleType=stLog
        aux=myPainter->fontMetrics().horizontalAdvance("10");
      sprintf(buffer,"%d",myAxis.eMin);
      myPainter->setFont(expFont);
      myAxis.maxTextWidth=aux+myPainter->fontMetrics().horizontalAdvance(buffer);
      sprintf(buffer,"%d",myAxis.eMax);
      aux+=myPainter->fontMetrics().horizontalAdvance(buffer);
      myAxis.maxTextWidth=qMax(myAxis.maxTextWidth,aux);
      myPainter->setFont(numFont);
    }
    myAxis.done=1;
    myAxis.minF=myAxis.scaleMin;
    myAxis.maxF=myAxis.scaleMax;
    return 0;
  }

  /**** Ora la parte, ben più complessa, relativa al caso di scale lineari ***/
  myAxis.halfTicNum=false;
  //Elimino da minVal e maxVal quello che c'è oltre le prime 4 cifre significative:
  sprintf(buffer,"%+10.3e",double(maxVal));
  sscanf(buffer,"%f", &maxVal1);
  sprintf(buffer,"%+10.3e",double(minVal));
  sscanf(buffer,"%f", &minVal1);
  //gestisco il caso particolare di estremi, arrotondati alle prime quattro cifre significative, uguali:
  if(minVal1==maxVal1){
    if(minVal1==0){
      myAxis.scaleFactor=1;
      myAxis.ticInterval=1;
      myAxis.ticDecimals=0;
      myAxis.scaleMax=1;
      myAxis.scaleMin=-1;
    }
    if(minVal1>0){
      myAxis.scaleMin=0;
      myAxis.scaleMax=2*minVal1;
      myAxis.ticInterval=minVal1;
    }
    if(minVal1<0){
      myAxis.scaleMin=2*minVal1;
      myAxis.scaleMax=0;
      myAxis.ticInterval=-minVal1;
    }
    sprintf(buffer,"%+10.3e",double(minVal1));
    sscanf(buffer+7, "%3u", &iermx);
    myAxis.done=0;
    Max.roundValue=Min.roundValue=minVal;
    sprintf(buffer,"%+10.3e",double(Max.roundValue));
    sscanf(buffer+7, "%3u", &iermx);
    iermn=iermx;
    goto ComputeScaleFactor;
  }

  beginRounding(Min, Max, minVal, maxVal, include0);
  if(exactMatch){
    Min.roundValue=minVal;
    Max.roundValue=maxVal;
    myAxis.scaleMin=minVal;
    myAxis.scaleMax=maxVal;
    if(minVal==maxVal){
      myAxis.done=0;
    }else if(minVal==0 || maxVal==0){
      myAxis.done=2;
    }else{
      fAux=fabsf((minVal-maxVal)/maxVal);
      if(fAux<0.0001f)
        myAxis.done=1;
      else
        myAxis.done=2;
    }
  }else{
    /* Se Min.Value e Max.Value hanno segni opposti vanno entrambi arrotondati con la funzione "plus"; se hanno lo stesso segno quello minore in valore assoluto verrà arrotondato usando la funzione "minus" l'altro con "plus": */
    if(Max.Value*Min.Value<=0) {
      pmax=plus;
      pmin=plus;
    }else if(fabsf(Max.Value) >= fabsf(Min.Value)){
       pmax=plus;
       pmin=minus;
    }else{
      pmax=minus;
      pmin=plus;
    }
    myAxis.done=1;
    for(icifra=1; icifra<=3; icifra++){
      float f1, f2;
//      float f3;
      f1=Max.Value*powf(10,-float(Max.ie));
      f2=Min.Value*powf(10,-float(Min.ie));
//      f3=fabs(f1-f2-1.2);
      //caso particolarissimo da trattare separatamente da tutti gli altri:
      if( fabsf(f1-f2-1.2f)<1e-5f  &&  Max.i3==0 && Max.i4==0){
        Max.roundValue=Max.Value;
        Min.roundValue=Min.Value;
        myAxis.done = 2;
        break;
      }
      ifrac=1;
      Max.roundValue=pmax(Max,icifra,ifrac);
      Min.roundValue=pmin(Min,icifra,ifrac);
      roundRange=Max.roundValue-Min.roundValue;
      if(roundRange!=0) {
        Ratio=(Max.Value-Min.Value) / roundRange;
        if(Ratio >= RATIOLIM) {
            myAxis.done = 2;
            break;
        }
      } else break;
      ifrac=2;
      Max.roundValue=pmax(Max,icifra,ifrac);
      Min.roundValue=pmin(Min,icifra,ifrac);
      roundRange=Max.roundValue-Min.roundValue;
      if(roundRange!=0) {
        Ratio=(Max.Value-Min.Value) / roundRange;
        if(Ratio >= RATIOLIM) {
          myAxis.done = 2;
          break;
        }
      } else break;
      ifrac=5;
      Max.roundValue=pmax(Max,icifra,ifrac);
      Min.roundValue=pmin(Min,icifra,ifrac);
      roundRange=Max.roundValue-Min.roundValue;
      if(roundRange!=0) {
        Ratio=(Max.Value-Min.Value) / roundRange;
        if(Ratio >= RATIOLIM) {
          myAxis.done = 2;
          break;
        }
      } else
        break;
    }
    if(myAxis.done != 2) {
      Max.roundValue=Max.Value;
      Min.roundValue=Min.Value;
    }
    myAxis.scaleMin = Min.roundValue;
    myAxis.scaleMax = Max.roundValue;
  }
  /* Calcolo del numero di tacche (sempre fra minTic e minTic+3)*/
  sprintf(buffer,"%+10.3e",double(Max.roundValue));
  sscanf(buffer+7, "%u", &iermx);
  sprintf(buffer,"%+10.3e",double(Min.roundValue));
  sscanf(buffer+7, "%u", &iermn);
  if(myAxis.done != 2){
    ntic=minTic+1;
  }else{
    //Il seguente calcolo di irmn2 verrà sfruttato per il calcolo del numero di decimali sulle tacche
    sprintf(buffer,"%+10.3e",double(Min.roundValue));
    sscanf(buffer+3, "%1u", &irmn2);
    ntic=computeTic(Min.roundValue,Max.roundValue,minTic);
    if(ntic==minTic)
      myAxis.halfTicNum=true;
  }
  /* Distanza tra le tacche: */
  myAxis.ticInterval= (myAxis.scaleMax-myAxis.scaleMin) / (float(ntic)+1);
  if(myAxis.halfTicNum) myAxis.ticInterval/=2;

  /* L'algoritmo fin qui attivato non si comporta male. Il suo principale difetto è che spesso allarga gli estremi della scala, sempre con il vincolo di un riempimento  non inferiore all'80% quando non sarebbe necessario. Ad es. se gli estremi di una variabile sono 0-35, l'algoritmo calcola 0-40 con ntic=7.
 Pertanto qui effettuo un correttivo:  se il grafico ci sta tra la prima tacca e l'estremo destro, ovvero fra l'estremo sinistro e l'ultima tacca, allora il numero di tacche è ridotto di uno e la tacca rispettivamente sinistra o destra diventa l'estremo sinistro o destro.
 */
  if(include0==0 && myAxis.done==2 && !myAxis.halfTicNum){
    if(minVal>0.9999f*(myAxis.scaleMin+myAxis.ticInterval)){
      myAxis.scaleMin=myAxis.scaleMin+myAxis.ticInterval;
      ntic--;
      myAxis.ticInterval = (myAxis.scaleMax-myAxis.scaleMin) / (float(ntic)+1);
    }else if(maxVal<1.0001f*(myAxis.scaleMax-myAxis.ticInterval)){
      myAxis.scaleMax=myAxis.scaleMax-myAxis.ticInterval;
      ntic--;
      myAxis.ticInterval = (myAxis.scaleMax-myAxis.scaleMin) / (float(ntic)+1);
    }
  }

ComputeScaleFactor:
  /* Qui gestisco cutslimits. Questa variabile indica se gli estremi visuaizzati sono interni agli estremi effettivi. SIccome PlotXY opera in semplice recisione, questo è significativo solo se l'errore supera quanto rappresentbile entro la sesta cifra significativa. */
  myAxis.cutsLimits=false;
  if(myAxis.scaleMin>minVal)
      if(minVal!=0)
          if(qAbs((myAxis.scaleMin-minVal)/minVal)>float(1.e-6))
              myAxis.cutsLimits=true;
  if(myAxis.scaleMax<maxVal)
      if(maxVal!=0)
          if(qAbs((myAxis.scaleMax-maxVal)/maxVal)>float(1.e-6))
              myAxis.cutsLimits=true;
/*
  if(myAxis.scaleMin>minVal||myAxis.scaleMax<maxVal)
    myAxis.cutsLimits=true;
  else
    myAxis.cutsLimits=false;
*/
  /* Calcolo di eventuale fattore di scala:  */
  aux=iermx;
  if(Max.roundValue==0)
    aux=iermn;
  else if (iermn>iermx && Min.roundValue!=0)
    aux=iermn;
  if(aux>-2 && aux<4)
     myAxis.scaleExponent=0;
  else
    myAxis.scaleExponent=(aux-(aux<0))/3*3;
  scaleFactor=powf(10.0f,float(myAxis.scaleExponent));
  myAxis.scaleFactor=scaleFactor;
  myAxis.scaleMax/=scaleFactor;
  myAxis.scaleMin/=scaleFactor;
  myAxis.ticInterval/=scaleFactor;

  // Calcolo decimali da scrivere sulle etichette numeriche:
  myAxis.ticDecimals=computeDecimals(myAxis.scaleMin,myAxis.ticInterval,myAxis.halfTicNum);


  /* Calcolo  della larghezza massima delle etichette dell'asse, se di tipo LY o RY  */
//  painter->setFont(QFont(fontName, exponentFontSize));
  float yf, yy, YY;
  char num[10], format[5]="%.1f";
  yf=myAxis.scaleMin;
  sprintf(format+2,"%1df",qMax(myAxis.ticDecimals,0));
  sprintf(num,format,double(myAxis.scaleMax));
  sscanf(num,"%f",&YY);
  if(myAxis.done==0){
    aux=myPainter->fontMetrics().horizontalAdvance(num);
    sprintf(num,format,double(myAxis.scaleMin));
    aux=qMax(aux,myPainter->fontMetrics().horizontalAdvance(num));
    goto Return;
  }
  aux=0;
  i=0;
  myPainter->setFont(numFont);
  do{
/*Il seguente numero 10 è dovuto al fatto che al massimo ci possono essere 7 ticmarks con label numeriche, il che implica 8 spazi fra una ticmark con label  e l'altra.
  Peraltro si hanno casi particolari in cui si raggiunge 10. Un esempio è quando i due estremi degli assi sono estremamente vicini, come nel file "Motori3" var. u2:TQGEN e scelta manuale degli assi con   Ymin=-17235  Ymax=-17234
*/
    if(++i==10){
//      QMessageBox::information(this, tr("TestLineChart"),  tr("Sorry, cannot find better scales for these axes!"), QMessageBox::Ok,QMessageBox::Ok);
        break;
    }
    sprintf(format+2,"%1df",qMax(myAxis.ticDecimals,0));
    sprintf(num,format,double(yf));
    aux=qMax(aux,myPainter->fontMetrics().horizontalAdvance(num));
    yf+=myAxis.ticInterval*(1+myAxis.halfTicNum);
    sprintf(num,format,double(yf));
    sscanf(num,"%f",&yy);
  }while(yy<=YY);
 Return:
/*A questo punto ho messo in Aux la massima ampiezza delle etichette numeriche.
  Va ora calcolata maxTextWidth tenendo conto dell'ampiezza della eventuale label di asse.
  Si ricorda che writeAxisLabel con l'ultimo parametro true anziché scrivere calcola solo la lunghezza della label; il termine dipendente da ticWidth.y è conseguenza del fatto che la label, non essendo in corrispondenza dei ticWidth, viene accostata un po' più vicino al rettangolo del grafico dei valori numerici.
   */
//  int iii=myPainter->fontMetrics().width("-0.0008");
  if(myAxis.type==atX)
    myAxis.maxTextWidth=aux;
  else if(myAxis.type==atYL)
    myAxis.maxTextWidth=qMax(aux,writeAxisLabel(0,0,myAxis,true)-yAxis.ticPixWidth* !myAxis.halfTicNum);
  else if(myAxis.type==atYR)
    myAxis.maxTextWidth=qMax(aux,writeAxisLabel(0,0,myAxis,true));
  else{
    QMessageBox::critical(this, tr("TestLineChart"),
         tr("Unexpected Error \"AxisType\"\n"
                "Please, contact program maintenance"),
                 QMessageBox::Ok,QMessageBox::Ok);
    qApp->closeAllWindows();
  }


  myAxis.minF=myAxis.scaleMin*myAxis.scaleFactor;
  myAxis.maxF=myAxis.scaleMax*myAxis.scaleFactor;
  /*  Modifica 31/1/2017. Se l'intervallo di una variabile è strambo, ad es. fra -0.3 e +1000.3,    può accadere che scaleMax<maxVal e/o scaleMin>minVal. Questo è accettabile, ma è fastidioso che il dettaglio della situazione non si riesca a vedere nemmeno zoomando. Provo a fare la seguente modifica (fino al return) per vedere se la situazione migliora.
*/

/*
if(myAxis.done==2){
  myAxis.minF=minVal*myAxis.scaleFactor;
  myAxis.maxF=maxVal*myAxis.scaleFactor;
  Ratio=maxVal-minVal / roundRange;
}else{
  myAxis.minF=myAxis.scaleMin*myAxis.scaleFactor;
  myAxis.maxF=myAxis.scaleMax*myAxis.scaleFactor;
}
*/
  return 0;
}



//---------------------------------------------------------------------------
int CLineChart::scaleXY(SFloatRect2 & dispRect, const bool justTic){
/*
 * Questa funzione serve per fare i calcoli delle scale sui due o tre assi.
 * Riceve in ingresso una versione "grezza" di dispRect, con numeri non ancora tondi,
 * e ne calcola una versione rifinita.
 *
 * Durante l'esecuzione di un resize viene ugualmente cambiata, solo per ricalcolare
 * il numero delle tacche (justTic=true).  Per ragioni di semplicità implementativa non
 * propago la gestione di justTic anche a scalaAxis, anche se faccio fare un po' di
 * calcoli inutili al programma.

  ***** CODICI DI RITORNO Ret
  - Ret =0 se è tutto OK
  - Ret =1 se scaleAxis ha rilevato un problema (attualmente solo scala log assieme a valori non tutti positivi della grandezza)
  - ret=2 allora windowIsCut=true
*/
  unsigned Include0=100*forceYZero;
  int ret;
  SFloatRect2 R=dispRect;

  if(!justTic){
    ret =scaleAxis(xAxis, R.Left, R.Right, minXTic, Include0, exactMatch);
    ret+=scaleAxis(yAxis, R.LBottom, R.LTop, minYTic, Include0, exactMatch);
    dispRect.Left=xAxis.scaleMin*xAxis.scaleFactor;
    dispRect.Right=xAxis.scaleMax*xAxis.scaleFactor;
    dispRect.LBottom=yAxis.scaleMin*yAxis.scaleFactor;
    dispRect.LTop=yAxis.scaleMax*yAxis.scaleFactor;
  }else{
    ret =scaleAxis(xAxis, R.Left, R.Right, minXTic, Include0, true);
    ret+=scaleAxis(yAxis, R.LBottom, R.LTop, minYTic, Include0, true);
  }
  if(ret==1)
    return ret;
  if(twinScale){
    if(!justTic){
      scaleAxis(ryAxis, R.RBottom, R.RTop, minYTic, Include0, exactMatch);
        dispRect.RBottom=ryAxis.scaleMin*ryAxis.scaleFactor;
        dispRect.RTop=ryAxis.scaleMax*ryAxis.scaleFactor;
    }else{
      scaleAxis(ryAxis, R.RBottom, R.RTop, minYTic, Include0, true);
    }
  }else
    ryAxis.maxTextWidth=0;
  if(xAxis.cutsLimits ||yAxis.cutsLimits ||
                          (ryAxis.cutsLimits&&twinScale))
    cutsLimits=true;
  else
    cutsLimits=false;
  return ret;
}

void CLineChart::setActiveDataCurs(int setCurs){
  SXYValues values;
  switch(setCurs){
    case 0:
      dataCursVisible=false;
      dataCurs2Visible=false;
      break;
    case 1:
      dataCursVisible=true;
      dataCurs2Visible=false;
      if(dataGot)
        giveValues(dataCurs.x()+1, linearInterpolate, false, false);
      break;
    case 2:
      if(!dataCursVisible)return;
      dataCurs2Visible=true;
      if(!dataGot)break;
      values=giveValues(dataCurs.x()+1, linearInterpolate, false, false);
      //Prendo i valori con differenze orizzontali e verticali rispetto alla posizione del cursore primario:
      values=giveValues(dataCurs2.x()+1, linearInterpolate, true, true);
      emit valuesChanged(values,true,true);
      break;
    }
    update();
}
void CLineChart::setDispRect(SFloatRect2 rect){
     dispRect=rect;
     zoomed=true;
}


SFloatRect2 CLineChart::setFullDispRect(){
 /* Questa funzione effettua un prima grossolana valutazione della finestra del grafico
  * da utilizzare, cioè gli estremi minimo e mwssimo dell'asse orizzontale e degli
  *  assi vertiali. Questa finestra contiene in generale come estremi numeri con molti
  * digit significativi, non comodi da usare come estremi per i grafici: essi verranno
  * in seguito sufficientemente modificati in modo da ottenere numeri "rotondi".
  *
  * Essa mette i risultati in "dispRect". Sarebbe opportuno (in quanto, oltre che utile,
  * molto facile) convertirla quanto prima in una funzione pura.
*/
  bool RmMInitialised=false, LmMInitialised=false;
  int i;
  struct SMinMax AuxmM;
  SFloatRect2 myDispRect;
  SMinMax xmM, lYmM, rYmM;

  //Le seguenti quattro rige sono inutili ma evitano dei warning:
  lYmM.Max=0;
  lYmM.Min=0;
  rYmM.Max=0;
  rYmM.Min=0;

  if(!dataGot){
    QMessageBox::critical(this, "TestLineChart", "Data !dataGot in sefUllDisprect", QMessageBox::Ok);
    qApp->closeAllWindows();
  }

  xmM=findMinMax(px[0],filesInfo[0].numOfPoints);
  for(i=1; i<nFiles; i++){
    AuxmM=findMinMax(px[i],filesInfo[i].numOfPoints);
    xmM.Min=qMin(xmM.Min,AuxmM.Min);
    xmM.Max=qMax(xmM.Max,AuxmM.Max);
  }

  int iTotPlot=0;
  for(i=0; i<nFiles; i++){
    for(int iPlot=0; iPlot<nPlots[i]; iPlot++){
      AuxmM=findMinMax(py[i][iPlot],filesInfo[i].numOfPoints);
      if(curveParamLst[iTotPlot].rightScale){
        if(RmMInitialised){
          rYmM.Min=qMin(rYmM.Min,AuxmM.Min);
          rYmM.Max=qMax(rYmM.Max,AuxmM.Max);
        }else{
          rYmM=AuxmM;
          RmMInitialised=true;
        }
      }else{
        if(LmMInitialised){
          lYmM.Min=qMin(lYmM.Min,AuxmM.Min);
          lYmM.Max=qMax(lYmM.Max,AuxmM.Max);
        }else{
          lYmM=AuxmM;
          LmMInitialised=true;
        }
      }
      iTotPlot++;
    }
  }

  myDispRect.Left=xmM.Min;
  myDispRect.Right=xmM.Max;
  myDispRect.LTop=lYmM.Max;
  myDispRect.LBottom=lYmM.Min;
  myDispRect.RTop=rYmM.Max;
  myDispRect.RBottom=rYmM.Min;
  return myDispRect;
}


void CLineChart::setPlotPenWidth(EPlotPenWidth myWidth){
  PPlotPenWidth=myWidth;
  /* In linea di massima si potrebbe pensare di attribuire qui PPlotPenWidth a plotPen, ticPen, framePen. Il problema è che il valore passato non è sempre un numero fisso: nel caso sia pwAuto, dipenderà  dalle dimensioni della finestra. Di conseguenza sarebbe abbastanza inutile.
Pertanto l'attribuzione dei valori effettivi degli spessori delle penne sarà fatta all'inizio della funzione goPlot(bool, bool). */
}

void CLineChart::setXZeroLine(bool zeroLine_){
    xAxis.addZeroLine=zeroLine_;
}

void CLineChart::setYZeroLine(bool yZeroLine_){
    yAxis.addZeroLine=yZeroLine_;
    ryAxis.addZeroLine=yZeroLine_;
}


void CLineChart::setXScaleType(enum EScaleType xScaleType_){
    xAxis.scaleType=xScaleType_;
}

void CLineChart::setYScaleType(enum EScaleType yScaleType_){
    yAxis.scaleType=yScaleType_;
}

//---------------------------------------------------------------------------
int CLineChart::writeAxisLabel(int X, int Y, SAxis &axis, bool _virtual ){
/* Funzione che scrive l' "etichetta di asse" su un asse.
 * Si tratta di un'etichetta che può contenere una potenza di 10 (se necessaria per numeri
 * molto grandi o piccoli)  ovvero, fra parentesi, l'unità di misura con prefisso.
 * Se autoLabelXY=true, si usano le unità di misura attribuite alle variabili attraverso
 * xVarParam e curveParam e passate con getData().
 * Il valore locale AutolabelXY_ partedal valore della variabile globale del grafico,
 * ma viene riconvertita in false se il numero è talmente piccolo o grande che non c'è un
 * prefix adatto per rappresentarlo. Il set di 'prefix' che uso è minore di quello completo
 * del SI, inquanto alcuni pprefissi quali yootto oyotta non li conosce nessuno e sono per
 * me più dannosi che utili.
 *
 * Naturalmente esse potranno essere usate solo se le unità relative ai vari plot del
 * medesimo asse sono uguali.
 * Se però useUserUnits=true, vengono usate le unità passate attraverso getUserUnits()
 * a prescindere dai valori di xVarParam e cuveParam.
 *
 * In tutti i casi in cui ci sono unità di misura ed è necessario un fattore di asse per
 * evitare numeri sono molto grandi o molto piccoli lungo gli assi invece delle potenze di
 * 10 vengono usati i prefissi (elencati qui sotto sotto "prefix").
 *
 * Se è Virtual=true, l'effettiva scrittura non viene fatta ma viene solo calcolata
 * l'ampiezza della label, che viene ritornata dalla funzione.
 * Se Virtual=false il valore di ritorno è indeterminato.
 */
  char prefix[]={'f','p','n','u','m','0','k','M','G','T','P'};
  int iTotPlot;
  EadjustType hAdjust, vAdjust;
  QString unitS=""; //E' l'unità di misura dell'asse corrente, valutata considerando i valori di autoLabelXY e useUserUnits (v. spiegazione inizio funzione).


  /***  Fase 1: Determino gli allineamenti ***/
 switch(axis.type){
   case atX:
     hAdjust=atCenter;
     vAdjust=atLeft;
     break;
   case atYL:
     hAdjust=atCenter;
     vAdjust=atCenter;
     break;
   case atYR:
     hAdjust=atCenter;
     vAdjust=atCenter;
     break;
 }

 /***  Fase 2: Determino il comportamento sulla base delle tre variabili booleane
  * autoLabelXY, useUserUnits, useSmartUnits (dettagli in Developer.odt|Gestione delle
  * etichette di asse.
 ***/
  bool autoLabelXY_=autoLabelXY;
 // 2.1: analizzo casi speciali
 if(axis.scaleType==stDB){
   unitS="dB";
//   autoLabelXY_=false;  //Questa riga va commentata perché posso volere l'etichetta su uno degli assi e dB sull'altro, mentre autoLabelXY_ agisce su tutti gli assi
   goto Escape;
 }

 if(!autoLabelXY_ && ! useUserUnits){
   unitS="";
   goto Escape;
 }

 if(abs(axis.scaleExponent)>18){  //caso speciale in cui devo forzare l'uso delle potenze di 10
     useUserUnits=false;
     autoLabelXY_=false;
     unitS="";
     goto Escape;
 }
 if(useUserUnits){
   switch(axis.type){
     case atX:  unitS=userUnits.x; break;
     case atYL: unitS=userUnits.y; break;
     case atYR: unitS=userUnits.ry; break;
   }
   goto Escape;
 }

 if(axis.scaleType==stLog){
   unitS="";
   autoLabelXY_=false;
   goto Escape;
 }

 if(axis.scaleExponent/3+5<0){
    autoLabelXY_=false;
    goto Escape;
 }
 if(axis.scaleExponent/3+5>=int(sizeof(prefix))){
    autoLabelXY_=false;
    goto Escape;
 }

//2.2: info da file. Se arrivo qui devo interpretare le unità in funzione di informazioni prelevate da metadati o dal nome di variabile prelevati dal file di input
if(axis.type==atX)
   unitS=xVarParam.unitS;
  else{
    for(iTotPlot=0; iTotPlot<numOfTotPlots; iTotPlot++){
      //Se una curva non va graficata sull'asse corrente non la considero:
      if(curveParamLst[iTotPlot].rightScale  && axis.type!=atYR) continue;
      if(!curveParamLst[iTotPlot].rightScale && axis.type==atYR) continue;
      //Se la variabile corrente ha unità indeterminata l'asse non potrà avere label testuale:
      if(curveParamLst[iTotPlot].unitS==""){
        unitS="";
        break;
      }
      // Tutte le variabili sul medesimo asse devono avere la stessa unità. Pertanto una volta definita la unit per una variabile dell'asse corrente anche le altre devono avere la stessa oppure non metto niente sull'asse.
      //devo però tener conto della regola, illustrata anche in Tutorial, che non si mette come unità automatica "s" sugli assi y e yr, anche per evitare he tutte le variabili TACS predano questa unità in modo improprio.
      if(unitS=="" && curveParamLst[iTotPlot].unitS!="s"){
        unitS=curveParamLst[iTotPlot].unitS;
      }else if(unitS!=curveParamLst[iTotPlot].unitS){
        unitS="";
        break;
      }
    }
  }

  /***  Fase 3: Determino nei vari casi base ed esponente (v. Developer.odt|Gestione
   * delle etichette di asse) ***/

/* Si ha l'obiettivo di passare prima o poi all'uso dell'unica funzione smartWriteUnit(). Per ora vi sono ancora delle difficoltà nel calcolo delle spaziature, e quindi nel caso della sola potenza di 10 uso ancora il più vecchio writeText2.
Notare che a writeText2 si passano separatamente base ed esponente, mentre in smartWriteUnit la potenza di 10 è automaticamente riconosciuta dalla presenza come primi tre caratteri di "*10"
Per avere compatibilità con entrambe le funzioni uso tre variabili testuali: msgBase, msgExp e unitS. Quest'ultima contiene tutto assieme secondo la convenzione per smartWriteUnit
*/
Escape:
  QString msgBase="", msgExp="";
  bool useWriteText2= (!autoLabelXY_ && !useUserUnits) || (useUserUnits && unitS=="");
  if(axis.scaleType==stDB)
      useWriteText2=false; //necessario perché "dB" è in unitS che non è usato da writetext2()
  if(useWriteText2){
      if(axis.scaleExponent!=0){
        msgBase="10";
        msgExp=QString::number(axis.scaleExponent);
      }else{
          msgBase="";
          msgExp="";
      }
  }else{
    if(prefix[axis.scaleExponent/3+5]!='0' && axis.scaleType!=stDB)
      unitS.prepend(prefix[axis.scaleExponent/3+5]);
  }

//Write:
  // La gestione delle parentesi la faccio qui, in modo che agisce sia nel caso di labels automatiche che specificate manualmente dall'utente
  //Aggiustamenti DPI-aware tarati a Feb 2017:
  if(axis.type==atX) Y+=int(1*onePixDPI);
  if(axis.type==atYL && axis.halfTicNum)
      X-=int(yAxis.ticPixWidth-1*onePixDPI);
  if(axis.type==atYR && axis.halfTicNum)
      X+=int(ryAxis.ticPixWidth-onePixDPI);
  if(axis.type==atYR && !axis.halfTicNum)
      X+=int(2*onePixDPI);

  /* Si ha l'obiettivo di passare prima o poi all'unica fuzione smartWriteUnit(). Per ora vi sono ancora delle difficoltà nel calcolo delle spaziature, e quindi nel caso della sola potenza di 10 uso ancora il più vecchio drawText2.
Notare che a drawText2 si passano separatamente base ed esponente, mentre in smartWriteUnit la potenza di 10 è automaticamente riconosciuta dalla presenza come primi tre caratteri di "10*"
*/

  if (useWriteText2){ //Caso residuo solo per le potenze di 10
      //qDebug()<<"Writing Axis Label through drawtext2 ";
      return writeText2(myPainter,X,Y,hAdjust,vAdjust,msgBase,msgExp,useBrackets, _virtual)+1;
  }else{  //caso in cui si possono usare lettere greche, puntini, esponenti
    return smartWriteUnit(myPainter, baseFont, X,Y,hAdjust,vAdjust,unitS,useBrackets, _virtual)+1;
  }
}

void  CLineChart::writeLegend(bool _virtual){

/* Quando _virtual=true la routine fa solo il calcolo di legendHeight e blocksPerRow. In caso contrario fa la scrittura.
_virtual=Questo artificio è molto utile per evitare di creare una routine indipendente per il calcolo delle altezze, con la possibilità sempre presente di un disallineamento con la routine di effettiva scrittura.
Esso è usato solo quando il file è unico, in quanto nella modalità a più files viene comunque fatto in due passate perché altrimenti il codice risultava di difficile scrittura.
Però mantenere le due passate all'interno di questo unico file aiuta molto.

La presente routine viene chiamata con _virtual=true solo da sé stessa, all'inizio delle attività, per l'appunto per calcolare le altezze, e quindi la posizione in cui scrivere.
Poi effettua la scrittura.
Il software esterno chiama prima writeLegend delle routine di esecuzione del grafico in modo che la legenda è già scritta in basso quando si fa il grafico, e lo spazio che occupa è quindi già noto.

La leggenda ha aspetto differente a seconda che si tratti di un grafico
le cui variabili provengono da diversi files, o da un unico file.

1) variabili provenienti da diversi files.
   In questo caso creo dei blocchi cosituiti dal nome del file e dalle relative variabili da plottare. Se in una riga ci stanno più blocchi ce li metto. In tal modo se ho molte variabili per file verrà un file per riga, altrimenti anche più files su una medesima riga.
Questo consente di sfruttare bene le tre righe disponibili per la leggenda sia nel caso in cui ho molti files e poche variabili per file che nel caso opposto di pochi files, ciascuno conmolte variabili.
I blocchi vengono calcolati assieme a legendHeight alla prima passata.
la variabile x è a comune ed è la variabile tempo e non è necessario riportarne il nome in leggenda. La leggenda contiene un paragrafo per ogni file riportante prima i nome del file, poi il carattere ':', poi l'elencazione delle variabili, eventualmente su più righe.

2) variabili provenienti tutte da unico file. In tal caso si potrebbe anche trattare di grafico di tipo XY. La leggenda contiene all'inizio, fra parentesi, informazione sul nome del file e della variabile x poi, dopo il carattere ':'. l'elenco delle variabili.

PER ENTRAMBI I CASI limito comunque la leggenda ad un massimo di 3 righe, rinunciando a riportare alcune variabili se questo spazio è insufficiente. L'utente avrà comunque la possibilità di avere informazione completa sui grafici visualizzati attivando la visione dei valori numerici e quindi la relativa leggenda.
 */
  int iTotPlot=-1;
  int numRows, xPosition=plotRect.left(), yPosition;
  int markerXPos, markerYPos;

  QString msg;

  hovDataLst.clear();
   if(!_virtual){
    writeLegend(true); //mette il valore in legendHeight;
  }
  lgdFont.setUnderline(false);
  myPainter->setFont(lgdFont);
  if(_virtual)
    yPosition=0;
  else
    yPosition=plotRect.y()+plotRect.height()-legendHeight+textHeight;
  if(legendHeight==-1)return;
  numRows=1;
  // Uso la leggenda con il nome di file all'inizio sia nel caso in cui nFIles=1 che nel caso che ho function plots tutte riferite al medesimo file.
  // per identificare la leggenda a file singolo invece di nFiles faccio un confronto dei nomi delle stringhe di file
  bool oneFileName=nFiles==1;
  if(nFiles>1){
    oneFileName=true;
    for (int i=0; i<nFiles; i++){
//      QString dbgStr=filesInfo[i].name;
      if(filesInfo[i].name!=filesInfo[0].name){
        oneFileName=false;
        break;
      }
    }
  }

  //Nel seguente caso di nFiles=1 il medesimo codice serve per il calcolo delle dimensioni della leggenda e l'effettiva scrittura:
  if(oneFileName){
    myPainter->setPen(Qt::black);
    msg="(file "+filesInfo[0].name + "; x-var "+xVarParam.name+")  ";
    if(!_virtual)myPainter->drawText(xPosition,yPosition,msg);
    xPosition+=myPainter->fontMetrics().horizontalAdvance(msg);
    while(1){
      iTotPlot++;
      if(iTotPlot==numOfTotPlots)    break;
      msg=curveParamLst[iTotPlot].name+"    ";
      if(curveParamLst[iTotPlot].rightScale)
       lgdFont.setUnderline(true);
      else
        lgdFont.setUnderline(false);
      myPainter->setFont(lgdFont);
      if(!blackWhite)
        myPainter->setPen(curveParamLst[iTotPlot].color);
      //Decido se devo andare a capo sulla base della stringa senza "    ", ma se poi scrivo metto la stringa con "    ":
      if(xPosition+myPainter->fontMetrics().horizontalAdvance(curveParamLst[iTotPlot].name)<plotRect.width()){
      if(!_virtual){
           myPainter->drawText(xPosition,yPosition,msg);
           struct SHoveringData hovData;
           hovData.rect=myPainter->boundingRect(xPosition,yPosition-textHeight,400,50,Qt::AlignLeft, msg);
           hovData.iTotPlot=iTotPlot;
           hovDataLst.append(hovData);
           markerXPos=xPosition+myPainter->fontMetrics().horizontalAdvance(curveParamLst[iTotPlot].name)+int(markHalfWidth)+1;
           markerYPos=int(yPosition-0.8f*myPainter->fontMetrics().height()+int(markHalfWidth));
           //Posizione dei marcatori vicino ai rispettivi nomi sulla leggenda:
           markPositions[iTotPlot].setX(markerXPos);
           markPositions[iTotPlot].setY(markerYPos);
        }
        xPosition+=myPainter->fontMetrics().horizontalAdvance(msg);
      }else{
        iTotPlot--;
        numRows++;
        yPosition+=textHeight;
        xPosition=0;
        if(numRows==4){
          numRows--;
          break;
        }
      }
    }
    legendHeight=int((numRows+0.5f)*textHeight);
    return;
  }
  //Nel seguente caso di nFiles>1 il codice è organizzato su due passate e si fanno tutte con Virtual = false:
  if(!oneFileName && !_virtual){
    bool endBlock;
    int count=0, //generico contatore
    iRow, //indice della riga corrente
    nRows, //numero di righe scritte
    blocksPerRow[3],  //quanti bloccki per riga
    iTotPlStartBlock; //indice globale di variabile di inizio blocco
    iRow=0;
    blocksPerRow[0]=blocksPerRow[1]=blocksPerRow[2]=0;
    msg = "file";
    for(int iFile=0; iFile<nFiles; iFile++){
      endBlock=false;
      iTotPlStartBlock=iTotPlot;
      blocksPerRow[iRow]++;
      if(xPosition==0)
        msg=msg+filesInfo[iFile].name+":  ";
      else
        msg=msg+"  "+filesInfo[iFile].name+":  ";
      for(int iVar=0; iVar<nPlots[iFile]; iVar++){
        iTotPlot++;
        msg=msg+curveParamLst[iTotPlot].name+"   ";
 //Se sforo il fine riga tolgo il blocco corrente dalla riga corrente a meno che non sia l'unico blocco:
        if(myPainter->fontMetrics().horizontalAdvance(msg)>plotRect.width()){
          if(blocksPerRow[iRow]>1){
            blocksPerRow[iRow]--;
            iTotPlot=iTotPlStartBlock;
            msg="";
            iFile--;
            count++;
            if(iVar<0||count>3){
              QMessageBox::critical(this,"CLineChart","error in writeLegend");
              qApp->quit();
            }
          }
          iRow++;
          endBlock=true;
          break;
        }
     }
     if(endBlock){
       if(iRow<3)
         continue;
       else
         break;
      }
    }
    nRows=3;
    if(blocksPerRow[2]==0)nRows=2;
    if(blocksPerRow[1]==0)nRows=1;
    legendHeight=(nRows+1)*textHeight;
    //Ora le dimensioni sono calcolate, faccio l'effettiva scrittura:
    yPosition=plotRect.y()+plotRect.height()-legendHeight+textHeight;
    int curBlock=0;
    iTotPlot=-1;
    xPosition=0;
    iRow=0;
    for(int iFile=0; iFile<nFiles; iFile++){
      curBlock++;
      if(curBlock>blocksPerRow[iRow]){
        curBlock=1;
        iRow++;
        if(iRow>2)break;
        xPosition=0;
        yPosition+=textHeight;
      }
      myPainter->setPen(Qt::black);
      if(xPosition==0)
        msg="file "+filesInfo[iFile].name+":  ";
      else
        msg="  file "+ filesInfo[iFile].name+":  ";
      myPainter->drawText(xPosition,yPosition,msg);
      //Ora che ho scritto ad inizio riga il nome del file scrivo l'elenco delle corrispondenti variabili:
      xPosition+=myPainter->fontMetrics().horizontalAdvance(msg);
      for(int iVar=0; iVar<nPlots[iFile]; iVar++){
        iTotPlot++;
        if(!blackWhite)
          myPainter->setPen(curveParamLst[iTotPlot].color);
        msg=curveParamLst[iTotPlot].name+"   ";
        lgdFont.setUnderline(curveParamLst[iTotPlot].rightScale);
        myPainter->setFont(lgdFont);
        myPainter->drawText(xPosition,yPosition,msg);
        struct SHoveringData hovData;
        hovData.rect=myPainter->boundingRect(xPosition,yPosition-textHeight,400,50,Qt::AlignLeft, msg);
        hovData.iTotPlot=iTotPlot;
        hovDataLst.append(hovData);
        markerXPos=xPosition+myPainter->fontMetrics().horizontalAdvance(curveParamLst[iTotPlot].name)+int(markHalfWidth)+1;
        markerYPos=yPosition-int(0.8f*myPainter->fontMetrics().height()+int(markHalfWidth));
        xPosition+=myPainter->fontMetrics().horizontalAdvance(msg);
       //Posizione dei marcatori vicino ai rispettivi nomi sulla leggenda:
       markPositions[iTotPlot].setX(markerXPos);
       markPositions[iTotPlot].setY(markerYPos);
     }
      lgdFont.setUnderline(false);
      myPainter->setFont(lgdFont);
   }
 }
}

#include <math.h>
#define abs(x) (((x)>0?(x):(x)*(-1)))
/*$$$*******************************************************************/
/**************Righe da FilterClip.cpp**********************************/
/***********************************************************************/

CLineChart::CFilterClip::CFilterClip(){
    maxErr=0.5;
    lineDefined=false;
}


bool CLineChart::CFilterClip::getLine(float X1_, float Y1_, float X2_, float Y2_){
    /* Descrivo l'equazione della retta tramite Ax+By+C=0, nella quale pongo B=1.
    */
    X1=X1_;	Y1=Y1_; X2=X2_;	Y2=Y2_;
    lineDefined=true;
    if(X2==X1)
      if(Y1==Y2){
        lineDefined=false;
        /* Le seguenti due righe servono in quanto, se i due punti passati sono coincidenti
        ometterò di tracciare un eventuale successivo punto solo se coincidente con
        il precedente.*/
        lastX=X1;
        lastY=Y1;
        A=0.0f;
        C=X1;
        return lineDefined;
      }else{
        A=1.f;
        B=0.f;
        C=-X1;
        aux=1.f;
      }
    else{
      A=(Y1-Y2)/(X2-X1);
      B=1.f;
      C=-A*X2-Y2;
      aux=sqrtf(A*A+1.f);
    }
    //Aux=sqrt(A*A+B*B);
    Vector.X=X2-X1;
    Vector.Y=Y2-Y1;
    return lineDefined;
}

void CLineChart::CFilterClip::getRect(int X0, int Y0, int X1, int Y1){
    R.Left=X0;
    R.Top=Y0;
    R.Right=X1;
    R.Bottom=Y1;
}

bool  CLineChart::CFilterClip::isInRect(float X, float Y){
    if(X>=R.Left && X<=R.Right &&	Y>=R.Top && Y<=R.Bottom)
        return true;
    else
        return false;
}

bool  CLineChart::CFilterClip::isRedundant(float X, float Y){
    //ritorna true se il punto passato è all'interno della striscia
    float dist;
    /* Se non sono riuscito a definire in precedenza una retta era perché avevo
    utilizzato due punti coincidenti,
    */
    if(!lineDefined) {
        if(X==lastX && Y==lastY)
            return true;
        else
            return false;
    }
    dist=abs(A*X+B*Y+C)/aux;
    if (dist<maxErr){
        if( Vector.X*(X-X2) + Vector.Y*(Y-Y2) >=0  || strongFilter ){
            X2=X;
            Y2=Y;
            return true;
        }
    }
    return false;
}

int CLineChart::CFilterClip::giveRectIntersect(FloatPoint & I1, FloatPoint &I2){
    /* Questa funzione copia nei parametri passati i valori delle eventuali intersezioni
    del segmento congiungente i due punti interni (X1,Y1) e (X2,Y2), passati con la
    precedente getLine, con il rettangolo R passato con il recente GetRect.
    La funzione ritorna il numero di intersezioni trovate.
    */
    float X;
    FloatPoint P[2];
    int iPoint=-1;

    if(giveX(R.Top,X))
        if(X>=R.Left && X<=R.Right){
            P[++iPoint].X=X;
            P[iPoint].Y=R.Top;
        }
    if(giveX(R.Bottom,X))
        if(X>=R.Left && X<=R.Right){
            P[++iPoint].X=X;
            P[iPoint].Y=R.Bottom;
        }
    if(giveY(R.Left,X))
        if(X>=R.Top && X<=R.Bottom){
            P[++iPoint].X=R.Left;
            P[iPoint].Y=X;
        }
    if(giveY(R.Right,X))
        if(X>=R.Top && X<=R.Bottom){
            P[++iPoint].X=R.Right;
            P[iPoint].Y=X;
        }
    switch (iPoint){
        case -1:
            return 0;
        case 0:
            I1=P[0];
            return 1;
        case 1:
            //Se sono stati trovati due punti devo individuare quale è il primo che si incontra
            //andando da (X1,Y1) a (X2,Y2).
            if( (P[0].X-X1)*(P[0].X-X1) + (P[0].Y-Y1)*(P[0].Y-Y1) <
                    (P[1].X-X1)*(P[1].X-X1) + (P[1].Y-Y1)*(P[0].Y-Y1)  ) {
                I1=P[0];
                I2=P[1];
            }else{
                I1=P[1];
                I2=P[0];
            }
    }
    return 2;
}

float inline CLineChart::CFilterClip::giveX(float Y){
    /* Funzione che mi dà l'ascissa X dell'intersezione con una data Y
    della retta passante per i punti interni (X1,Y1), (X2,Y2).
    */
    return -(B*Y+C)/A;
}

bool CLineChart::CFilterClip::giveX(float Y, float &X){
    /* Funzione che cerca l'intersezione con una data Y	del segmento che congiunge i punti
    interni (X1,Y1), (X2,Y2), e ne mette in X il valore.
    Gli estremi sono esclusi.
    */
    float factor;
    if(Y2==Y1) return false;
    factor=(Y-Y1)/(Y2-Y1);
    if(factor>0 && factor<=1){
        X=X1 + factor*(X2-X1);
        return true;
    }else
        return false;
}

float inline CLineChart::CFilterClip::giveY(float X){
    /* Funzione che mi d‡ l'ordinata Y dell'intersezione con una data X
    della retta passante per i punti interni (X1,Y1), (X2,Y2).
    */
    return -(A*X+C)/B;
}

bool CLineChart::CFilterClip::giveY(float X, float & Y){
    /* Funzione che cerca l'intersezione con una data X	del segmento che congiunge i punti
    interni (X1,Y1), (X2,Y2), e ne mette in Y il valore.
    Gli estremi sono esclusi.
    */
    float Factor;
    if(X2==X1) return false;
    Factor=(X-X1)/(X2-X1);
    if(Factor>0 && Factor<1){
        Y=Y1 + Factor*(Y2-Y1);
        return true;
    }else
        return false;
}


CLineChart::CFilterClipD::CFilterClipD(){
    maxErr=0.5;
    lineDefined=false;
}


bool CLineChart::CFilterClipD::getLine(double X1_, double Y1_, double X2_, double Y2_){
    /* Descrivo l'equazione della retta tramite Ax+By+C=0, nella quale pongo B=1.
    */
    X1=X1_;	Y1=Y1_; X2=X2_;	Y2=Y2_;
    lineDefined=true;
    if(X2==X1)
      if(Y1==Y2){
        lineDefined=false;
        /* Le seguenti due righe servono in quanto, se i due punti passati sono coincidenti
        ometterò di tracciare un eventuale successivo punto solo se coincidente con
        il precedente.*/
        lastX=X1;
        lastY=Y1;
        A=0.0;
        C=X1;
        return lineDefined;
      }else{
        A=1.;
        B=0.;
        C=-X1;
        aux=1.;
      }
    else{
      A=(Y1-Y2)/(X2-X1);
      B=1.;
      C=-A*X2-Y2;
      aux=sqrt(A*A+1.);
    }
    //Aux=sqrt(A*A+B*B);
    Vector.X=X2-X1;
    Vector.Y=Y2-Y1;
    return lineDefined;
}

void CLineChart::CFilterClipD::getRect(int X0, int Y0, int X1, int Y1){
    R.Left=X0;
    R.Top=Y0;
    R.Right=X1;
    R.Bottom=Y1;
}

bool  CLineChart::CFilterClipD::isInRect(double X, double Y){
    if(X>=R.Left && X<=R.Right &&	Y>=R.Top && Y<=R.Bottom)
        return true;
    else
        return false;
}

bool  CLineChart::CFilterClipD::isRedundant(double X, double Y){
    //ritorna true se il punto passato è all'interno della striscia
    double dist;
    /* Se non sono riuscito a definire in precedenza una retta era perché avevo
    utilizzato due punti coincidenti,
    */
    if(!lineDefined) {
        if(X==lastX && Y==lastY)
            return true;
        else
            return false;
    }
    dist=abs(A*X+B*Y+C)/aux;
    if (dist<maxErr){
        if( Vector.X*(X-X2) + Vector.Y*(Y-Y2) >=0  || strongFilter ){
            X2=X;
            Y2=Y;
            return true;
        }
    }
    return false;
}

int CLineChart::CFilterClipD::giveRectIntersect(DoublePoint & I1, DoublePoint &I2){
    /* Questa funzione copia nei parametri passati i valori delle eventuali intersezioni
    del segmento congiungente i due punti interni (X1,Y1) e (X2,Y2), passati con la
    prescedente getLine, con il rettangolo R passato con il recente GetRect.
    La funzione ritorna il numero di intersezioni trovate.
    */
    double X;
    DoublePoint P[2];
    int iPoint=-1;

    if(giveX(R.Top,X))
        if(X>=R.Left && X<=R.Right){
            P[++iPoint].X=X;
            P[iPoint].Y=R.Top;
        }
    if(giveX(R.Bottom,X))
        if(X>=R.Left && X<=R.Right){
            P[++iPoint].X=X;
            P[iPoint].Y=R.Bottom;
        }
    if(giveY(R.Left,X))
        if(X>=R.Top && X<=R.Bottom){
            P[++iPoint].X=R.Left;
            P[iPoint].Y=X;
        }
    if(giveY(R.Right,X))
        if(X>=R.Top && X<=R.Bottom){
            P[++iPoint].X=R.Right;
            P[iPoint].Y=X;
        }
    switch (iPoint){
        case -1:
            return 0;
        case 0:
            I1=P[0];
            return 1;
        case 1:
            //Se sono stati trovati due punti devo individuare quale è il primo che si incontra
            //andando da (X1,Y1) a (X2,Y2).
            if( (P[0].X-X1)*(P[0].X-X1) + (P[0].Y-Y1)*(P[0].Y-Y1) <
                    (P[1].X-X1)*(P[1].X-X1) + (P[1].Y-Y1)*(P[0].Y-Y1)  ) {
                I1=P[0];
                I2=P[1];
            }else{
                I1=P[1];
                I2=P[0];
            }
    }
    return 2;
}

double inline CLineChart::CFilterClipD::giveX(double Y){
    /* Funzione che mi dà l'ascissa X dell'intersezione con una data Y
    della retta passante per i punti interni (X1,Y1), (X2,Y2).
    */
    return -(B*Y+C)/A;
}

bool CLineChart::CFilterClipD::giveX(double Y, double &X){
    /* Funzione che cerca l'intersezione con una data Y	del segmento che congiunge i punti
    interni (X1,Y1), (X2,Y2), e ne mette in X il valore.
    Gli estremi sono esclusi.
    */
    double factor;
    if(Y2==Y1)
        return false;
    factor=(Y-Y1)/(Y2-Y1);
    if(factor>0 && factor<=1){
        X=X1 + factor*(X2-X1);
        return true;
    }else
        return false;
}

double inline CLineChart::CFilterClipD::giveY(double X){
    /* Funzione che mi d‡ l'ordinata Y dell'intersezione con una data X
    della retta passante per i punti interni (X1,Y1), (X2,Y2).
    */
    return -(A*X+C)/B;
}

bool CLineChart::CFilterClipD::giveY(double X, double & Y){
    /* Funzione che cerca l'intersezione con una data X	del segmento che congiunge i punti
    interni (X1,Y1), (X2,Y2), e ne mette in Y il valore.
    Gli estremi sono esclusi.
    */
    double factor;
    if(X2==X1)
        return false;
    factor=(X-X1)/(X2-X1);
    if(factor>0 && factor<1){
        Y=Y1 + factor*(Y2-Y1);
        return true;
    }else
        return false;
}

