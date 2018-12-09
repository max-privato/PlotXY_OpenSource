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

#ifndef CLABELSDLG_H
#define CLABELSDLG_H

#include <QAbstractButton>
#include <QDialog>
#include "Globals.h"
#include "CLineChart.h"

namespace Ui {
class CLabelsDlg;
}

class CUnitsDlg : public QDialog
{
    Q_OBJECT
    
public:
    bool useBrackets;
    bool useSmartUnits;
    bool doReset; //commands to reset useUserUnits to false
    QString xUnit, yUnit, ryUnit; //Units of measure of the three axes
    explicit CUnitsDlg(QWidget *parent = 0);
    void getTwinScales(bool isTwinScale_);
    void setUseBrackets(bool useBrackets_);
    void setXUnit(QString xUnit_);
    void showEvent(QShowEvent *);
    ~CUnitsDlg();
    
private slots:
    void on_bracketsChkBox_clicked(bool checked);
    void on_yUnitEdit_editingFinished();
    void on_ryUnitEdit_editingFinished();
    void on_xUnitEdit_editingFinished();
    void on_buttonBox_clicked(QAbstractButton *button);
    void on_buttonBox_accepted();

private:
    Ui::CLabelsDlg *ui;
};

#endif // CLABELSDLG_H
