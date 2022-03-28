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

#ifndef CPLOTWIN_H
#define CPLOTWIN_H

#include <QWidget>
#include "CLineChart.h"
#include "CValuesWin.h"
#include "Dialogs/CScaleDlg.h"
#include "Dialogs/CPlotOptions.h"
#include "Dialogs/CPrintWOptions.h"


namespace Ui {
class CPlotWin;
}

//struct SFloatRect2{float Left,Right,LTop,LBottom,RTop,RBottom;};

class CPlotWin : public QWidget
{
    Q_OBJECT

public:
    bool dataTBtnChecked;
    bool lastWinIsCut; //Assume il valore di windowIsCut dell'ultimo grafico tracciato
    explicit CPlotWin(QWidget *parent = nullptr);
    void focusInEvent(QFocusEvent *) override;
//    void enterEvent(QEvent *) override;
    void getData(float **x1, float*** y1, SCurveParam &x1Info, QList <SCurveParam> *y1Info, QList <SFileInfo> filesInfo);
    struct SFourData giveFourData();
    void getOption(bool useCopiedDialog_); //per ora un'unica opzione possibile; in futuro potrebbe divenire "getOptions" e ricevere una struttura)
    void plot(bool update=false);
    void setDrawType(int drawType_);
    ~CPlotWin() override;

signals:
    void winActivated(int);
    void setInterpolation(bool);
private slots:
    void on_interpolateBox_clicked(bool checked);
    void on_diffTBtn_clicked();
    void on_SVGBtn_clicked();
    void on_copyBtn_clicked();
    void on_optionsBtn_clicked();
    void on_markBtn_clicked();
    void on_titleBtn_clicked(bool checked);
    void on_scaleTBtn_clicked();
    void on_printTBtn_clicked();

public slots:
    void lineChatClickedOn(void);

    void XYchartResizeStopped(void);
    void chartValuesChanged(SXYValues values, bool hDifference, bool vDifference);
    void on_dataTBtn_clicked(bool checked);
    void updateChartOptions(SOptions programOptions);
private:
    bool exactMatch;
    bool *variableStep, useCopiedDialog, wasResizing;
    int drawType; //numero che corrisponde all'indice dell'enum EDrawtyupe in LineChart: 0, default, è la mia routine filterClip, 1 è QtF, 2 QtI, 3 QtPoly.
    int numOfTotPlotFiles, //Numero dei files da cui si fanno grafici. Ogni funzione vale come un file.
        numOfTotPlots; //contiene il numero di grafici totali, sommando quelli dei vari files eccetto.
    int *numOfPoints;
    int labelFontBasePoints;
    int myDPI; //numero effettivo di punti per pollice
    float **x, ***y;
//    EPlotType plotType;
//    SCurveParam  *curveParam;
    QList <SCurveParam>  lCurveParam;
    QVector <int> numOfPlots;
    SFileInfo currFileInfo;
    CPlotOptions *plotOpsWin;
    CPrintWOptions *printWOptsDlg;
    CScaleDlg *myScaleDlg;
    SOptions programOptions;
    CValuesWin * valuesWin;
    Ui::CPlotWin *ui;
    QString baseTitle; //titolo della finestra senza il tempo aggiunto
    void hideEvent(QHideEvent *) override;
    void showEvent(QShowEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void resizeEvent(QResizeEvent *) override;
};

#endif // PLOTWIN_H
