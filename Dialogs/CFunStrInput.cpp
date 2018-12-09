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

#include "CFunStrInput.h"
#include <QScreen>
#include "ui_CFunStrInput.h"

CFunStrInput::CFunStrInput(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CFunStrInput){
    ui->setupUi(this);
    QScreen *screen=QGuiApplication::primaryScreen();
    int myDPI=screen->logicalDotsPerInch();
    //Se sono su schermi 4k aumento la dimensione della finestra rispetto a quanto progettato in Designer, ma non la raddoppio; In futuro metterò un'opzione che consentirà all'utente di scegliere fra finestre medie, piccole o grandi. Quella che faccio qui è piccola
    if(myDPI>100){
      resize(1.5*geometry().width(),1.5*geometry().height());
      setMinimumHeight(1.5*minimumHeight());
    }
}

QString CFunStrInput::giveStr(){
  ui->lineEdit->selectAll();
  return ui->lineEdit->text();
}

void CFunStrInput::getStr(QString str){
    // Questa funzione serve quando sto caricando lo stato: se carico una funzione di variabile  la metto anche qui dentro in modo da facilitare le successive modificazioni
    ui->lineEdit->setText(str);
    ui->lineEdit->selectAll();
}

CFunStrInput::~CFunStrInput()
{
    delete ui;
}
