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

#include "CFourWin.h"
#include "ui_CFourWin.h"
#include "SuppFunctions.h"
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QFileDialog>
#include <QClipboard>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QScreen>
#include <QSettings>
#include <complex>
#define SQRT2 1.414213562
#define _max(a, b)  (((a) > (b)) ? (a) : (b))
#define _min(a, b)  (((a) < (b)) ? (a) : (b))

typedef std::complex <double>dcmplx;


void CFourWin::lineChatClickedOn(void){
  QFocusEvent *e=nullptr;
    focusInEvent(e);
}

CFourWin::CFourWin(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CFourWin)
{
  ui->setupUi(this);
  setWindowFlags(windowFlags() | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
  //I font fissati in Win MS Shell Dlg 8 punti vencongono convertiti in Ubuntu in font 11 punti"!
#if defined(Q_OS_UNIX)
  QFont font=ui->saveSetBtn->font();
  font.setPointSize(8);
  ui->saveSetBtn->setFont(font);
  ui->gridChkBox->setFont(font);
  ui->iniTimeLbl->setFont(font);
  ui->finTimeLbl->setFont(font);
  ui->infoBtn->setFont(font);
  ui->valuesBox->setFont(font);
  font.setPointSize(7);
  ui->harmNLbl->setFont(font);
  ui->harmValLbl->setFont(font);
#endif

  dftDone=false;
  optionsSetOk=false;
  harmOrders=nullptr;
  ampl=nullptr;
  amplitudes=nullptr;
  phases=nullptr;
  ui->amplChart->plotType=ptBar;
  ui->phaseChart->plotType=ptBar;
  amplValueTxt="<i>value (peak):</i><br>";
//    initialFontPoints=font().pointSize();
  ui->amplChart->addLegend=false;
  ui->amplChart->linearInterpolate=false;
  ui->amplChart->setActiveDataCurs(1);
  ui->amplChart->yGrid=ui->gridChkBox->isChecked();
  ui->phaseChart->addLegend=false;
  ui->phaseChart->linearInterpolate=false;
  ui->phaseChart->setActiveDataCurs(1);
  ui->phaseChart->yGrid=ui->gridChkBox->isChecked();
  ui->saveSetLbl->setVisible(false);
  connect(ui->amplChart,SIGNAL(valuesChanged(SXYValues,bool,bool)),this, SLOT(valChangedAmp(SXYValues,bool,bool)));
  connect(ui->phaseChart,SIGNAL(valuesChanged(SXYValues,bool,bool)),this, SLOT(valChangedPh(SXYValues,bool,bool)));

  /* I seguenti due connect servono per evitare che se l'utente clicca nell'area LineChart di una FourWin, dopo i primi click, non avviene più lo switch automatico della tab. Infatti l'evento focusInEvent, si attiva solo se il click avviene sulla riga di intestazione della finestra FourWin,  o la prima volta che si clicca sull'area di LineChart. Le volte successive non si attiva; per fare lo switch della tab anche in questo caso, catturo dentro LineChart il comando mousePressEvent():
*/
  connect(ui->amplChart,SIGNAL(chartClickedOn()),this, SLOT(lineChatClickedOn()));
  connect(ui->phaseChart,SIGNAL(chartClickedOn()),this, SLOT(lineChatClickedOn()));


  fourOptions= new CFourOptions(this);
  fourOutInfo= new CFourOutputInfo(this);
  QScreen *screen=QGuiApplication::primaryScreen();
  double myDPI=screen->logicalDotsPerInch();

  //rendo L'ASPETTO DPI-aware
  if(myDPI>100.){
    // Qui sono nella fase della costruzione, quindi prima del caricamento del registry. Pertanto le modifiche al size che faccio qui non hanno influenza su quello che poi è salvato dall'utente. Mi posso quindi permettere le seguenti righe:
    int w=geometry().width();
    int h=geometry().height();

    resize(int(qMin(1.3,myDPI/96.0))*w,int(qMin(1.1,myDPI/96.0))*h);

    w=ui->frame->maximumSize().width();
    ui->frame->setMaximumWidth(w*int(qMax(1.0,0.90*myDPI/96.0)));
    w=ui->frame->minimumSize().width();
    ui->frame->setMinimumWidth(w*int(qMax(1.0,0.95*myDPI/96.0)));
  }

 #if defined(Q_OS_MAC)
  QFont dummyF=ui->harmNLbl->font();
  int dummySize=dummyF.pointSize();
  dummySize*=1.5;
  dummyF.setPointSize(dummySize);
  ui->gridChkBox->setFont(dummyF);
  ui->infoBtn->setFont(dummyF);
  ui->iniTimeLbl->setFont(dummyF);
  ui->finTimeLbl->setFont(dummyF);
  ui->valuesBox->setFont(dummyF);
  ui->harmNLbl->setFont(dummyF);
  ui->harmValLbl->setFont(dummyF);
  ui->saveSetBtn->setFont(dummyF);
#else
  ;
  //Per ragioni misteriose sul Vaio il font delle due label vengono troppo grossi, ed in particolare più grossi di quelli del tempo iniziale e finale! Per ora faccio un aggiustamento euristico:
  /*
  if(myDPI>100){
      QFont dummyF=ui->harmNLbl->font();
      int dummySize=dummyF.pointSize();
      dummySize*=0.85;
      dummyF.setPointSize(dummySize);
      ui->harmNLbl->setFont(dummyF);
      ui->harmValLbl->setFont(dummyF);
  }
  */
 #endif

}

int  CFourWin::analyseAndShow(bool changed){
    /* Questa funzione analizza le opzioni e aggiorna i grafici di ampiezza e/o fase.
     * Essa è richiamata sia allo show della finestre (in quel caso changed è true), che
     * dopo il click sul bottone delle opzioni (in quel caso changed=false mi evita
     * di rifare taluni calcoli inutili).
*/
    int ret=0;
    int harm, harm1, harm2;
    float amplFactor;
    QString msg;
    msg.setNum(myData.opt.initialTime,'g',5);
    ui->iniTimeLbl->setText("t1: "+msg);

    msg.setNum(myData.opt.finalTime,'g',5);
    ui->finTimeLbl->setText("t2: "+msg);

    //Eventuale ricalcolo.
    if(changed){
        indexesFromTimes(myData);
        // La seguente riga è funzionante e può essere riattivata in qualunque momento. Ovvio che non ha senso calcolare due volte i coefficienti di fourier, prima con l'algoritmo a campioni equispaziati e poi a campioni non equispaziati, pertanto nella versione release il calcolo per campioni equispaziati (prossima riga) va diattivato.
//        ret=performDFT();
        ret=performNuDFT();
        if (ret>0)
          return ret;
    }
    //Eventuale correzione con amplFactor
    harm1=myData.opt.harm1;
    harm2=myData.opt.harm2;
    switch(myData.opt.amplUnit){
       case peak:
        amplFactor=1.0;
        amplValueTxt="<i>value (peak):</i><br>";
        break;
       case rms:
        amplFactor=float(1.0/SQRT2);
        amplValueTxt="<i>value (rms):</i><br>";
        break;
       case puOf0:
        amplFactor=float((100.0/SQRT2)/fabs(double(ampl01[0])));
        amplValueTxt="<i>value (%/h0):</i><br>";
        break;
       case puOf1:
        amplFactor=float(100./fabs(double(ampl01[1])));
        amplValueTxt="<i>value (%/h1):</i><br>";
        break;
    }
    ui->harmValLbl->setText(amplValueTxt);

    amplitudes[0]=ampl[0];
    if(myData.opt.amplUnit==puOf0)
        amplitudes[0]=100.0;
    if(myData.opt.amplUnit== puOf1)
        amplitudes[0]=fabsf(ampl[0]/ampl[1]);
    for(harm=_max(1,harm1); harm<=harm2; harm++)
        amplitudes[harm]=ampl[harm]*amplFactor;

    //Aggiornamento grafici.
    amplitudes1=amplitudes+harm1;
    phases1=phases+harm1;
    ui->amplChart->getData(harmOrders+harm1, &amplitudes1, harm2-harm1+1);
    ui->phaseChart->getData(harmOrders+harm1, &phases1, harm2-harm1+1);
    ui->amplChart->yGrid=ui->gridChkBox->isChecked();
    ui->phaseChart->yGrid=ui->gridChkBox->isChecked();
    ui->amplChart->plot(true);
    ui->phaseChart->plot(true);
    ui->phaseChart->setVisible(true);
    if(myData.opt.amplSize==fifty){
      ui->plotsLayout->setStretch(0,5);
      ui->plotsLayout->setStretch(1,5);
    }
    if(myData.opt.amplSize==seventy){
      ui->plotsLayout->setStretch(0,7);
      ui->plotsLayout->setStretch(1,3);
    }
    if(myData.opt.amplSize==hundred){
      ui->phaseChart->setVisible(false);
    }
    computeTHD();
    QString msgRMS, msg0, msg1, msg2;
    msgRMS.setNum(RMS,'g',5);
    msg0.setNum(THD0,'g',5);
    msg1.setNum(THD1,'g',5);
    msg2.setNum(PWHC,'g',5);

    //La seguente infoString viene poi visualizzata alla pressione del bottone "?"
    infoString="";
    if(myData.opt.harm1==0)
       infoString= "RMS: "+msgRMS+"; THD0: "+msg0+"%; THD1: "+msg1+"%";
    if(myData.opt.harm1==1)
       infoString= "RMS: "+msgRMS+"; THD1: "+msg1+"%";
    if(myData.opt.harm1>1)
       infoString= "RMS: "+msgRMS;
    if(myData.opt.harm1<=14 && myData.opt.harm2>=40)
        infoString= infoString+"\n IEC 61000-3-2 PWHC: "+msg2;

    return ret;
}


void CFourWin::computeTHD(){
  float THD=0, work=0;
  if(myData.opt.harm1>2)
      return;
  // Here I exclude harm1, which I'll introduce later only for THD0:
  for(int harm=2; harm<=myData.opt.harm2; harm++)
    THD+=amplitudes[harm]*amplitudes[harm];
  if (myData.opt.harm1<=14 && myData.opt.harm2>=40){
      for(int harm=14; harm<=40; harm++)
        work+=harm*amplitudes[harm]*amplitudes[harm];
   PWHC=sqrtf(work);
  }

  // THD % relative ad armoniche 0 e 1:
  if(amplitudes[0]==0)
      return;
  THD0=float(100.0*(sqrt(double(THD+amplitudes[1]*amplitudes[1])))/fabs(double(amplitudes[0])));
  if(amplitudes[1]==0)
      return;
  THD1=float(100*sqrt(double(THD))/fabs(double(amplitudes[1])));
}

void CFourWin::copyOrPrint(EOutType type){

  /* Con questa funzione si può fare un copy o print sia dei dati numerici che di uno
   * o entrambi i grafici.
   * Se si fanno i dati numerici l'uscita è di tipo testo
   * Se si fa uno o più diagrammi nella clipboard viene messa un'immagine. Però, per
   * comodità dell'utente nella parte superiore vengono scritte nell'imagine informazioni
   * utili quali il nome del file e della variabile, l'intervallo temporale su cui è
   * fatta l'analisi, e le armoniche considerate.
   * L'utente finale potrà tranquillamente tagliare questa parte, se non di suo interesse.
*/
  bool ok=true;
  bool avoidBoldRow=true; //this row is a flag to avoid writing bold row when copying plots. For the time being it is not available to tfrom the GUI.
  int imageWidth, imageHeight; //larghezza e altezza dell'immagine combinata contenente intestazione e grafici a barre (o grafico a barre)
  int yPosition;
  QClipboard *clipboard = QApplication::clipboard();
  QImage * amplImg=nullptr, *phImg=nullptr, *combinedImage;
  QPainter *painter;
  QString pdfFileName, fullFileName;
  QString dateStr=QDateTime::currentDateTime().date().toString();
//  fourOutInfo->setPhaseChart(ui->phaseChart->isVisible());
  fourOutInfo->setType(type);

  if(type==otCopy)
    fourOutInfo->setWindowTitle("Fourier Copy Options");
  else
    fourOutInfo->setWindowTitle("Fourier Print Options");

  fourOutInfo->exec();
  if(!fourOutInfo->accepted)return;

  QImage dummyImage(10,10,QImage::Format_RGB32);
  QRect rect, dummyRect(0,0,10,10), headRect1, headRect2;
  QString headText1, headText2; // testo che si mette all'inizio in tutti i casi; la prima variabie in bold
  headText1="MC's PlotXY - Fourier chart(s). Copied on "+ dateStr.mid(4);
  headText2="File: "+myData.fileName +"; Variable: "+myData.varName+"\n";
  headText2+=QString("t1: %1; ").arg(double(myData.opt.initialTime),0,'g',5);
  headText2+=QString("t2: %1      -  ").arg(double(myData.opt.finalTime),0,'g',5);
  switch (myData.opt.amplUnit){
//enum EAmplUnit {peak, rms, puOf0, puOf1};
  case  peak:
    headText2+= "Amplitude: peak value";
      break;
  case  rms:
    headText2+= "Amplitude: rms value";
    break;
  case puOf0:
    headText2+= "Amplitude: rms % of 0-order value";
    break;
  case puOf1:
    headText2+= "Amplitude: % of 1-order value";
    break;
  }

  if(fourOutInfo->numData){
   //E' stato richiesto un copy dei valori numerici.
     QMessageBox MB;
     if(type==otCopy){
       QString allText;
       allText=headText1+"\n"+headText2;
       allText+="\n\nHarm.\tAmplitude\tPhase\n";
       for(int i=myData.opt.harm1; i<=myData.opt.harm2; i++){
         allText+=QString("%1").arg(i);
         allText+="\t";
         allText+=QString("%1").arg(double(amplitudes[i]),8);
         allText+="\t";
         allText+=QString("%1").arg(double(phases[i]),8);
         allText+="\n";
       }
       clipboard->setText(allText);
       MB.setText("Fourier numerical data copied into the system clipboard");
     }else{
       //E' stato richiesto un print
       QPrinter myPrinter;
       myPrinter.setDocName("MC's PlotXY - Fourier Print");
       if(fourOutInfo->pdfOutput){
         pdfFileName = QFileDialog::getSaveFileName(this, "Save File", QDir::currentPath(),
             "PDF  (*.pdf)");
         if (pdfFileName=="") return;
         myPrinter.setOutputFileName(pdfFileName);
         QFileInfo FI(pdfFileName);
         FI.makeAbsolute();
         fullFileName=FI.filePath();
         MB.setText("Fourier numerical data printed on file \n"+fullFileName);
       }else
         MB.setText("Fourier numerical data printed on the default system printer");

       //Questa parte vale sia per stampa su stampante fisica che su pdf:
       QPainter prPainter(&myPrinter);
       int xPos=0, yPos=0;
       prPainter.drawText(xPos,yPos,headText1);
       yPos+=prPainter.fontMetrics().height();
       prPainter.drawText(xPos,yPos,headText2);
       yPos+=2*prPainter.fontMetrics().height();
       prPainter.drawText(xPos,yPos,"nHarm.");
       xPos+=50;
       prPainter.drawText(xPos,yPos,"tAmplitude");
       xPos+=100;
       prPainter.drawText(xPos,yPos,"Phase");
       yPos+=prPainter.fontMetrics().height();
       for(int i=myData.opt.harm1; i<=myData.opt.harm2; i++){
         xPos=0;
         prPainter.drawText(xPos,yPos,QString("%1").arg(i));
         xPos+=50;
         prPainter.drawText(xPos,yPos,QString("%1").arg(double(amplitudes[i]),8));
         xPos+=100;
         prPainter.drawText(xPos,yPos,QString("%1").arg(double(phases[i]),8));
         yPos+=prPainter.fontMetrics().height();
       }
     }
     if(ok)
       MB.exec();
    return;
  } else{   //Ora non sono più in numerical data ma in stampa dei plot
      // Calcolo lo spazio necessario al tracciamento di HeadText, e lo metto in dummyRect
      QPainter * painter;
      painter = new QPainter(&dummyImage);
      QFont font=painter->font();
      font.setBold(true);
      painter->setFont(font);
      painter->drawText(dummyRect,0,headText1,&headRect1);
      painter->drawText(dummyRect,0,headText2,&headRect2);
      delete painter;
  }

  //ora che ho le dimensioni della parte testuale posso allocare lo spazio alla combinedImage.

  if(avoidBoldRow)
    imageHeight=headRect2.height();
  else
    imageHeight=headRect1.height()+headRect2.height();
  imageWidth=qMax(headRect1.width(),headRect2.width());
  if(fourOutInfo->amplChart){
      amplImg=ui->amplChart->giveImage();
      imageHeight+=amplImg->rect().height();
      imageWidth=qMax(imageWidth,amplImg->rect().width());
  }
  if(fourOutInfo->phaseChart){
      phImg=ui->phaseChart->giveImage();
      imageHeight+=phImg->rect().height();
      imageWidth=qMax(imageWidth,phImg->rect().width());
  }
  //Ora posso allocare combinedImage che conterrà il testo più il o i grafici.
  combinedImage=new QImage(imageWidth, imageHeight,QImage::Format_RGB32);
  combinedImage->fill(Qt::white);
  painter = new QPainter(combinedImage);

  //Scrittura parte testuale.
  //la prima riga la metto in bold:
   QFont font=painter->font();
   font.setBold(true);
   painter->setFont(font);
   if(!avoidBoldRow)
     painter->drawText(combinedImage->rect(),0,headText1);
   //le altre righe sono a spessore normale:
   font.setBold(false);
   painter->setFont(font);
   if(avoidBoldRow)
     yPosition=0;
   else
     yPosition=headRect1.height();
   rect=combinedImage->rect();
   rect.moveTop(yPosition);
   painter->drawText(rect,0,headText2);

   if(fourOutInfo->amplChart){
     if(avoidBoldRow)
        yPosition=headRect2.height();
     else
        yPosition=headRect1.height()+headRect2.height();
     rect=amplImg->rect();
     rect.moveTop(yPosition);
     painter->drawImage(rect,*amplImg);
   }
   if(fourOutInfo->phaseChart){
     yPosition=headRect1.height()+headRect2.height();
     if(fourOutInfo->amplChart)
       yPosition+=amplImg->rect().height();
     rect=phImg->rect();
     rect.moveTop(yPosition);
     painter->drawImage(rect,*phImg);
   }
   QMessageBox MB;
   bool criticalMB=false;
   if(type==otCopy){
     clipboard->setImage(*combinedImage);
   //Con la precedente riga si è passata la ownership di Image alla clipboard. Quando verrà copiato in essa nuovo materiale il contenuto verrà liberato. E' pertanto vietata la seguente riga (ora commentata):
   //     delete combinedImage;
     MB.setText("Fourier plot(s) data copied into the system clipboard");
   }else{  //otPrint
       QPrinter myPrinter;
       qDebug()<<"printer is valid: " << myPrinter.isValid();
       if(fourOutInfo->pdfOutput){  //print su file pdf
         pdfFileName = QFileDialog::getSaveFileName(this, "Save File", QDir::currentPath(),
               "PDF  (*.pdf)");

         if (pdfFileName=="") return;
         myPrinter.setOutputFileName(pdfFileName);
         QFileInfo FI(pdfFileName);
         FI.makeAbsolute();
         fullFileName=FI.filePath();
         MB.setText("Fourier plot(s) printed on file \n"+fullFileName);
       }else
         MB.setText("Fourier plot(s) printed on the default system printer");

       QPainter prPainter;
       ok=prPainter.begin(&myPrinter);
       if(ok)
         prPainter.drawImage(combinedImage->rect(),*combinedImage);
       else{
         MB.setIcon(QMessageBox::Critical);
         MB.setText("Unable to open the default system printer");
         criticalMB=true;
       }
   }
   delete painter;
   // Originally I wanted to always issue MB.exec(). In pcarctice it si annoying, so I decide to issue it only in case of a critical message:
   if(criticalMB)
     MB.exec();

}

void CFourWin::focusInEvent(QFocusEvent *){
    /* Quando entro in una certa istanza della finestra devo fare in modo che la corrispondente tabella varSel venga evidenziata. Pertanto prelevo il digit che caratterizza il numero della finestra considerata, che è nel titolo, lo metto nell'int "i" e emetto sul segnale che verrà utilizzato da dataSelWin per attivare la corrispondente pagina del TabSheet.
  */


  // **** NOTA DICEMBRE 2019
  // **** NON SI SA PERCHE' QUESTA FUNZIONE NON VIENE MAI RICHIAMATA.
  // **** CODICE IDENTICO IN CPLOTWIN FUNZIONA PERFETTAMENTE
  // ****

    // La seguente riga non va bene quando l'opzione "/set" è selezionata. perché ovviamente cambia l'ultimo carattere.
    //    QChar c=windowTitle()[windowTitle().count()-1];
    QChar c=windowTitle()[windowTitle().size()-1];
    QString s=QString(c);
    int i=s.toInt();
    emit winActivated(i);
    raise();
}

void CFourWin::getData(struct SFourData data_){
  /* Questa funzione viene richiamata da CDataSelWin per inviare a Fourier i dati per il calcolo così come li ha ricevuti da myPLotWin->GiveFourData().
   * Nei dati passati però mancano le opzioni opt.
*/
    bool err=false;
    myData=data_;
    QSettings settings;
    settings.beginGroup("fourWin");
    myData.opt.amplUnit=EAmplUnit(settings.value("amplUnit",0).toInt());  //EAmplUnit=peak;
    myData.opt.amplSize=EAmplSize(settings.value("amplSize",1).toInt());  //EAmplSize=seventy;
    myData.opt.harm1=settings.value("harm1",DEFAULTHARM1).toInt();
    myData.opt.harm2=settings.value("harm2",DEFAULTHARM2).toInt();
    if(myData.opt.harm2<myData.opt.harm1+2){
        myData.opt.harm2=DEFAULTHARM1;
        err=true;
    }
    if(myData.opt.harm2<myData.opt.harm1+2){
        myData.opt.harm2=DEFAULTHARM2;
        err=true;
    }
    if(err)
        QMessageBox::warning(this, "CFourWin","Wrong harm1 or harm2 in stored settings.\n"
           "Using default values");
    settings.endGroup();
}

bool CFourWin::indexesFromTimes(SFourData data){
/* Funzione che calcola gli indici delle variabili corrisponsenti agli istanti del calcolo
 * della DFT specificati dall'utente o determinati automaticamente.
 * A partire dal 2020 la DFT è calcolata con un algoritmo che consente di avere campioni
 * non equispaziati. Pertanto anche il calcolo degli indici deve considerare questa
 *  possibilità.
 *
 * il calcolo prende come campioni quelli compresi, estremi inclusi, fra quello
 * immediatamente oltre t1, e quello immediatamente oltre t2 se esiste, altrimenti
 * l'ultimo. Gli estremi inclusi sono caratterizzati da indexLeft e indexRight.
 * Questa scelta dipende dal fatto che l'implicita periodicità assunta per il calcolo
 * dei polinomi di Fourier fa sì che se il calcolo è fra t1 e t2, il valore in t2 è
 * implicitamente assunto pari a quello in t1.
 *
 * Il calcolo degli indici di DFT dai tempi viene fatto sia al FormShow che dopo il click
 *  sulle opzioni di Fourier.
 * Siccome viene fatto in due punti indipendenti lo incapsulo qui, per essere sicuro che
 *  l'algoritmo sia sempre esattamente lo stesso.
 * Non lo metto all'interno di performDFT() in quanto quando effettuo questo calcolo dopo
 * il click sul bottone Opzioni di Fourier, prima di fare il performDFT(), che può essere
 *  un'operazione lenta, verifico che con le opzioni non siano soltanto cambiate delle
 *  opzioni di visualizzazione (e quindi il calcolo non va rifatto).
 * Questo check comporta anche la verifica se gli indici di calcolo sono cambiati oppure
 *  no.
*/
  bool changed=false;
  int oldIndexLeft=indexLeft, oldIndexRight=indexRight;
  int nearInt(float);
   //Gli indici indexLeft e indexRight definiscono il più ampio set di campioni **interni** a t1 e t2. Per il calcolo, come specificato sopra, il valore della funzione in t1 verrà calcolato con intepolazione lineare fra quello in indexLeft-1 e in indexLeft, mentre quello di destra sarà in indexRight. Se indexLeft=0, ovviamente, prenderò come primo campione proprio quello indexLeft.
  bool indexLeftDefined=false;
  indexLeft=0;
  indexRight=data.numOfPoints-1;
  for(int sample=1; sample<data.numOfPoints; sample++){
    if(data.x[sample]>data.opt.initialTime && !indexLeftDefined ){
        indexLeft=sample;
        indexLeftDefined=true;
    }
    if(data.x[sample]>data.opt.finalTime){
        indexRight=sample-1;
        break;
    }
  }

  // NOTA: indexLeft viene sempre >0. Questo è sfruttato nel calcolo di period in performNuDFT()

  //Vecchio calcolo valido per campioni equispaziati:
  /*
  int i, stepsPerSecond;
  stepsPerSecond=int((data.numOfPoints-1) / (data.x[data.numOfPoints-1]-data.x[0]));
  i= nearInt( (data.opt.initialTime-data.x[0])*stepsPerSecond)+1;

  if(i!=indexLeft){
      indexLeft=i;
      changed=true;
  }
  i=(data.numOfPoints-1) - nearInt( (data.x[data.numOfPoints-1]-data.opt.finalTime)*stepsPerSecond );
  if(i!=indexRight){
      indexRight=i;
      changed=true;
  }
  */
  if(indexLeft!=oldIndexLeft)
    changed=true;
  if(indexRight!=oldIndexRight)
    changed=true;
  fourOptions->getHMax((indexRight-indexLeft+1)/2);
  return changed;
}


void CFourWin::on_copyBtn_clicked()
{
    copyOrPrint(otCopy);
}

void CFourWin::on_infoBtn_clicked() {
    QString msg="File: "+myData.fileName+"\n"+"Variable: "+myData.varName+"\n";
    msg=msg+infoString;
    QMessageBox::information(this,"Fourier chart info",msg);
}


void CFourWin::on_optionsBtn_clicked(){
    int i;
    struct SFourOptions newOpts;

    optionsSetOk=true;
    fourOptions->getData(myData);
    i=fourOptions->exec();
    if(i==QDialog::Rejected)return;
    newOpts=fourOptions->giveData();

    //diagnostica:
    if(newOpts.initialTime < myData.x[0]){
       QMessageBox::information(this,"CFourWin","invalid initial time");
       optionsSetOk=false;
       return;
    }
    if(newOpts.initialTime >= newOpts.finalTime){
       QMessageBox::information(this,"CFourWin","initial time must be less than than final time");
       optionsSetOk=false;
       return;
    }
    // Devo verificare che l'istante finale richiesto non superi il massimo campione.
    if(newOpts.finalTime > myData.x[myData.numOfPoints-1]){
      //Può accadere che il valore richiesto superi di pochissimo l'ultimo campione, e la differenza non sia visibile nelle stringhe visualizzate. In questo caso considero i due numeri uguali
       QString str0, str;
       str0.setNum(newOpts.finalTime);
       str.setNum( myData.x[myData.numOfPoints-1]);
       if(str!=str0)
         QMessageBox::information(this,"CFourWin","Chosen t1 beyond final time.\n"
          "Selected final time, t2="+str);
       newOpts.finalTime = myData.x[myData.numOfPoints-1];
    }
    bool changed=false;
    if(newOpts.harm1!=myData.opt.harm1 ||newOpts.harm2!=myData.opt.harm2)
        changed=true;
    myData.opt=newOpts;
    //Il seguente changed fa una verifica se è necessario ricalcolare una DFT, per evitare di rifare un lento calcolo inutile
    changed=changed||indexesFromTimes(myData);
    analyseAndShow(changed);
}



int CFourWin::performDFT(){
/* Questa function è quasi uno script per fare l'analisi DFT.
 *Significato di alcune variabili:
 *  *ampl,     //Ampiezze delle armoniche calcolate
 *  *phases,   //Fasi delle armoniche calcolate
 *  ampl01[2], //Solo le ampiezze di armonica 0 e 1 per fare l'eventuale p.u.
*/
  int ret=0;
  int harm, sample, nSamples=indexRight-indexLeft+1;
  int harm1=myData.opt.harm1, harm2=myData.opt.harm2;
  const double pi=3.14159265358979;
  float *y1=myData.y+indexLeft;
  double  aux2, dft0;
  dcmplx dft, j(0,1), auxC, auxC1;
  double phase(dcmplx x);
  QGuiApplication::setOverrideCursor(Qt::WaitCursor);
  if(harm2<2){
      QMessageBox::critical(this,"CFourWin","Maximum harmonic order must be at least 2");
      return 1;
  }
  delete[] harmOrders;
  harmOrders=new float[harm2+1];
  for (int i=0; i<=harm2; i++)
    harmOrders[i]=float(i);
  delete[] ampl;  //Ampiezze delle armoniche prima della correzione con amplFactor
  delete[] amplitudes; //Ampiezze dopo la correzione con amplFactor (ad es. per trasformaz. in p.u.).
  delete[] phases;
  ampl=new float[harm2+1];
  amplitudes=new float[harm2+1];
  phases=new float[harm2+1];
   // Riga per debug:
  if(indexRight>=myData.numOfPoints){
    QMessageBox::critical(this,"CFourWin","Internal error \"myData.indexRight\" in CFourWin");
    QApplication::closeAllWindows();
  }
  auxC1=j*dcmplx(2.0*pi/nSamples);
  aux2=180./pi;
//  harmOrders[0]=0;
  dftDone=true;

  for(harm=harm1+(harm1==0); harm<=harm2; harm++){
    dft=0;
  /*  OLD algorithm
    auxC=auxC1*dcmplx(harm);
    for (sample=0; sample<nSamples; sample++){
      dft+=dcmplx(double(y1[sample]))*exp(-auxC*dcmplx(sample));
    }
    ampl[harm]=float(abs(dcmplx(2.0)*dft/dcmplx(nSamples)));
    // the following factor 2.0f has been checked with inverse-fourier in Matlab and Excel
    phases[harm]=2.f*float(aux2*atan2(real(dft),-imag(dft)));
*/

    auxC=auxC1*dcmplx(harm);
    //La seguente formula da https://en.wikipedia.org/wiki/Discrete_Fourier_transform; l'analoga formula di Matlab ha gli indici sfalsati di 1 in quanto la base degli indici per matlab è 1.
    for (sample=0; sample<nSamples; sample++){
      dft+=dcmplx(double(y1[sample]))*exp(-auxC*dcmplx(sample));
    }
//    ampl[harm]=float(abs(dcmplx(2.0)*dft/dcmplx(nSamples)));
    ampl[harm]=2.f*float(abs(dft))/float(nSamples);
    double phase(dcmplx x);
    phases[harm]= float(aux2*phase(dcmplx(-imag(dft),real(dft))));

//    harmOrders[harm]=harm;
  }
  //Computation of DC component. It is always computed because is used to express harmonics as ratio to it, and to evaluate signal RMS:
  dft0=0;
  for (sample=0; sample<nSamples; sample++)
      dft0+=double(y1[sample]);
  ampl01[0]=float(dft0)/nSamples;
  if(harm1==0){
    ampl[0]=float(dft0)/nSamples;
    phases[0]=0;
  }
  //Calcolo della componente di ordine 1 (va comunque calcolata per poter fare
   //l'eventuale p.u.):
  if(harm1<2){
     ampl01[1]=ampl[1];
  }else{
    dft=0;
    for (sample=0; sample<nSamples; sample++){
      dft+=dcmplx(double(y1[sample]))*exp(-auxC1*dcmplx(sample));
    }
    ampl01[1]=2*float(abs(dft))/nSamples;
  }
  //Calcolo RMS
  if(harm1==0)
    RMS=ampl01[0]*ampl01[0];
  for(harm=qMax(harm1,1); harm<=harm2; harm++){
    RMS+=ampl[harm]*ampl[harm]/2.f;
  }
  RMS=sqrtf(RMS);
  QApplication::restoreOverrideCursor();
  return ret;
}

int CFourWin::performNuDFT(){
/* Variante di performDFT che consente la DFT anche per campioni non equispaziati (non-
 * uniform-DFT)- Si ricorda che indexLeft e indexRight definiscono gli indici da
 * considerare per il calcolo ESTREMI INCLUSI, con la DFT equispaziata. Dettagli in
 * indexesFromTimes().
 * Per il calcolo della DFT non equispaziata la situazione è più complicata, in quanto
 * occorrerebbe fare gli integrali dei trapezi esattamente fra t1 e t2, mentre indexLeft
 * è a destra di t1. Per ora faccio una formulazione approssimata che in caso di spaziatura
 *  uniforme dà risultati uguali alla DFT uniforme ma può avere un piccolo errore sulla
 * non uniforme in quanto assume che esattamente in indexLeft-1 c'è un campione il cui
 * valore è pari a quello presente in t2.
 * Questa assunzione è implicita nel modo con cui è calcolato period.
 *
*/


    // NON VA PIU' HO FATTO DEL LAVORO PERCHé LA VERSIONE PUBBLICATA AVEVA ERRORI, SOPRATTUTTO NEL CALCOLO DEGLI INDICI SINISTRO E DESTRO.
  /*La versione attuale di performDFT() funziona perfettamente con  dati semplicissimi ti siompleDFT.xcv:
    "time","x"
     0,3.23205
     0.004,1.08418
     0.008,-0.489044
     0.012,0.686527
     0.016,2.98629
     0.02,3.23205

   ottenuta con OM, eseguendo fra 0 e 0.02, con Interval pari a 0.004:

   model tinyHarmo
     constant Real pi = Modelica.Constants.pi;
     Real x = 1.5 + 2 * cos(2 * pi * 50 * time + pi/6);
   end tinyHarmo;

    ==> e togliendo la riga finale dal file, in quanto OM alla fine metteva due campioni con lo stesso valore.
   Correttamente performDFT() non tiene in considerazioen il campione all'istante 0, replicato in quello all'istante 0.02 (in alternativa avremmo potuto scartare il campione all'istante finale)

    la performNuDFT() qui presente, più curata di quella pubblicata, in questo caso semplice non va.

    VERIFICARE IL CASO PROPOSTO DA Jovan Mrvic, e infine su casi più complessi.

    */

  int ret=0;
  int harm, nSamples=indexRight-indexLeft+1;
  int harm1=myData.opt.harm1, harm2=myData.opt.harm2;
  const float pi=3.14159265358979f;
  float *x1=myData.x+indexLeft-1;
  float *y1=myData.y+indexLeft-1;
  float period;
  float  ak, bk;

  if(indexLeft<1){
    QMessageBox::critical(this,"CFourWin","Internal error \"indexLeft\"");
    return 2;
  }
//  period=x1[nSamples]-x1[-1];
  period=myData.x[indexRight]-myData.x[indexLeft-1];

  QGuiApplication::setOverrideCursor(Qt::WaitCursor);
  if(harm2<2){
      QMessageBox::critical(this,"CFourWin","Maximum harmonic order must be at least 2");
      return 1;
  }
  delete[] harmOrders;
  harmOrders=new float[harm2+1];
  for (int i=0; i<=harm2; i++)
    harmOrders[i]=float(i);
  delete[] ampl;  //Ampiezze delle armoniche prima della correzione con amplFactor
  delete[] amplitudes; //Ampiezze dopo la correzione con amplFactor (ad es. per trasformaz. in p.u.).
  delete[] phases;
  ampl=new float[harm2+1];
  amplitudes=new float[harm2+1];
  phases=new float[harm2+1];
   // Riga per debug:
  if(indexRight>=myData.numOfPoints){
    QMessageBox::critical(this,"CFourWin","Internal error \"myData.indexRight\" in CFourWin");
    QApplication::closeAllWindows();
  }
//  harmOrders[0]=0;
  dftDone=true;

  for(harm=harm1; harm<=harm2; harm++){
    //La seguente formula è ricavata con la regola dei trapezi dalla definizione dei coefficienti del polinomio della scomposizione in serie di Fourier, fatta da me. La cosa interessante è che se viene applicata a campioni equispaziati fornisce, una volta che i coefficienti di seno e coseno sono combinati fra di loro attraverso i numeri complessi,  esattamente la formula  usata in performDFT(), di letteratura
    ak=0;
    bk=0;
    float Om=2*pi/period;
    int sample;
    for (sample=0; sample<nSamples; sample++){
      ak+=(y1[sample]*cosf(harm*Om*x1[sample])+y1[sample+1]*cosf(harm*Om*x1[sample+1]))*(x1[sample+1]-x1[sample]);
      bk+=(y1[sample]*sinf(harm*Om*x1[sample])+y1[sample+1]*sinf(harm*Om*x1[sample+1]))*(x1[sample+1]-x1[sample]);
//      float aux1=y1[sample]*cosf(harm*Om*x1[sample])+y1[sample+1]*cosf(harm*Om*x1[sample+1]);
//      float aux2=x1[sample+1]-x1[sample];
//      aux1=0;
    }

    if(harm==0)
      ampl[harm]=ak/period;  //verrà divisa per 0 più sotto
    else
      ampl[harm]=1.f/period*(sqrtf(ak*ak+bk*bk));
    phases[harm]= atan2f(ak,bk)*180.f/pi;
//    harmOrders[harm]=harm;
  }

  // Le componenti di ordine 0 e 1 vanno sempre calcolate per fare il p.u.
  if(harm1>0 || harm2<1)
    for(harm=0; harm<2; harm++){
      ak=0;
      bk=0;
      float Om=2*pi/period;
      int sample;
      for (sample=0; sample<nSamples; sample++){
        ak+=(y1[sample]*cosf(harm*Om*x1[sample])+y1[sample+1]*cosf(harm*Om*x1[sample+1]))*(x1[sample+1]-x1[sample]);
        bk+=(y1[sample]*sinf(harm*Om*x1[sample])+y1[sample+1]*sinf(harm*Om*x1[sample+1]))*(x1[sample+1]-x1[sample]);
      }
      if(harm==0)
        ampl[harm]=ak/period;
      else
        ampl[harm]=1.f/period*(sqrtf(ak*ak+bk*bk));
      phases[harm]= atan2f(ak,bk)*180.f/pi;
//      harmOrders[harm]=harm;
    }
  ampl[0]/=2.f;
  //La fase della componente 0 è indefinita e può venire qualunque numero Pertanto pongo:
  phases[0]=0;

/*
  //Calcolo della componente continua (va comunque calcolata per poter fare
  //l'eventuale p.u.):
  ak=0;
  for (int n=0; n<nSamples; n++)
    ak+=(y1[n]+y1[n+1])*(x1[n+1]-x1[n]);
  ak/=2.0f*period;
  ampl01[0]=ak;
  if(harm1==0){
    ampl[0]=ak;
    phases[0]=0;
  }
*/
  ampl01[0]=ampl[0];
  ampl01[1]=ampl[1];

  if(harm1==0)
    RMS=ampl01[0]*ampl01[0];
  for(harm=qMax(harm1,1); harm<=harm2; harm++){
    RMS+=ampl[harm]*ampl[harm]/2.f;
  }
  RMS=sqrtf(RMS);
  QApplication::restoreOverrideCursor();
  return ret;
}

//void CFourWin::resizeEvent(QResizeEvent *){
//}

void CFourWin::showEvent(QShowEvent *){
    struct SCurveParam curveParam;
    QSettings settings;
    settings.beginGroup("fourWin");
    ui->gridChkBox->setChecked(settings.value("useGrids",false).toBool());
    settings.endGroup();

    switch(myData.varName[0].toLatin1()){
        case 'v': curveParam.unitS[0]='V'; break;
        case 'c': case 'i': curveParam.unitS[0]='A'; break;
        case '1': curveParam.unitS[0]='A'; break;
        case 'a': curveParam.unitS[0]=char('^'); break;
        case 'p': curveParam.unitS[0]='W'; break;
        case 'e': curveParam.unitS[0]='J'; break;
        default: curveParam.unitS[0]=QChar(0);
    }
    ui->amplChart->setYZeroLine(true);
    ui->phaseChart->setYZeroLine(true);

   //Richiedo l'analisi completa, quindi con anche il calcolo di DFT (parametro passato changed=true). Coincide con l'analisi che si fa quando si cambiano le opzioni con il relativo bottone. Non comprende quindi solo le operazioni che non si modificano con il cambio di quelle opzioni come la definizione dell'unità di misura fatta qui sopra.
    analyseAndShow(true);
}

void CFourWin::valChangedAmp(SXYValues values, bool , bool ){
  ui->valuesBox->setTitle("Ampl. chart");
  ui->harmValLbl->setText(amplValueTxt);
  valChanged(values);
}
void CFourWin::valChangedPh(SXYValues values, bool , bool ){
  ui->valuesBox->setTitle("Phase chart");
  ui->harmValLbl->setText("<i>value (deg):</i><br>");
  valChanged(values);
}

void CFourWin::resetStateBtns(void){
    ui->saveSetBtn ->setVisible(true);
    ui->saveSetLbl->setVisible(false);
}


void CFourWin::updateChartOptions(SOptions opts){
  ui->amplChart->autoLabelXY=opts.autoLabelXY;
  ui->amplChart->useBrackets=opts.useBrackets;
  ui->phaseChart->autoLabelXY=opts.autoLabelXY;
  ui->phaseChart->useBrackets=opts.useBrackets;
}


void CFourWin::valChanged(SXYValues values){
  QString msg;

  // Se visualizzo solo alcune armoniche, ad es. dalla 5 alla 30, le armoniche precedenti non devono avere alcun punto e non devono emettere il relativo valore.
  //In effetti è stato visto che questo accade correttamente, salve che in prossimità dello 0 dove può arrivare un valore.
  // In attesa di comprendere in dettaglio cosa accade, una soluzione pratica e funzionante è di uscire se l'armonica visualizzata è inferiore alla minima visualizzabile
 if (values.X[0]<myData.opt.harm1-0.5f)
     return;

  msg=msg.setNum(int(values.X[0]+0.5f));

  ui->harmNLbl->setText("<i>harm n.:</i> "+msg);
//  msg=msg.setNum(values.Y[0][0],'g',5);
  msg=smartSetNum(values.Y[0][0],5);

  ui->harmValLbl->setText(ui->harmValLbl->text()+msg);

}


CFourWin::~CFourWin()
{
//    delete fourOutInfo;
    delete ui;
}

int nearInt(float f){
    int i=int(f);
    if(f-i>0.5f)
        i++;
    return i;
}


void CFourWin::on_printBtn_clicked()
{
    copyOrPrint(otPrint);
}


void CFourWin::on_gridChkBox_clicked()
{
    ui->phaseChart->yGrid=ui->gridChkBox->isChecked();
    ui->amplChart->yGrid=ui->gridChkBox->isChecked();
    ui->amplChart->plot();
    ui->amplChart->plot();
}

void CFourWin::on_saveSetBtn_clicked(){
   /* Tutti i settings da salvare, tranne hGrid si trovano all'interno di fourOptions.
    * Quindi me li faccio dire da lui e li salvo.
   */

    QSettings settings;
    settings.beginGroup("fourWin");
    settings.setValue("useGrids",ui->gridChkBox->isChecked());

    //Salvataggio dei settaggi presenti all'interno di fourOptions:
    fourOptions->getData(myData);
    SFourOptions options=fourOptions->giveData();
    settings.setValue("amplUnit",int(options.amplUnit));
    settings.setValue("amplSize",int(options.amplSize));
    settings.setValue("harm1",options.harm1);
    settings.setValue("harm2",options.harm2);
    settings.setValue("initialTime",options.initialTime);
    settings.setValue("finalTime",options.finalTime);
    settings.endGroup();

    ui->saveSetBtn ->setVisible(false);
    ui->saveSetLbl->setVisible(true);

    QTimer::singleShot(700, this, SLOT(resetStateBtns()));

}
double phase(dcmplx x) {
  double phase;
  const double pi=3.141592654;

  if(fabs(real(x))<1e-100){
    phase=pi/2.;
    if(imag(x)<0) phase+=pi;
  }else
   phase=atan(imag(x)/real(x));
  //atan ritorna una fase fra -pi/2 e pi/2; ma la fase è fra -180 e +180:
  if(real(x)<0)
    phase+=pi;
  if(phase>2*pi)
    phase-=2*pi;
  return phase;
}
