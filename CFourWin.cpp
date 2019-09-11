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
Essa è richiamata sia allo show della finestre (in quel caso changed è true), che dopo il click sul bottone delle opzioni (in quel caso changed=false mi evita di rifare taluni calcoli inutili).*/
    int ret=0;
    int harm, harm1, harm2;
    float amplFactor;
    static float *phases1, *amplitudes1;
    QString msg;
    msg.setNum(myData.opt.initialTime,'g',5);
    ui->iniTimeLbl->setText("t1: "+msg);

    msg.setNum(myData.opt.finalTime,'g',5);
    ui->finTimeLbl->setText("t2: "+msg);

    //Eventuale ricalcolo.
    if(changed){
        indexesFromTimes(myData);
        ret=performDFT();
        if (ret>0)
          return ret;
    }
    //Eventuale correzione con amplFactor
    harm1=myData.opt.harm1;
    harm2=myData.opt.harm2;
    amplFactor=1.0; //evita un warning in quanto nel case succcessivo non ho la voce default
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
        amplFactor=float((1.0/SQRT2)/fabs(double(ampl01[0])));
        amplValueTxt="<i>value (pu/h0):</i><br>";
        break;
       case puOf1:
        amplFactor=float(1./fabs(double(ampl01[1])));
        amplValueTxt="<i>value (pu/h1):</i><br>";
        break;
    }
    ui->harmValLbl->setText(amplValueTxt);

    amplitudes[0]=ampl[0];
    if(myData.opt.amplUnit==puOf0)
        amplitudes[0]=1.0;
    if(myData.opt.amplUnit== puOf1)
        amplitudes[0]=fabsf(ampl[0]/ampl[1]);
    for(harm=_max(1,harm1); harm<=harm2; harm++)
        amplitudes[harm]=ampl[harm]*amplFactor;

    //Aggiornamento grafici.
    amplitudes1=amplitudes+harm1;
    ui->amplChart->getData(harmOrders+harm1, &amplitudes1, harm2-harm1+1);
    phases1=phases+harm1;
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
    QString msg0, msg1;
    msg0.setNum(THD0,'g',4);
    msg1.setNum(THD1,'g',4);
    //La seguente thdString viene poi visualizzata alla pressione del bottone "?"
    thdString="";
    if(myData.opt.harm1==0)
       thdString= "THD0: "+msg0+"%; THD1: "+msg1+"%";
    if(myData.opt.harm1==1)
       thdString= "THD1: "+msg1+"%";
    return ret;
}


void CFourWin::computeTHD(){
  float THD=0;
  for(int harm=_max(myData.opt.harm1,2); harm<=myData.opt.harm2; harm++)
    THD+=amplitudes[harm]*amplitudes[harm];

  // THD % relative ad armoniche 0 e 1:
  if(amplitudes[0]==0)
      return;
  THD0=float(100*sqrt(double(THD))/fabs(double(amplitudes[0])));
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
  int imageWidth, imageHeight; //larghezza e altezza dell'immagine combinata contenente intestazione e grafici a barre (o grafico a barre)
  int yPosition;
  QClipboard *clipboard = QApplication::clipboard();
  QImage * amplImg=nullptr, *phImg=nullptr, *combinedImage;
  QPainter *painter;
  QString pdfFileName, fullFileName;
  QString dateStr=QDateTime::currentDateTime().date().toString();
  fourOutInfo->setPhaseChart(ui->phaseChart->isVisible());
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
  headText2+=QString("t2: %1").arg(double(myData.opt.finalTime),0,'g',5);

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
      // Calcolo lo spazio necessario al tracciamento di HeadText, e lo metto in boundRect
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
   painter->drawText(combinedImage->rect(),0,headText1);
   //le altre righe sono a spessore normale:
   font.setBold(false);
   painter->setFont(font);
   yPosition=headRect1.height();
   rect=combinedImage->rect();
   rect.moveTop(yPosition);
   painter->drawText(rect,0,headText2);

   if(fourOutInfo->amplChart){
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

  /*         pdfFileName= QInputDialog::getText(this, "pdf File Name",
               "File name (without \".pdf\"):", QLineEdit::Normal,"pdfFile" , &ok);
           if (ok && !pdfFileName.isEmpty()){
  */
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
       }
   }
   delete painter;
   MB.exec();

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
/* Il calcolo degli indici di DFT dai tempi viene fatto sia al FormShow che dopo il click sulle opzioni di Fourier.
Siccome viene fatto in due punti indipendenti lo incapsulo qui, per essere sicuro che l'algoritmo sia sempre esattamente lo stesso.
Non lo metto all'interno di performDFT() in quanto quando effettuo questo calcolo dopo il click sul bottone Opzioni di Fourier, prima di fare il performDFT(), che può essere un'operazione lenta, verifico che con le opzioni non siano soltanto cambiate delle opzioni di visualizzazione (e quindi il calcolo non va rifatto). Questo check comporta anche la verifica se gli indici di calcolo sono cambiati oppure no.
*/
  bool changed=false;
  int i, stepsPerSecond;
  int nearInt(float);
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
  fourOptions->getHMax((indexRight-indexLeft+1)/2);
  return changed;
}


void CFourWin::on_copyBtn_clicked()
{

    copyOrPrint(otCopy);
}

void CFourWin::on_infoBtn_clicked()
{
    QString msg="File: "+myData.fileName+"\n"+"Variable: "+myData.varName+"\n";
    msg=msg+thdString;
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
    //Il seguente changed va una verifica se è necessario ricalcolare una DFT, per evitare di rifare un lento calcolo inutile
    changed=changed||indexesFromTimes(myData);
    analyseAndShow(changed);
}



int CFourWin::performDFT(){
/* Questa function è quasi uno script per fare l'analisi DFT.
Significato di alcune variabili:
 *ampl,     //Ampiezze delle armoniche calcolate
 *phases,   //Fasi delle armoniche calcolate
 ampl01[2], //Solo le ampiezze di armonica 0 e 1 per fare l'eventuale p.u.
*/
  int ret=0;
  int harm, sample, nSamples=indexRight-indexLeft;
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
  delete[] ampl;  //Ampiezze delle armoniche prima della correzione con amplFactor
  delete[] amplitudes; //Ampiezze dopo la correzione con amplFactor (ad es. per trasformaz. in p.u.).
  delete[] phases;
  harmOrders=new float[harm2+1];
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
  harmOrders[0]=0;
  dftDone=true;

  for(harm=harm1+(harm1==0); harm<=harm2; harm++){
    dft=0;
    auxC=auxC1*dcmplx(harm);
    for (sample=0; sample<nSamples; sample++){
      dft+=dcmplx(double(y1[sample]))*exp(-auxC*dcmplx(sample));
    }
    ampl[harm]=float(abs(dcmplx(2.0)*dft/dcmplx(nSamples)));
    double phase(dcmplx x);
    phases[harm]= float(aux2*phase(dcmplx(-imag(dft),real(dft))));

    harmOrders[harm]=harm;
  }
  //Calcolo della componente continua (va comunque calcolata per poter fare
  //l'eventuale p.u.):
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
  QApplication::restoreOverrideCursor();
  return ret;
}

void CFourWin::resizeEvent(QResizeEvent *){
}

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
        default: curveParam.unitS[0]=0;
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
