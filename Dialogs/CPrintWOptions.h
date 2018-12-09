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

#ifndef CPRINTWOPTIONS_H
#define CPRINTWOPTIONS_H

#include <QDialog>
#include <qprinter.h>
#include <qprintdialog.h>

namespace Ui {
class CPrintWOptions;
}

class CPrintWOptions : public QDialog
{
    Q_OBJECT

public:
    bool bwPrint, //if true black/white print
    doPrint, //se true occorre stampare
    pdfOutput;  //print plot into a pdf file
    int fpSize, //frame pen size (in printer dots)
        cpSize,   //curve pen size (in printer dots)
        sdSize,   //plot dot size (in ptrinter dots)
       thinPrint;
    void setPrinter(QPrinter *myPrinter_);
    explicit CPrintWOptions(QWidget *parent = 0);
    ~CPrintWOptions();

private slots:
    void on_printBtn_clicked();
    void on_cancelBtn_clicked();
    void on_portraitRBTn_clicked(bool checked);
    void on_landscRBTn_clicked(bool checked);
    void on_thinRBTn_clicked(bool checked);
    void on_thickRBTn_clicked(bool checked);

    void on_pdfCBOX_clicked(bool checked);

private:
    bool portrait;
    QPrinter * myPrinter;
    Ui::CPrintWOptions *ui;
};

#endif // CPRINTWOPTIONS_H
