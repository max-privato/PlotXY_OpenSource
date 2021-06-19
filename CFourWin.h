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
#define DEFAULTHARM2 40

#include <QDialog>
#include "CLineChart.h"
#include "Globals.h"
#include "Dialogs/CFourOptions.h"
#include "Dialogs/CFourOutputInfo.h"


namespace Ui {
class CFourWin;
}

class CFourWin : public QWidget
{
    Q_OBJECT
    
public:
    struct SFourData myData;
    explicit CFourWin(QWidget *parent = nullptr);
    void focusInEvent(QFocusEvent *) override;
    void getData(struct SFourData data_);
    ~CFourWin() override;
signals:
    void winActivated(int);
public slots:
    void lineChatClickedOn(void);

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
    void computeTHD(); //Oltre a calcolare il THD, se il numero di armoniche richiesto è 40 calcola anche il PWHC secondo IEC 61000-3-2
    bool indexesFromTimes(SFourData data);
    int performDFT();
    int performNuDFT();
//    void resizeEvent(QResizeEvent *) override;
    void showEvent(QShowEvent *) override;
    void valChanged(SXYValues values);

    bool dftDone,
         optionsSetOk;  //se le opzioni selezionate sono ok
    int indexLeft, indexRight;
    int  initialFontPoints;
    QString infoString, amplValueTxt;
    float *harmOrders,
          *ampl, //Ampiezze delle armoniche prima della correzione con amplFactor (definito loocalmente in analiseAndShow).  Sono quindi valori di picco delle sinusoidi
          *amplitudes, //Ampiezze dopo la correzione con amplFactor (ad es. per trasformaz. in p.u.).
          *phases,
          *phases1, *amplitudes1, //puntatori a cui non verrà allocato spazio di memoria in quanto puntano a elementi di amplitudes e phases
          ampl01[2], //Solo le ampiezze di armonica 0 e 1 per fare l'eventuale p.u.
          THD0, THD1,  //THD relativi ad armonica 0 e 1 (in percentuale)
          PWHC, //PWHC secondo IEC 61000-3-2, calcolato solo se le armoniche richieste sono pari a 40; anche questa è in percentuale
          RMS; //RMS considerando il range di armoniche prescelto
};

#endif // CFOURWIN_H
