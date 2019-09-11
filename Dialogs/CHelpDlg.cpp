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

#include "CHelpDlg.h"
#include "ui_CHelpDlg.h"
#include <QScreen>

CHelpDlg::CHelpDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CHelpDlg)
{
    ui->setupUi(this);
    QScreen *screen=QGuiApplication::primaryScreen();
    int myDPI=int(screen->logicalDotsPerInch());
    //Se sono su schermi 4k aumento la dimensione della finestra rispetto a quanto progettato in Designer, ma non la raddoppio; In futuro metterò un'opzione che consentirà all'utente di scegliere fra finestre medie, piccole o grandi. Quella che faccio qui è piccola
    if(myDPI>100){
      resize(myDPI/96.0f*geometry().width(),myDPI/96.0f*geometry().height());
    }

}

CHelpDlg::~CHelpDlg()
{
    delete ui;
}
