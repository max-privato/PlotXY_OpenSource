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

#include "CUnitsDlg.h"
#include "ui_CUnitsDlg.h"

CUnitsDlg::CUnitsDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CLabelsDlg){
    ui->setupUi(this);
    useSmartUnits=true;
}

CUnitsDlg::~CUnitsDlg()
{
    delete ui;
}

void CUnitsDlg::on_bracketsChkBox_clicked(bool checked){
  useBrackets=checked;
}

void CUnitsDlg::getTwinScales(bool isTwinScale_){
    ui->ryUnitEdit->setEnabled(isTwinScale_);
}

void CUnitsDlg::setUseBrackets(bool useBrackets_){
    useBrackets=useBrackets_;
    ui->bracketsChkBox->setChecked(useBrackets);
}

void CUnitsDlg::setXUnit(QString xUnit_){
    xUnit=xUnit_;
    ui->xUnitEdit->setText(xUnit_);
}


void CUnitsDlg::on_xUnitEdit_editingFinished(){
    xUnit=ui->xUnitEdit->text();
}
void CUnitsDlg::on_ryUnitEdit_editingFinished(){
    ryUnit=ui->ryUnitEdit->text();
}
void CUnitsDlg::on_yUnitEdit_editingFinished(){
    yUnit=ui->yUnitEdit->text();
}



void CUnitsDlg::on_buttonBox_clicked(QAbstractButton *button){
  if(button->text()=="Reset"){
    doReset=true;
    ui->xUnitEdit->setText("");
    ui->yUnitEdit->setText("");
    ui->ryUnitEdit->setText("");
  }
}


void CUnitsDlg::showEvent(QShowEvent *){
    doReset=false;
}


void CUnitsDlg::on_buttonBox_accepted()
{
    useSmartUnits=ui->smartUnitsCB->isChecked();
}
