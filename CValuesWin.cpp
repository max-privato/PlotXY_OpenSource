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

#include "CValuesWin.h"
#include <QScreen>
#include "ui_CValuesWin.h"

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



CValuesWin::CValuesWin(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CValuesWin){
    ui->setupUi(this);
    aVarHPos=8;
    fileHPos=3;
    setWindowFlags(Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::Tool);

    //Definisco il font in maniera DPI-aware

    //rendo il minimumSize DPI-aware
    QSize minSize=minimumSize();
    QScreen *screen=QGuiApplication::primaryScreen();
    int myDPI=int(screen->logicalDotsPerInch());

    if(myDPI>100){
       myFont=QFont("Arial",10);
      setMinimumHeight(int(2.0f*minSize.height()));
      setMinimumWidth(int(1.7f*minSize.width()));
    } else{
        myFont=QFont("Arial",10);
    }
#if defined(Q_OS_MAC)
    myFont=QFont("Arial",12);
#endif

}


void  CValuesWin::paintEvent(QPaintEvent *){
    int i, iFile, iVarTot=0;
    QBrush brush;
    QPen pen;
    QPainter myPainter(this);

    //Sbianco il fondo:
    brush.setColor(Qt::white);
    brush.setStyle(Qt::SolidPattern);
    myPainter.setBrush(brush);
    myPainter.drawRect(0,0,width(),height()-ui->checkBox->height()-2);

    //Update nomi di file:
    myFont.setWeight(QFont::Bold);
    myPainter.setFont(myFont);
    for(iFile=0; iFile<nFiles; iFile++){
        myPainter.drawText(fileHPos,fileVPos[iFile], ""+fileText[iFile]);
    }
    myFont.setWeight(QFont::Normal);

    //Update variabili "x":
    myFont.setStyle(QFont::StyleItalic);
    myPainter.setFont(myFont);

    for(iFile=0; iFile<nFiles; iFile++){
        myPainter.drawText(aVarHPos,xVarVPos[iFile],xVarText[iFile]);
    }
    //Update variabili "y":
    myFont.setStyle(QFont::StyleNormal);
    for(iFile=0; iFile<nFiles; iFile++)
    for(i=0; i<nPlots[iFile]; i++){
        pen.setColor(myColors[iVarTot]);
        myPainter.setPen(pen);
        myPainter.drawText(aVarHPos,yVarVPos[iVarTot],yVarText[iVarTot]);
        iVarTot++;
    }
}

void CValuesWin::setInterpolation(bool interpolation){
    ui->checkBox->setChecked(interpolation);
}

void CValuesWin::setUp(int nFiles_, const QVector <int> &nPlots_, const QList <SCurveParam> &curveParam_, QList <SFileInfo> filesInfo){
/*Questa funzione va richiamata alla visualizzazione della finestra con i dati che non cambiano con il movimento del cursore dati.
  */
    int iFile, iVar, iTotPlot=0, nextY, yStep=fontMetrics().height();

    yStep=QFontMetrics(myFont).lineSpacing();
    //Fase 1: processamento nFiles e nVars
    nFiles=nFiles_;
    nPlots=nPlots_;
    nTotPlot=0;
    for(int iFile=0; iFile<nFiles; iFile++){
        nTotPlot+=nPlots[iFile];
    }

    //Fase 2: importo gli altri dati
    for(iFile=0; iFile<nFiles; iFile++){
        for(iVar=0; iVar<nPlots[iFile]; iVar++){
            myColors[iTotPlot]=curveParam_[iTotPlot].color;
            iTotPlot++;
        }
        fileText[iFile]=filesInfo[iFile].name+":";
    }

    //Fase 3: calcolo posizioni di scrittura
    iTotPlot=0;
    nextY=3+yStep;
    for(iFile=0; iFile<nFiles; iFile++){
        fileVPos[iFile]=nextY;
        nextY+=yStep+1; //Spazio leggermente di più fra il nome del file e la prima variabile in quanto il nome può avere, a differenza dei numeri, lettere "basse" come la 'p'.
        xVarVPos[iFile]=nextY;
        nextY+=yStep; //lascio spazio per la variabile x
        for(iVar=0; iVar<nPlots[iFile]; iVar++){
            yVarVPos[iTotPlot]=nextY;
            nextY+=yStep;
            iTotPlot++;
        }
        nextY+=2;
    }
    nextY+=4;
    setMaximumHeight(nextY+2);
    setGeometry(x(),y(),width(), nextY+2);
}



void CValuesWin::updateVarValues(float *x, float **y, bool diff, bool highPrecision){
  /* Function che aggiorna i numeri delle variabili x e y, e poi chiama paintEvent() che li scrive.
  */
  int i, iFile, iVarTot=0;

  //Valori delle variabili sull'asse x
  for(iFile=0; iFile<nFiles; iFile++){
//    xVarText[iFile].setNum(x[iFile],'g',5+highPrecision);
    xVarText[iFile]=smartSetNum(x[iFile],5+highPrecision);
    if(diff)
      xVarText[iFile] = "*" + xVarText[iFile];
  }
  //Ora i valori delle variabili sull'asse y
  for(iFile=0; iFile<nFiles; iFile++)
    for(i=0; i<nPlots[iFile]; i++){
//      yVarText[iVarTot].setNum(y[iFile][i],'g',5+highPrecision);
      yVarText[iVarTot]=smartSetNum(y[iFile][i],5+highPrecision);
      if(diff)
        yVarText[iVarTot] = "*" + yVarText[iVarTot];
      iVarTot++;
    }
  update();
}


CValuesWin::~CValuesWin()
{
    delete ui;
}

void CValuesWin::on_checkBox_clicked(bool checked)
{
    emit interpolationChanged(checked);
}
