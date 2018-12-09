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

#ifndef CFOURWIN_H
#define CFOURWIN_H

#define DEFAULTHARM1 0
#define DEFAULTHARM2 30

#include <QDialog>
#include "CLineChart.h"
#include "Globals.h"
#include "dialogs/CFourOptions.h"
#include "dialogs/CFourOutputInfo.h"


namespace Ui {
class CFourWin;
}

class CFourWin : public QWidget
{
    Q_OBJECT
    
public:
    struct SFourData myData;
    explicit CFourWin(QWidget *parent = 0);
    void getData(struct SFourData data_);
    ~CFourWin();
public slots:
    void updateChartOptions(SOptions opts);
    void valChangedAmp(SXYValues values, bool, bool);
    void valChangedPh(SXYValues values, bool, bool);

private slots:
    void on_optionsBtn_clicked();
    void on_infoBtn_clicked();
    void on_copyBtn_clicked();
    void on_printBtn_clicked();
    void on_gridChkBox_clicked();
    void on_saveSetBtn_clicked();
    void resetStateBtns(void);

private:
    Ui::CFourWin *ui;
    CFourOptions *fourOptions;
    CFourOutputInfo *fourOutInfo;
    int analyseAndShow(bool changed);

    void copyOrPrint(EOutType type);
    void computeTHD();
    bool indexesFromTimes(SFourData data);
    int performDFT();
    void resizeEvent(QResizeEvent *);
    void showEvent(QShowEvent *);
    void valChanged(SXYValues values);

    bool dftDone,
         optionsSetOk;  //se le opzioni selezionate sono ok
    int indexLeft, indexRight;
    int  initialFontPoints;
    QString thdString, amplValueTxt;
    float *harmOrders,
          *ampl, //Ampiezze delle armoniche prima della correzione con amplFactor
          *amplitudes, //Ampiezze dopo la correzione con amplFactor (ad es. per trasformaz. in p.u.).
          *phases,
          ampl01[2], //Solo le ampiezze di armonica 0 e 1 per fare l'eventuale p.u.
          THD0, THD1; //THD relativi ad armonica 0 e 1 (in percentuale)
};

#endif // CFOURWIN_H
