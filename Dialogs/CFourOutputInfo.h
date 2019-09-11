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

#ifndef CFOUROUTPUTINFO_H
#define CFOUROUTPUTINFO_H

#include <QDialog>

//Il seguente enum è definito anche in CFourWin.h
enum EOutType{otCopy, otPrint};


namespace Ui {
class CFourOutputInfo;
}

class CFourOutputInfo : public QDialog
{
    Q_OBJECT

public:
    bool accepted, numData, pdfOutput, phaseChart, amplChart;
    CFourOutputInfo(QWidget *parent=0);
    void setPhaseChart(bool active);
    void setType(EOutType type);
    ~CFourOutputInfo();
private slots:
    void showEvent(QShowEvent *);
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_numDataChkBox_clicked(bool checked);
    void on_amplChkBox_clicked(bool checked);
    void on_phaseChkBox_clicked(bool checked);

private:
    Ui::CFourOutputInfo *ui;

};

#endif // CFOUROUTPUTINFO_H
