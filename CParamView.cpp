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

#include "CParamView.h"
#include <qelapsedtimer.h>
#include <qdebug.h>
#include "ui_CParamView.h"
#if defined(Q_OS_MAC)
   #define FACTORROWS 1.3
#else
   #define FACTORROWS 1.2
#endif

CParamView::CParamView(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CParamView)
{
    ui->setupUi(this);
    ui->tableWidget->setStyleSheet("QHeaderView::section { background-color:lightGrey }");
    QStringList labels;
    labels.append("Name");
    labels.append("Value");
    labels.append("Unit");
    ui->tableWidget->setHorizontalHeaderLabels(labels);
    matrixHidden=false;
    tableFilled=false;
    dataGot=false;
}

CParamView::~CParamView()
{
    delete ui;
}

void CParamView::getData(QStringList names_, QList <float> values_, QStringList units_, QStringList  descriptions_){
    /*  Questa semplice funzione è usata per passare i dati a ParamView, senza costruire la tabella.
     * Questa scelta è dovuta alle seguenti consideraiozni:
     * 1) i dati che vegono  passati sono disponibili nella function di DataSelWin
     *    loadFile, e quindi al momnento del caricamento del file li devo passare
     * 2) non faccio però in quella ooccasione la costruzione della tabella dei parametri
     *    in quanto essa può essere molto onerosa. Nel caso di EngineV6 dura quadi 3 secondi
     *    più del caricamento dell'intero file di dati in memoria. E' quindi opportuno
     *    fare la costruzione al momento che la tabella parametri è visualizzaa la prima
     *    volta. In tal modo si evita l'overhead della costuzione della tabella nei casi
     *    in cui si vogliono solo fare grafici senza guardare i parametri
    */
    descriptions=descriptions_;
    names=names_;
    units=units_;
    values=values_;
    dataGot=true;
}

void CParamView::fillTable(){
    /* Questa funzione riemmpe i dati della tabella parametri. Deve essere richiamata
     * dopo che le liste sono già state passate tramite getData. La giustificazione di
     * questa divisione fra le funzioni è riportanta nel commento alla function getData.
     * La verifica dela sequenza giusta viene fatta attraverso il check della variabile
     * booleana dataGot.
     */

    //L'uscita al seguente if si verifica solo in caso di errore diprogrammazione. Non è quindi
    // del tutto indispensabile prevedere unmessaggio di errore. Emetto solounmessaggio dDebug();
    if(!dataGot){
       qDebug()<<"Error: requested fillTable when data were not got";
        return;
    }

  ui->tableWidget->clearContents();
  ui->tableWidget->setRowCount(names.count());

#ifdef Q_OS_MAC
  QFont myFont=QFont("arial",11);
#else
  QFont myFont=QFont("arial",9);
#endif

  int itemIdx=0;
  bool areAnyMatrix=false;
  for (int i=0; i<names.count(); i++){
    QTableWidgetItem *item0 = new QTableWidgetItem();
    QTableWidgetItem *item1 = new QTableWidgetItem();
    QTableWidgetItem *item2 = new QTableWidgetItem();
    item0->setFont(myFont);
    item1->setFont(myFont);
    item2->setFont(myFont);
    item0->setText(names[i]);
    if(!areAnyMatrix)
      if(names[i].contains(']'))
        areAnyMatrix=true;

    item0->setToolTip(descriptions[i]);
    item2->setText(units[i]);
    QString str;
    str.setNum(values[i]);
    item1->setText(str);
    ui->tableWidget->setItem(itemIdx,0,item0);
    ui->tableWidget->setItem(itemIdx,1,item1);
    ui->tableWidget->setItem(itemIdx,2,item2);
    itemIdx++;

    QFontMetrics fontMetrics(myFont);
    int myHeight=int(FACTORROWS*fontMetrics.height());
    ui->tableWidget->horizontalHeader()->setFixedHeight(int(1.3*myHeight));
    for(int i=0; i<ui->tableWidget->rowCount(); i++){
       ui->tableWidget->setRowHeight(i,myHeight);
    }
  }
  ui->tableWidget->resizeColumnsToContents();
  int c0=ui->tableWidget->columnWidth(0);
  int c1=ui->tableWidget->columnWidth(1);
  int c2=ui->tableWidget->columnWidth(2);
  int sw= ui->tableWidget->verticalScrollBar()->width();
  setMaximumWidth(c0+c1+c2+int(1.5*sw));
  setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
  QString num;
  num.setNum(itemIdx);
  for(int i=itemIdx; i<ui->tableWidget->rowCount(); i++)
      ui->tableWidget->hideRow(i);
  setWindowTitle(num+" parameters");
  if(areAnyMatrix)
    ui->matrixBtn->setEnabled(true);
  else
    ui->matrixBtn->setEnabled(false);

  tableFilled=true;
}


