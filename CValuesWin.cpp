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
