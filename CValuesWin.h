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

#ifndef CVALUESWIN_H
#define CVALUESWIN_H
#include "Globals.h"
#include "CLineChart.h"
#include <QDialog>
#include <QPainter>

namespace Ui {
class CValuesWin;
}

class CValuesWin : public QDialog
{
    Q_OBJECT

public:
    explicit CValuesWin(QWidget *parent = nullptr);
    void setUp(int nFiles, const QVector<int> &nVars, const QList<SCurveParam> &curveParam_, QList<SFileInfo> filesInfo), //Va richiamata alla visualizzazione della finestra con i dati che non cambiano con il movimento del cursore dati
    updateVarValues(float *x, float **y, bool diff, bool highPrecision); //Questa funzione va richiamata ad ogni movimento del cursore dati.
    ~CValuesWin() override;
signals:
    void interpolationChanged(bool checked);

private slots:
    void on_checkBox_clicked(bool checked);
public slots:
    void setInterpolation(bool interpolation);

private:
    // nella simbologia delle seguenti variabili "x", "y" e "a"  stanno per variabile x,  y e entrmbe ("all") rispettivamente, mentre "h" e "v" stanno per posizione orizzontale e verticale della scrittura sulla finestra.
    int  aVarHPos, 	//ascissa del testo di tutte le variabili
         xVarVPos[MAXFILES],  //ordinata del testo delle variabili tipo "x"
         yVarVPos[MAXVARS],    //ordinate del testo delle variabili "y"
         fileHPos,  //ascissa del testo dei nomi di file
         fileVPos[MAXFILES], //ordinate del testo dei nomi di file
         nFiles,  nTotPlot;
    QString xVarText[MAXFILES], //testo delle variabili "x"
            yVarText[MAXVARS], //testo delle variabili "y"
            fileText[MAXFILES]; //testo dei nomi dei files
    QColor myColors[MAXVARS];
    QFont myFont;
    QVector <int> nPlots;
    Ui::CValuesWin *ui;
    void  paintEvent(QPaintEvent *)override;
};

#endif // CVALUESWIN_H