void CParamView::rearrangeTable(bool excludeMatrices){
   /* Questa funzione include o esclude la visualizzazione delle righe contenenti elementi
    * di matrici. Non deve generare tutte le volte glil item che sono già stati generati
    * in fillTable.
*/
  int itemIdx=0;

  if(!tableFilled)
    return;

  for (int i=0; i<names.count(); i++){
    QString strDbg=names[i];
    if(names[i].contains("[")&& excludeMatrices)
       continue;
    QString str;
    ui->tableWidget->item(itemIdx,0)->setText(names[i]);
    ui->tableWidget->item(itemIdx,0)->setToolTip(descriptions[i]);
    str.setNum(values[i]);
    ui->tableWidget->item(itemIdx,1)->setText(str);
    ui->tableWidget->item(itemIdx,2)->setText(units[i]);
    itemIdx++;
  }
  ui->tableWidget->resizeColumnsToContents();
  int c0=ui->tableWidget->columnWidth(0);
  int c1=ui->tableWidget->columnWidth(1);
  int c2=ui->tableWidget->columnWidth(2);
  int sw= ui->tableWidget->verticalScrollBar()->width();
  setMaximumWidth(c0+c1+c2+int(1.5*sw));
  setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
  QString num;
  num.setNum(itemIdx);
  for(int i=itemIdx; i<ui->tableWidget->rowCount(); i++)
      ui->tableWidget->hideRow(i);
  setWindowTitle(num+" parameters");
}




void CParamView::resizeEvent(QResizeEvent *){
    /*Dopo che ho soddisfatto il requisito minimo, se le colonne sono più strette delle tabelle le allargo fino a raggiungere la piena larghezza.
    Faccio la seguente logica:
    - se ho spazio a sufficienza divido le due colonne in maniera unfiforme
    - se non ho spazio a sufficienza per dividere le due colonne in maniera uniforme ma la somma delle colonne sta nello spazio a disposizione sistemo le due colonne in maniera da avere la lolonna più larga priva di spazi bianchi
    - se non ho spazio a sufficienza non faccio nulla
    */

    /*
    ui->tableWidget->resizeColumnsToContents();
    bool widthIsEnough=false;
    bool widthIsEnoughForUniformCols=false;
    int cw[2];
    int tableWidth=ui->tableWidget->geometry().width();
    int largestColIdx=0, otherColIdx=1;

    cw[0]=ui->tableWidget->columnWidth(0);
    cw[1]=ui->tableWidget->columnWidth(1);
    if(cw[1]>cw[0]){
      largestColIdx=1;
      otherColIdx=0;
    }

    if(cw[0]+cw[1]<=tableWidth)
        widthIsEnough=true;
    if(cw[0]<tableWidth/2 && cw[1]<tableWidth/2)
        widthIsEnoughForUniformCols=true;

    if(!widthIsEnough)
        return;
    if(widthIsEnoughForUniformCols){ //colonne uniformi
      int vrWidth=ui->tableWidget->verticalHeader()->geometry().width();

      ui->tableWidget->setColumnWidth(0,(tableWidth-vrWidth-2 -ui->tableWidget->verticalHeader()->frameWidth())/2);
      ui->tableWidget->setColumnWidth(1,(tableWidth-vrWidth-2-ui->tableWidget->verticalHeader()->frameWidth())/2);
    } else{  //colonne disuniformi
        ui->tableWidget->setColumnWidth(largestColIdx,tableWidth-cw[otherColIdx]);
    }
*/
}

void CParamView::showEvent(QShowEvent *){

}


void CParamView::on_matrixBtn_clicked()
{

    QElapsedTimer timer;
    timer.start();

    if (matrixHidden){
      showMatrices();
      ui->matrixBtn->setText("Hide matrix values");
    }else{
      hideMatrices();
      ui->matrixBtn->setText("Show matrix values");
     }
    qDebug() << "The operation took" << timer.elapsed() << "milliseconds";
}

void CParamView::showMatrices(){
    if(!matrixHidden)
        return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    rearrangeTable(false);
    QApplication::restoreOverrideCursor();
   matrixHidden=false;
}

void CParamView::hideMatrices(){
   /*Elimino tuutte le righe contenenti matrici che appesantiscono la lettura*/
    if(matrixHidden)
        return;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    rearrangeTable(true);
    QApplication::restoreOverrideCursor();
    matrixHidden=true;
}

