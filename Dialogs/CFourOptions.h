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

#ifndef CFOUROPTIONS_H
#define CFOUROPTIONS_H

#include <QDialog>
#include "Globals.h"

namespace Ui {
class CFourOptions;
}

class CFourOptions : public QDialog
{
    Q_OBJECT
    
public:
    explicit CFourOptions(QWidget *parent = 0);
    void getData(SFourData data_);
    void getHMax(int hMax_);
    SFourOptions giveData();
    ~CFourOptions();
    
private slots:
    void on_startTimeEdit_editingFinished();
    void on_endTimeEdit_editingFinished();

private:
    bool dataGot;
    SFourData data;
    Ui::CFourOptions *ui;
    void updatehMax(SFourData data);

};

#endif // CFOUROPTIONS_H
