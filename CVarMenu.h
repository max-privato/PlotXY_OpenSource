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

#ifndef CVARMENU_H
#define CVARMENU_H

#include <QTableWidget>

class CVarMenu : public QTableWidget
{
    Q_OBJECT
public:
    QList <bool> monotonic; //vettore che per ogni variabile indica se è monotòna
    explicit CVarMenu(QWidget *parent = 0);
    void mySetItem(int row, int column, QTableWidgetItem * item, bool monotonic_);

signals:
    void draggingDone();
    void groupSelected(int beginRow, int endRow);
    void myCellClicked(int row, int column, bool rightBtn);

public slots:
    void setCurrFile(int);
    void timerEnd();
private:
    bool dragging;
    int currFile,
          groupBeginRow, groupEndRow; //righe iniziale e finale di una selezione di gruppo di variabili
    QColor neCellBkColor;
    QColor backgroundColor;
    QPoint dragStartPosition, groupBeginPos, groupEndPos;
    void keyReleaseEvent(QKeyEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
};

#endif // CVARMENU_H
