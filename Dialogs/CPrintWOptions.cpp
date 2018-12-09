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

#include "CPrintWOptions.h"
#include "ui_CPrintWOptions.h"
#include <QWidget>

CPrintWOptions::CPrintWOptions(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CPrintWOptions)
{
    ui->setupUi(this);
    bwPrint=false;
    doPrint=false;
    pdfOutput=ui->pdfCBOX->isChecked();
    portrait=true;
    thinPrint=true;
}

CPrintWOptions::~CPrintWOptions()
{
    delete ui;
}

void CPrintWOptions::on_printBtn_clicked()
{

  bwPrint=ui->bwCBOX->checkState();

  /*
   * Per ragioni sconosciute l'attivazione del QPrintDialog danneggia qualcosa nella myPrinter.
   * In particolare se dopo che ho esiquito QPrintDialoc faccio myPrinter->pageRect(); ottengo un rettangolo vuoto.
   * Pertanto rinuncio a questa funzione che, fra l'altro, non era essenziale.
   *
  QPrintDialog dialog(myPrinter);
  dialog.setOption(QAbstractPrintDialog::PrintPageRange,false);
  dialog.setOption(QAbstractPrintDialog::PrintToFile,false);
  QRect prnRect=myPrinter->pageRect();
  if(dialog.exec()==QDialog::Accepted)
    doPrint=true;
  else
    doPrint=false;
*/
  doPrint=true;
  QRect prnRect=myPrinter->pageRect();
  if(portrait)
    myPrinter->setOrientation(QPrinter::Portrait);
  else
    myPrinter->setOrientation(QPrinter::Landscape);
  prnRect=myPrinter->pageRect();
  close();
}

void CPrintWOptions::setPrinter(QPrinter *myPrinter_){
  myPrinter=myPrinter_;
}


void CPrintWOptions::on_cancelBtn_clicked()
{
  doPrint=false;
  close();
}

void CPrintWOptions::on_portraitRBTn_clicked(bool checked)
{
   portrait=checked;
}

void CPrintWOptions::on_landscRBTn_clicked(bool checked)
{
    portrait=!checked;
}

void CPrintWOptions::on_thinRBTn_clicked(bool checked)
{
    thinPrint=checked;
}

void CPrintWOptions::on_thickRBTn_clicked(bool checked)
{
    thinPrint=!checked;
}

void CPrintWOptions::on_pdfCBOX_clicked(bool checked)
{
  pdfOutput=checked;
}
