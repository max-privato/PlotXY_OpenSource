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

#include "CProgOptions.h"
#include <QSettings>
#include <QMessageBox>

#include "ui_CProgOptions.h"

extern SGlobalVars GV; //definite in main(); struttura in "Globals.h"

CProgOptions::CProgOptions(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CProgOptions)
{
    ui->setupUi(this);

}

void CProgOptions::getData(struct SOptions PO){
    ui->autoUnitCkb->setChecked(PO.autoLabelXY);
    ui->commasCkb->setChecked(PO.commasAreSeparators);
    ui->clpbrdDlgCkb->setChecked(PO.useCopiedDialog);
    ui->CompactMMCkb->setChecked(PO.compactMMvarMenu);
    ui->largeFCkb->setChecked(PO.largerFonts);
    ui->onlyPointsCkb->setChecked(PO.onlyPoints);
    ui->trimCkb->setChecked((PO.trimQuotes));
    ui->useBarCkb->setChecked(PO.barChartForFS);
    ui->useBrakCkb->setChecked(PO.useBrackets);
    ui->useGridsCkb->setChecked(PO.useGrids);
    ui->useOldColorsCkb->setChecked(PO.useOldColors);
    ui->remWinCkb->setChecked(PO.rememberWinPosSize);

    if (PO.plotPenWidth==0)
        ui->widthThinBtn->setChecked(true);
    if (PO.plotPenWidth==1)
        ui->widthThickBtn->setChecked(true);
    if (PO.plotPenWidth==2)
        ui->widthAutoBtn->setChecked(true);

    //    ui->useThinLinesCkb->setChecked(PO.useThinLines);
    QString msg;
    msg.setNum(PO.defaultFreq);
    ui->freqEdit->setText(msg);
}

SOptions CProgOptions::giveData(){
    SOptions opt;
    opt.autoLabelXY         =ui->autoUnitCkb->isChecked();
    opt.barChartForFS       =ui->useBarCkb->isChecked();
    opt.commasAreSeparators =ui->commasCkb->isChecked();
    opt.useCopiedDialog     =ui->clpbrdDlgCkb->isChecked();
    opt.compactMMvarMenu    =ui->CompactMMCkb->isChecked();
    opt.largerFonts         =ui->largeFCkb->isChecked();
    opt.onlyPoints          =ui->onlyPointsCkb->isChecked();
    opt.rememberWinPosSize  =ui->remWinCkb->isChecked();
    opt.trimQuotes          =ui->trimCkb->isChecked();
    opt.useBrackets         =ui->useBrakCkb->isChecked();
    opt.useGrids            =ui->useGridsCkb->isChecked();
    opt.useOldColors        =ui->useOldColorsCkb->isChecked();
    opt.defaultFreq         =ui->freqEdit->text().toFloat();
    if (ui->widthThinBtn->isChecked())
        opt.plotPenWidth=0;
    if (ui->widthThickBtn->isChecked())
        opt.plotPenWidth=1;
    if (ui->widthAutoBtn->isChecked())
        opt.plotPenWidth=2;
    return opt;
}


CProgOptions::~CProgOptions()
{
    delete ui;
}

void CProgOptions::on_buttonBox_clicked(QAbstractButton *button){
  //Richiesto reset totale del registro e settaggio valori default del programma
  if((QPushButton *)button== ui->buttonBox->button(QDialogButtonBox::Reset) ){
    QSettings settings;
    settings.clear();
    GV.PO.autoLabelXY           =AUTOLABELXY;
    GV.PO.barChartForFS         =BARCHARTFORFS;
    GV.PO.commasAreSeparators   =COMMASARESEPARATORS;
    GV.PO.compactMMvarMenu      =COMPACTMMVARMENU;
    GV.PO.defaultFreq           =DEFAULTFREQ;
    GV.PO.largerFonts           =LARGERFONTS;
    GV.PO.useBrackets           =USEBRACKETS;
    GV.PO.useGrids              =USEGRIDS;
    GV.PO.useCopiedDialog       =USECOPIEDDIALOG;
    GV.PO.useOldColors          =USEOLDCOLORS;
    GV.PO.onlyPoints            =ONLYPOINTSINPLOTS;
    GV.PO.trimQuotes            =TRIMQUOTES;
    GV.PO.rememberWinPosSize    =REMEMBERWINPOSANDSIZE;

    GV.PO.plotPenWidth          =PLOTPENWIDTH;;
    getData(GV.PO);
    QMessageBox mb;
    mb.setText("Default settings restored.\nRegistry Cleared.");
    mb.exec();
    close();
  }
  if((QPushButton *)button== ui->buttonBox->button(QDialogButtonBox::Ok) ){
    SOptions PO;

    PO=giveData();
    GV.PO=PO;
    QSettings settings;
    settings.beginGroup("globalOptions");
    settings.setValue("autoLabelXY", GV.PO.autoLabelXY);
    settings.setValue("barChartForFS", GV.PO.barChartForFS);
    settings.setValue("compactMMvarMenu", GV.PO.compactMMvarMenu);
    settings.setValue("largerFonts", GV.PO.largerFonts);
    settings.setValue("useBrackets", GV.PO.useBrackets);
    settings.setValue("useGrids", GV.PO.useGrids);
    settings.setValue("useOldColors", GV.PO.useOldColors);
    settings.setValue("useCopiedDialog", GV.PO.useCopiedDialog);
    settings.setValue("onlyPoints", GV.PO.onlyPoints);
    settings.setValue("trimQuotes", GV.PO.trimQuotes);

    settings.setValue("plotPenWidth", GV.PO.plotPenWidth);

    settings.setValue("commasAreSeparators", GV.PO.commasAreSeparators);
    settings.setValue("rememberWinPosSize", GV.PO.rememberWinPosSize);

    settings.setValue("defaultFreq", GV.PO.defaultFreq);

    //do l'opportunità alle altre parti del programma di utilizzare subito i parametri cambiati:
    emit programOptionsChanged(GV.PO);
  }
}
