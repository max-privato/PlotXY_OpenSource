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

#include "CPlotOptions.h"
#include "ui_CPlotOptions.h"

CPlotOptions::CPlotOptions(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CPlotOptions)
{
    ui->setupUi(this);
    accepted=false;
    swSizeIsPixel=false;
}

CPlotOptions::~CPlotOptions()
{
    delete ui;
}

SPlotOptions CPlotOptions::giveData(){
    struct SPlotOptions opts;
    //Dati scheda General:
    if(ui->xLinearBtn->isChecked())opts.xScaleType=stLin;
    if(ui->xLogBtn->isChecked())   opts.xScaleType=stLog;
    if(ui->xDBBtn->isChecked())    opts.xScaleType=stDB;
    if(ui->yLinearBtn->isChecked())opts.yScaleType=stLin;
    if(ui->yLogBtn->isChecked())   opts.yScaleType=stLog;
    if(ui->yDBBtn->isChecked())    opts.yScaleType=stDB;
    opts.grids=ui->gridBox->isChecked();
    //Dati scheda Font size:
    opts.autoAxisFontSize=ui->autoAxisFontBtn->isChecked();
    opts.axisFontSize=ui->fontPointCombo->currentText().toInt();
    opts.autoLegendFontSize=ui->autoLgdFontBtn->isChecked();

    //Dati scheda Plot type:
    if(ui->linesTypeBtn->isChecked())  opts.plotType=ptLine;
    if(ui->dotsTypeBtn->isChecked())   opts.plotType=ptSwarm;
    if(ui->barsTypeBtn->isChecked())   opts.plotType=ptBar;
    if(ui->widthThinBtn->isChecked())  opts.penWidth=pwThin;
    if(ui->widthThickBtn->isChecked()) opts.penWidth=pwThick;
    if(ui->widthAutoBtn->isChecked())  opts.penWidth=pwAuto;
    return opts;
}



void CPlotOptions::on_linesTypeBtn_clicked()
{
    ui->pointTypeGroup->setEnabled(false);
    ui->lineWidthGroup->setEnabled(true);
}


void CPlotOptions::prepare(bool useGrids, EPlotType plotType, EScaleType xScaleType, EScaleType yScaleType, EPlotPenWidth penWidth) {
/* Il valore predefinito dello stato dei vari widget, alla prima apertura del dialog dopo la sua creazione, dovrà essere congruente con i default del programma.
Preferisco non usare la variabile GV in CPlotOptions, per evitare una programmazione poco robusta.
Passo quindi alla funzione le opzioni che contano per la creazione di CPlotOptions, fra quelle definite nelle opzioni globali del programma, o da esse deducibili:
- use grids
- plotType
- xScaleType
- yScaleType
- penWidth
*/
    ui->gridBox->setChecked(useGrids);
    if(plotType==ptLine)
      ui->linesTypeBtn->setChecked(true);
    else if (plotType==ptBar)
      ui->barsTypeBtn->setChecked(true);
    else if (plotType==ptSwarm)
      ui->dotsTypeBtn->setChecked(true);

    if(xScaleType==stLin)
      ui->xLinearBtn->setChecked(true);
    else if (xScaleType==stDB)
      ui->xDBBtn->setChecked(true);
    else if (xScaleType==stLog)
      ui->xLogBtn->setChecked(true);

    if(yScaleType==stLin)
      ui->yLinearBtn->setChecked(true);
    else if (yScaleType==stDB)
      ui->yDBBtn->setChecked(true);
    else if (yScaleType==stLog)
      ui->yLogBtn->setChecked(true);

    if(penWidth==pwAuto)
      ui->widthAutoBtn->setChecked(true);
    else if(penWidth==pwThick)
      ui->widthThickBtn->setChecked(true);
    else
      ui->widthThinBtn->setChecked(true);
}


void CPlotOptions::on_barsTypeBtn_clicked()
{
    ui->pointTypeGroup->setEnabled(false);
    ui->lineWidthGroup->setEnabled(false);
}


void CPlotOptions::on_buttonBox_accepted()
{
    accepted=true;
    close();
}

void CPlotOptions::on_buttonBox_rejected()
{
    accepted=false;
    close();
}


void CPlotOptions::on_dotsTypeBtn_clicked(bool checked)
{
  if(checked){
    ui->pointTypeGroup->setEnabled(true);
    ui->lineWidthGroup->setEnabled(false);
  }
}

void CPlotOptions::on_linesTypeBtn_clicked(bool checked)
{
  if(checked){
    ui->pointTypeGroup->setEnabled(false);
    ui->lineWidthGroup->setEnabled(true);
  }
}

void CPlotOptions::on_pointPixelBtn_clicked(bool checked)
{
  if (checked) swSizeIsPixel=true;
}

void CPlotOptions::on_pointSquareBtn_clicked(bool checked)
{
  if (checked) swSizeIsPixel=false;
}


void CPlotOptions::on_fontPointCombo_currentIndexChanged(const QString )
{
//    ui->fixedAxisFontRBtn->setEnabled(true);
    ui->fixedAxisFontRBtn->setChecked(true);
    ui->autoAxisFontBtn->setChecked(false);
}

void CPlotOptions::on_autoAxisFontBtn_clicked()
{
//    ui->fixedAxisFontRBtn->setEnabled(false);
}
