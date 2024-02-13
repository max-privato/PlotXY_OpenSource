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

#ifndef CPLOTOPTIONS_H
#define CPLOTOPTIONS_H

#include <QDialog>
#include "CLineChart.h"

namespace Ui {
class CPlotOptions;
}

struct SPlotOptions{
    EScaleType xScaleType, yScaleType;
    bool grids, autoAxisFontSize, autoLegendFontSize;
    int axisFontSize;
    EPlotType plotType;
    EPlotPenWidth penWidth;
};

class CPlotOptions : public QDialog
{
    Q_OBJECT
    
public:
    bool accepted,
         customPoints; //true se l'utente ha scelto custo points per le scale
    bool swSizeIsPixel; //se true ed è richiesto plot di tipo swarm, per esso la dimensione del puntino è un semplice plixel
    explicit CPlotOptions(QWidget *parent = 0);
    SPlotOptions giveData();
    void prepare(bool grids, EPlotType plotType, EScaleType xScaleType, EScaleType yScaleType, EPlotPenWidth penWidth);
    ~CPlotOptions();


private slots:
    void on_linesTypeBtn_clicked();
    void on_barsTypeBtn_clicked();
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_dotsTypeBtn_clicked(bool checked);
    void on_linesTypeBtn_clicked(bool checked);
    void on_pointPixelBtn_clicked(bool checked);
    void on_pointSquareBtn_clicked(bool checked);
//    void on_fixedAxisFontRBtn_clicked(bool checked);
    void on_autoAxisFontBtn_clicked();

private:
    Ui::CPlotOptions *ui;
};

#endif // CPLOTOPTIONS_H
