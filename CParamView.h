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

#ifndef CPARAMVIEW_H
#define CPARAMVIEW_H

#include <QDialog>
#include <qstring.h>
#include <qscrollbar.h>

namespace Ui {
class CParamView;
}

class CParamView : public QDialog
{
    Q_OBJECT

public:
    explicit CParamView(QWidget *parent = nullptr);
    ~CParamView();
    void fillTable();
    void getData(QStringList names_, QList <float> values_, QStringList units_, QStringList
                             descriptions_);
    void resizeEvent(QResizeEvent *);
    void showEvent(QShowEvent *);

private slots:
    void on_matrixBtn_clicked();

private:
    Ui::CParamView *ui;
    bool dataGot;
    bool matrixHidden;
    bool tableFilled;

    QStringList names;
    QList <float> values;
    QStringList units;
    QStringList descriptions;

    void showMatrices();
    void hideMatrices();
    void rearrangeTable(bool excludeMatrices);


};

#endif // CPARAMVIEW_H
