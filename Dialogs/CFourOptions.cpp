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
#define NearInt(x) (int(x+0.5f))

#include "CFourOptions.h"
#include "ui_CFourOptions.h"

CFourOptions::CFourOptions(QWidget *parent) :
    QDialog(parent),    ui(new Ui::CFourOptions){
    ui->setupUi(this);
    dataGot=false;
}

void CFourOptions::getData(SFourData data_){
    QString msg;
    data=data_;
      msg.setNum(data_.opt.initialTime);
      ui->startTimeEdit->setText(msg);
      msg.setNum(data_.opt.finalTime);
      ui->endTimeEdit->setText(msg);

    switch(data_.opt.amplUnit){
      case  peak: ui->  peakBtn->setChecked(true); break;
      case   rms: ui->   rmsBtn->setChecked(true); break;
      case puOf0: ui->rmsTo0Btn->setChecked(true); break;
      case puOf1: ui->rmsTo1Btn->setChecked(true); break;
    }
    switch(data_.opt.amplSize){
      case   fifty: ui-> size50Btn->setChecked(true); break;
      case seventy: ui-> size70Btn->setChecked(true); break;
      case hundred: ui->size100Btn->setChecked(true); break;
    }
    msg.setNum(data_.opt.harm1);
    ui->harm1Edit->setText(msg);
    msg.setNum(data_.opt.harm2);
    ui->harm2Edit->setText(msg);
    dataGot=true;
}

void CFourOptions::getHMax(int hMax_){
  QString msg;
  msg.setNum(hMax_);
  ui->hMaxLbl->setText("max: "+msg);

}

SFourOptions CFourOptions::giveData(void){
    SFourOptions opt;
    opt.initialTime= ui->startTimeEdit->text().toFloat();
    opt.finalTime=   ui->  endTimeEdit->text().toFloat();
    if(ui->  peakBtn->isChecked()) opt.amplUnit=peak;
    if(ui->   rmsBtn->isChecked()) opt.amplUnit=rms;
    if(ui->rmsTo0Btn->isChecked()) opt.amplUnit=puOf0;
    if(ui->rmsTo1Btn->isChecked()) opt.amplUnit=puOf1;

    if(ui-> size50Btn->isChecked()) opt.amplSize=fifty;
    if(ui-> size70Btn->isChecked()) opt.amplSize=seventy;
    if(ui->size100Btn->isChecked()) opt.amplSize=hundred;
    opt.harm1= ui->harm1Edit->text().toInt();
    opt.harm2= ui->harm2Edit->text().toInt();
    opt.initialTime = ui->startTimeEdit->text().toFloat();
    opt.finalTime  = ui->endTimeEdit->text().toFloat();
    return opt;
}

void CFourOptions::updatehMax(SFourData data){
  // Funzione PURA
  int stepsPerSecond, indexLeft, indexRight;
  int nearInt(float);
  stepsPerSecond=(data.numOfPoints-1) / (data.x[data.numOfPoints-1]-data.x[0]);
  indexLeft= NearInt( (data.opt.initialTime-data.x[0])*stepsPerSecond)+1;
  indexRight=(data.numOfPoints-1) - NearInt( (data.x[data.numOfPoints-1]-
                                    data.opt.finalTime)*stepsPerSecond );
  QString msg;
   msg.setNum((indexRight-indexLeft+1)/2);
   ui->hMaxLbl->setText("hmax: "+msg);
}


CFourOptions::~CFourOptions() {
    delete ui;
}

void CFourOptions::on_startTimeEdit_editingFinished(){
  if(!dataGot)
    return;
  QString msg=ui->startTimeEdit->text();
  data.opt.initialTime=msg.toFloat();
  updatehMax(data);
}

void CFourOptions::on_endTimeEdit_editingFinished(){
   if(!dataGot)
     return;
    QString msg=ui->endTimeEdit->text();
    data.opt.finalTime=msg.toFloat();
    updatehMax(data);
}
