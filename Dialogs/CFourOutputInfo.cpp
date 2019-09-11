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

#include "CFourOutputInfo.h"
#include "ui_CFourOutputInfo.h"

CFourOutputInfo::CFourOutputInfo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CFourOutputInfo)
{
    ui->setupUi(this);
}


void CFourOutputInfo::setPhaseChart(bool active){
  ui->phaseChkBox->setChecked(active);
}

void CFourOutputInfo::showEvent(QShowEvent *){
    accepted=false;
}


void CFourOutputInfo::setType(EOutType type){
  if(type==otCopy){
    ui->pdfCBox->setVisible(false);
  }else{ //type=otPrint
      ui->pdfCBox->setVisible(true);
  }
}

CFourOutputInfo::~CFourOutputInfo()
{
    delete ui;
}

void CFourOutputInfo::on_buttonBox_accepted()
{
    accepted=true;
    numData=ui->numDataChkBox->isChecked();
    phaseChart=ui->phaseChkBox->isChecked();
    amplChart=ui->amplChkBox->isChecked();
    pdfOutput=ui->pdfCBox->isChecked();
    close();
}

void CFourOutputInfo::on_buttonBox_rejected()
{
    close();
}


void CFourOutputInfo::on_numDataChkBox_clicked(bool checked)
{
    ui->amplChkBox->setChecked(!checked);
    ui->phaseChkBox->setChecked(!checked);

}

void CFourOutputInfo::on_amplChkBox_clicked(bool checked)
{
 if(checked) // Se seleziono il grafico non devo stampare i dati numerici
   ui->numDataChkBox->setChecked(false);

}

void CFourOutputInfo::on_phaseChkBox_clicked(bool checked)
{
  if(checked) // Se seleziono il grafico non devo stampare i dati numerici
    ui->numDataChkBox->setChecked(false);

}
