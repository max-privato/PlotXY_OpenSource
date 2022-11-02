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

#include "CVarMenu.h"
#include <QHeaderView>
#include <QApplication>
#include <QtGui>

CVarMenu::CVarMenu(QWidget *parent):QTableWidget(parent)
{
  verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
//    horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
   dragging=false;
   groupBeginRow=-1;
   neCellBkColor.setRgb(240,240,240);
 }

void CVarMenu::mySetItem(int row, int column, QTableWidgetItem * item, bool monotonic_){
    monotonic.append(monotonic_);
    setItem(row,column, item);
}

void CVarMenu::setCurrFile(int fileNum){
   currFile=fileNum;
}

void CVarMenu::mouseMoveEvent(QMouseEvent *event)
 {
     int row, column;
     if (!(event->buttons() & Qt::LeftButton))
         return;
     if ((event->pos() - dragStartPosition).manhattanLength()
          < QApplication::startDragDistance())
         return;
     row=      rowAt(event->pos().y());
     column=columnAt(event->pos().x());
     if(column!=1)return;

     QDrag *drag = new QDrag(this);
     QMimeData *mimeData = new QMimeData;


     QByteArray varData;
     QDataStream dataStream(&varData, QIODevice::WriteOnly);
     QString varName= itemAt(event->pos())->text();
//     int varNum=row;
     int varNum;
     varNum=item(row,0)->text().toInt()-1;
     dataStream << varName << varNum <<currFile;

     mimeData->setData("MC's PlotXY/var", varData);
     drag->setMimeData(mimeData);
     dragging=true;
     backgroundColor=itemAt(event->pos())->background().color();
     itemAt(event->pos())->setBackground(Qt::yellow);
     drag->exec(Qt::MoveAction);
     /* Non sono riuscito in alcun modo a trovare la maniera di deselezionare il giallo di sfondo nella tabella di destinazione se l'operazione di drop non è effettuata e se l'utente esce direttamente dall'ultima riga (gialla), senza passare da altre righe.
    DETTAGLIO TECNICO: La soluzione naturale per togliere il giallo dallo sfondo della cella sarebbe di utilizzare il "leaveEvent" di CLineChart. Però esso, stranamente (per me è un BUG), non si attiva se la fuoriuscita dal widget avviene con il tasto sinistro premuto, quindi durante una operazione di drop.
Pertanto, anche se non è bello, faccio l'azione di sistemazione dello sfondo della varTable mediante una coppia Signal/Slot!!.*/
     emit draggingDone();
     dragging=false;

     itemAt(event->pos())->setBackground(backgroundColor);
//     itemAt(event->pos())->setBackgroundColor(backgroundColor);
  }


void CVarMenu::keyReleaseEvent(QKeyEvent *)  {
    groupBeginRow=-1;
    itemAt(groupBeginPos)->setBackground(neCellBkColor);
 }


void CVarMenu::mousePressEvent(QMouseEvent *event)  {
    if(event->button() == Qt::LeftButton){
        dragStartPosition = event->pos();
    }
}

void CVarMenu::mouseReleaseEvent(QMouseEvent *event){
  bool rightBtn=false;
  if(dragging)return;
  if(qApp->keyboardModifiers()==Qt::ShiftModifier  ){
    if(groupBeginRow==-1){
      groupBeginRow=rowAt(event->pos().y());
      groupBeginPos=event->pos();
      itemAt(event->pos())->setBackground(Qt::gray);
      return;
    }else{
      //A questo punto sto selezionando la cella terminale di un blocco e comandando la selezione multipla. Per prima cosa lancio un task che mantiene sul grigio la seconda cella del blocco grigia, e poi emetto un segnale di comando della selezione multipla.
      QTimer::singleShot(500, this, SLOT(timerEnd()));
      itemAt(event->pos())->setBackground(Qt::gray);
      groupEndPos=event->pos();
      groupEndRow=rowAt(groupEndPos.y());
      emit groupSelected(groupBeginRow,groupEndRow);
      groupBeginRow=-1;
      groupEndRow=-1;
    }
  }else{
    //Qui faccio una selezione semplice (click su variabile)
      // iy e ix sono gli indici di riga e colonna in cui è avvenuto il click.
      int iy=rowAt(event->pos().y());
      int ix=columnAt(event->pos().x());
      if(ix<0 || iy<0)
        return;
    //Se c'è il modificatore Control non faccio nulla: nel seguito lo userò per convertire l'unità di misura del tempo:

      /*
      if(event->modifiers()==Qt::ControlModifier){
          // Se sono sulla riga 0, quella del tempo, predsponsgo la conversione dai secondi alle ore, mettendo un'appendice al nome; altrimenti non faccio nulla ed esco
        if(iy==0){
            QString txt=item(0,1)->text();
            // verifico di non aver già aggiunto la stringa " (s->h)" al nome della variabile nella prima riga se così è la tolgo:
            if(txt.count()>7)
                txt=txt.mid(txt.count()-7,7);
            if(txt==" (s->h)"){ //A questo punto tolgo la stringa finale
               txt=item(0,1)->text();
               txt.truncate(txt.count()-7);
               item(0,1)->setText(txt);
               return;
            }
        }  // A questo punto posso aggiungere la stringa al nome:
        item(0,1)->setText( item(0,1)->text().append(" (s->h)"));
      }
      return;

      */

      if(event->button()==Qt::RightButton)
      rightBtn=true;
    // Il seguente emit causa l'esecuzione di CDataSelWin::varMenuTable_cellClicked()
    // (connect in CDataSelWin::CDataSelWin()  )
    emit myCellClicked(iy,ix, rightBtn);
  }
}


 void CVarMenu::timerEnd(){
     itemAt(groupEndPos)->setBackground(neCellBkColor);
}
