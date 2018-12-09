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

#ifndef CFUNSTRINPUT_H
#define CFUNSTRINPUT_H

#include <QDialog>

namespace Ui {
class CFunStrInput;
}

class CFunStrInput : public QDialog
{
    Q_OBJECT

public:
    explicit CFunStrInput(QWidget *parent = 0);
    QString giveStr();
    void getStr(QString str);
    ~CFunStrInput();

private:
    Ui::CFunStrInput *ui;
};

#endif // CFUNSTRINPUT_H
