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

#include <QAbstractScrollArea>
#include "CVarTableComp.h"
#include <QDragEnterEvent>
#include <QMessageBox>
#include <QScreen>
#include <QtWidgets>

CVarTableComp::CVarTableComp(QWidget *parent): QTableWidget(parent){
    int i,j;
    Qt::ItemFlags flags;
    QBrush myBrush;
    QScreen *screen=QGuiApplication::primaryScreen();
    myDPI=screen->logicalDotsPerInch();
    horizontalHeader()->setVisible(false);
    customiseCol=new CCustomiseCol(this);
    funStrInput=new CFunStrInput(this);

    allowSaving=false;
    timeVarReset=false;
    highestUsedRowIdx=1;
    numOfPlotFiles=0;
    numOfTotVars=0;
    singleFileNum=0;
    neCellBkColor.setRgb(240,240,240);
    headerGray.setRgb(210,210,210);
    hdrs[COLORCOL]="   ";
    hdrs[VARNUMCOL]="#";
    setColumnWidth(VARNUMCOL,fontMetrics().width(hdrs[VARNUMCOL]));
    iniVarNumColWidth=2*fontMetrics().width(hdrs[VARNUMCOL]);
    hdrs[FILENUMCOL]="f";
    hdrs[VARCOL]=" Variable name ";
    hdrs[XVARCOL]="X";


    //Il seguente connect è inutile in quanto la presenza di mouseReleaseEvent lo rende inattivo; pertanto myClicked viene richiamato esplicitamente dall'interno di mouseReleaseEvent.
//    connect(this, SIGNAL(cellClicked(int,int)), this, SLOT(myClicked(int,int)));

    for(i=0;i<=TOTROWS-8;i+=8){
      colors[2+i]=Qt::blue;
      colors[3+i]=Qt::red;
      colors[4+i]=Qt::green;  colors[4+i]=colors[4+i].darker(145);  //0xff00b0
      colors[5+i]=Qt::darkBlue;
      colors[6+i]=Qt::darkRed;
      colors[7+i]=Qt::darkGreen;
      colors[8+i]=Qt::magenta;
    }
//    unsigned int color=colors[3].rgb();
    setAcceptDrops(true);
    setRowCount(TOTROWS);
    setColumnCount(TOTCOLS);

    /* L'altezza delle righe non va toccata qui. E' scelta in CDataSelWin.::adaptToDPI
     */

    myDPI=screen->logicalDotsPerInch();

    //cellFont non va definito perché è già ricevuto da CDtaSelWin tramite getFont()
    //

    for (j=0; j<TOTCOLS; j++){
      QTableWidgetItem *newItem=new QTableWidgetItem;
      newItem->setFont(cellFont);
      newItem->setText(hdrs[j]);
      newItem->setBackground(headerGray);
      newItem->setTextAlignment(Qt::AlignCenter);
      setItem(0,j,newItem);
    }
    for(i=1;i<TOTROWS;i++){
      myBrush.setColor(colors[i]);
      if(i>8)myBrush.setStyle(Qt::VerPattern);
      for(j=0;j<TOTCOLS;j++){
        QTableWidgetItem *newItem=new QTableWidgetItem;
        newItem->setFont(cellFont);
        newItem->setText("");
        newItem->setForeground(myBrush);
        newItem->setBackgroundColor(neCellBkColor);
        newItem->setFlags(flags);
        setItem(i,j,newItem);
        if(j==0)
          newItem->setBackgroundColor(colors[i]);
        if(j!=VARCOL)
          newItem->setTextAlignment(Qt::AlignCenter);
      }
      item(i,COLORCOL)->setForeground(Qt::white);
      if(i>8)
         item(i,COLORCOL)->setText("-");
    }
    //Sul mac il font del numero di variabile può essere un po' più piccolo:
#if defined(Q_OS_MAC)
    for(i=0; i<TOTROWS; i++)
        item(i,VARNUMCOL)->setFont(QFont(font().family(),font().pointSize()-1));
#endif
    //Tolgo le scrollbar:
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    //Inizializzo gli headers:
    verticalHeader()->setVisible(false);

    xVarBrush.setColor(Qt::black);
    bkgroundColor=item(1,1)->backgroundColor();

    //Inizializzo le variabili elementari (bool, int, e ordine alfabetico):
    commonXSet=false;
    multiFile=false;
    timeVarReset=false;
    currFileIdx=0;
    xInfo.idx=-1;
    xInfo.isFunction=false;
    xInfo.isMonotonic=true;
    //inizializzo yLine:
    yLine=new float *[MAXVARSPERFUN];
    for(int i=0; i<MAXVARSPERFUN; i++){
     // I vettori yLine[i]contengono valori fittizzi e vengono creati per consentire di effettuare un check sintattico sulla linea descrivente una funzione di variabile al momento della sua introduione, per evitare di accettare già all'inizio una linea palsemente errata. Per semplicità quindi sono tutti composti da un unico elemento il cui valore è pari al suo indice i.
      yLine[i]=new float[1];
      *yLine[i]=float(i);
    }

}

QString CVarTableComp::analyse(){
/* Questa funzione analizza i dati di tabella e riempie le liste:
- SCurveParam xInfo
- QList <SCurveParam> yInfo [iFile]
- QList <SXYNameData> funInfoLst;

*****
E' ECCETTUATO IL CASO DI xINFO.idx CHE DEVE ESSERE NOTO IN INGRESSO.
Questo è molto importante in quanto quando si fa il load state occorre passare xinfoIdx, per consentire un corretto funzionamento della table. Tutto il resto della xInfo viene ottenuto internamente con questo analyse().
*****
Per ogni valore dell'indice yInfo contiene la lista delle variabili da plottare.
I valori di iFile corrispondono a quelli visualizzati e non sono necessariamente da 0 al max: tutto dipende da cosa c'è nella tabella.
la variabile funInfoLst contiene i parametri delle funzioni, cioè delle variabili ottenute come elaborazione matematica di altre variabili.

Il valore di iFile va da 0 a 2, mentre il valore visualizzato corrispondente va da 1 a 3. Questo conformemente agli standard del C e allo standard di visualizzazione di utente (il primo indice è normalmente 1 e non 0).

La funzione determina inoltre "xVarNum" che è l'indice della variabile da mettere sull'asse x, sempre cominciando da 0. E' quindi il numero che compare sulla tabella in corrispondenza delle variabile contrassegnata con la x diminuito di 1 (nel caso di multifile è sempre 0).

La funzione tiene conto del fatto che si può operare o meno in multiFile. Nel caso di file singolo l'unico valore di iFile è  0.
*/
    extern QString giveUnits(QChar c); //definita in CLineCalc
    int iRow, iFile;
    QString str, ret="";
    SCurveParam myPar;
    //Se la tabella è vuota xVarNum=-1
    if(xInfo.idx==-1)
        return "Empty VarList Table";
    for(iFile=0; iFile<MAXFILES; iFile++){
      yInfo[iFile].clear();
    }

    //Per prima cosa gestisco la variabile x.
    for(iRow=0; iRow<rowCount(); iRow++){
      if(item(iRow,XVARCOL)->text()!="x")continue;

      str=item(iRow,VARNUMCOL)->text();
      if(str[0]=='f'){
        xInfo.isFunction=true;
        xInfo.idx=str.mid(1,1).toInt()-1;
      }else
        xInfo.idx=str.toInt()-1;
      myPar.idx=xInfo.idx;
      myPar.isFunction=xInfo.isFunction;
      myPar.name=item(iRow,VARCOL)->text();
      myPar.color=item(iRow,VARCOL)->foreground().color();
      myPar.rightScale=false;
      myPar.isMonotonic =(xInfo.idx==0);
      xInfo=myPar;
    }

    /* Nel seguente loop per ogni valore di iFile metto nella corrispondente lista fileParams la lista dei parametri delle variabili da plottare. La variabile iFile va da 0 a 7 quando il corrispondente numero visibile in tabella va da 1 a 8
*** Qui opero solo sulle variabili semplici; le funzioni non vanno in yInfo e sono trattate dopo*/
    for(iFile=0; iFile<MAXFILES; iFile++){
      //Per ogni valore di iFile percorro tutte le righe:
      for(iRow=1; iRow<rowCount(); iRow++){
        QChar  c=item(iRow,VARCOL)->text()[0];
        if(item(iRow,XVARCOL)->text()=="x"){
          xInfo.unitS=giveUnits(c);
          continue;
        }
        if (multiFile){
          if(item(iRow,FILENUMCOL)->text().toInt()!=iFile+1)continue;
        }else{
          if(iFile+1!=singleFileNum)
              continue;
          if(item(iRow,VARNUMCOL)->text()[0]=='f'  ||  //si tratta di una funzione
                       item(iRow,VARNUMCOL)->text()=="")   //riga vuota
              continue;
        }
        if(multiFile && item(iRow,FILENUMCOL)->text().toInt()!=iFile+1)continue;
        if(item(iRow,VARCOL)->text()=="")
            continue;
        myPar.idx=item(iRow,VARNUMCOL)->text().toInt()-1;
        myPar.name=item(iRow,VARCOL)->text();
        myPar.color=item(iRow,VARCOL)->foreground().color();
        myPar.rightScale=false;
        if(item(iRow,XVARCOL)->text()=="r")
          myPar.rightScale=true;
        if(item(iRow,COLORCOL)->text()=="-")
          myPar.style=(Qt::DashLine);
        else
          myPar.style=(Qt::SolidLine);
        yInfo[iFile].append(myPar);
      }
    }

    numOfPlotFiles=0;
    for(iFile=0; iFile<MAXFILES; iFile++){
      if(yInfo[iFile].count()>0) numOfPlotFiles++;
    }

    //Ora compilo funInfoLst
    CLineCalc myLineCalc;
    SXYNameData calcData;
    funInfoLst.clear();
    myLineCalc.getFileInfo(allFileNums, allFileNames, varMaxNumsLst);
    for (int i=0; i<rowCount(); i++){
      if(item(i,VARNUMCOL)->text()[0]=='f'){
        QString cCalcLine=item(i,VARCOL)->text();
        myLineCalc.getLine(item(i,VARCOL)->text(),currFileIdx+1);
        calcData=myLineCalc.checkAndFindNames();
        ret=calcData.ret;
        if(ret.length()>0){
           QMessageBox::warning(this, "CCalcData", "ERROR\n"+ret);
            return ret;
        }
        calcData.name=item(i,VARNUMCOL)->text();
        calcData.color=item(i,VARCOL)->foreground().color();
        calcData.style=Qt::SolidLine;
        if(item(i,0)->text()=="-")
           calcData.style=Qt::DashLine;
        calcData.rightScale=item(i,XVARCOL)->text()=="r";
        calcData.lineInt=item(i,VARCOL)->text();
        funInfoLst.append(calcData);
      }
      numOfFuns=funInfoLst.count();
    }
    return "";
    //Naturalmente gli elementi del vettore plotInfo che sono stati compilati sono quelli per cui sono richiesti plot.
    //nulla assicura che siano sequenziali. Potrebbero essere ad es. 1 e 3 o 2 e 3.
    //La rimappatura verrà automaticamente effettuata quando si ricopieranno i valori dei dati dai file di input secondo le indicazioni di plotInfo

}


void CVarTableComp::myReset(bool deep) {
  QBrush myBrush;
  timeVarReset=false;
  for(int i=1; i<rowCount(); i++){
    myBrush.setColor(colors[i]);
    for(int j=1; j<columnCount(); j++){
      if(j<columnCount()-1)
        item(i,j)->setBackground(neCellBkColor);
      item(i,j)->setForeground(myBrush);
      item(i,j)->setText("");
      item(i,j)->setToolTip("");
    }
    item(i,COLORCOL)->setBackgroundColor(colors[i]);
    item(i,COLORCOL)->setForeground(Qt::white);
  }
  numOfTotVars=0;
  item(1,XVARCOL)->setText("x");
  if(deep)
    for(int j=1; j<columnCount(); j++)
       if(j!=XVARCOL)
           item(1,j)->setText("");
  commonXSet=false;
  xVarRow=1;
  xInfo.isFunction=false;
  funSet.clear();
  tabFileNums.clear();
  highestUsedRowIdx=1;
}

void CVarTableComp::resizeEvent(QResizeEvent *){

    int i,j,
           wi[TOTCOLS]; //larghezze delle varie colonne

    int x,y,w,h;
    double factor=96.;

    geometry().getRect(&x,&y,&w,&h);

    //L'altezza delle righe rowHeight() è scelta per tutte le righe in CDataSelWin.::adaptToDPI.
    setGeometry(x,y,w,rowCount()*rowHeight(0)+1);

    // Assegno la larghezza delle colonne. La larghezza di VARNUMCOL deve essere adeguata al contenuto. Le altre colonne eccetto VARCOL hanno larghezza prefissata, determinata dalle dimensioni orizzontali delle celle di header. La larghezza di VARCOL viene scelta in modo che la tabella occupi tutto lo spazio disponibile.

    //Purtroppo resizeColumnToContents(VARNUMCOL); ha un comportamento insoddisfacente, quindi faccio il calcolo manuale.
    wi[VARNUMCOL]=0;
    for (i=1; i<rowCount(); i++){
      wi[VARNUMCOL]=qMax(wi[VARNUMCOL],fontMetrics().width(item(i,VARNUMCOL)->text()));
//      if(i<4){
//        qDebug()<<"item text: "<<item(i,VARNUMCOL)->text();
//        qDebug()<<"width: "<<fontMetrics().width(item(i,VARNUMCOL)->text());
//      }
    }
    wi[VARNUMCOL]=int(myDPI/factor*qMax(wi[VARNUMCOL],fontMetrics().width(hdrs[VARNUMCOL])));
    wi[VARNUMCOL]+=int(myDPI/factor*fontMetrics().width("X"));
    setColumnWidth(VARNUMCOL,wi[VARNUMCOL]);

    i=0;
    for(j=0;j<TOTCOLS;j++){
        if(j!=VARNUMCOL)
            wi[j]=fontMetrics().width(hdrs[j]);
        if(j!=VARCOL)
            i+=wi[j];
    }
#if defined(Q_OS_MAC)
    factor=72.0;
#endif
    wi[FILENUMCOL]+=int(8*myDPI/factor);
    wi[XVARCOL]+=int(7*myDPI/factor);
#if defined(Q_OS_MAC)
    wi[FILENUMCOL]+=int(3*myDPI/factor);
    wi[XVARCOL]+=int(2*myDPI/factor);
#endif
    wi[VARCOL]=w-i-int(17.*myDPI/factor); //attribuisco a VARCOL tutto lo spazio disponibile diminuito di quello occupato dalle altre colonne, determinato sulla base dei contenuti (e non delle dimensioni della tabella, così come risultano dal resize in atto)
    for(i=0;i<TOTCOLS;i++)
      setColumnWidth(i,wi[i]);
}

void CVarTableComp::dragEnterEvent(QDragEnterEvent *event)  {
  if (event->mimeData()->hasFormat("MC's PlotXY/var"))
      event->accept();
  else
      event->ignore();
}

void CVarTableComp::dragMoveEvent(QDragMoveEvent *event)  {
    int row=rowAt(event->pos().y());
    if(row==0 || item(row,VARCOL)->text()!="")return;
    for(int i=1;i<rowCount();i++)
        item(i,VARCOL)->setBackground(neCellBkColor);
    item(row,VARCOL)->setBackground(Qt::yellow);
}

void CVarTableComp::dropEvent(QDropEvent *event)  {
    int row=rowAt(event->pos().y());
    if(row==0) return;

    if(item(row,VARCOL)->text()==""){
        QByteArray varData = event->mimeData()->data("MC's PlotXY/var");
        QDataStream dataStream(&varData, QIODevice::ReadOnly);
        QString varName;
        QString str;
        int varIdx,fileIdx;
        dataStream >> varName >> varIdx >> fileIdx;
        tabFileNums<<fileIdx+1;
        item(row,VARNUMCOL)->setText(str.setNum(varIdx+1));
        if(multiFile)
          item(row,FILENUMCOL)->setText(str.setNum(fileIdx+1));
        item(row,VARCOL)->setText(varName);
        item(row,FILENUMCOL)->setBackgroundColor(neCellBkColor);
        highestUsedRowIdx=qMax(row,highestUsedRowIdx);
        resizeEvent(nullptr);
        // nel momento che ho qualche variabile selezionata posso attivare la possibilità di fare variabili-funzione. Pertanto rendo chiare le celle della colonna f che sono divenute "cliccabili";
        for(int ii=1; ii<rowCount(); ii++){
          if(item(ii,FILENUMCOL)->text()==""&&item(ii,VARNUMCOL)->text()=="")
            item(ii,FILENUMCOL)->setBackgroundColor(Qt::white);
        }
        item(row,FILENUMCOL)->setBackgroundColor(neCellBkColor);
        event->acceptProposedAction();
        numOfTotVars++;
        allowSaving=true;
        if(funSet.size()>0 || tabFileNums.toSet().size()>1)
          allowSaving=false;
        emit contentChanged();
    }
}


void CVarTableComp::getFileNums(QList <int> fileNums, QList <int> varNums) {
    /* Quasi sempre fileNumsLst vengono chieste da CVarTableComp::queryFileNums all'esterno, in quanto esse servono a seguito di azioni dell'utente.
     * Però vi è un caso in cui il processo è inverso. Questo accade quando soto facendo il load state, e devo caricare i valori di queste liste per consentire a CLineCalc di fare la verifica sintattica.
     * In questo caso CDataSelQin chiamerà la presente funzione
     *
*/
    allFileNums=fileNums;
    varMaxNumsLst=varNums;
    // Se ho selezionato un unico file il calore di sinfleFileNum deve prender eil numero di quel file. Questo perché nel normale funzionamento singleFileNum è settato nella funzione setVar() mandata in esecuzion equando si seleziona una variabile, incluso quando si switcha su una tabella Plot.
    //Ma quando carico lo stato questa variabile rimarrebbe a 0 e essa viene testata in CVarTable::Analyse(). Se quindi sto caricando uno stato che ha informazioni sia in plot1 che in plot2, e singlefileNum non è scelto, al secondo plot analyse() non mette il valore corretto di yFile e alle fine il programma va in crash.
    int numOfFiles=0, numOfSingleFile=-1;

    for (int i=0; i<MAXFILES; i++)
       if (fileNums[i]>0){
          numOfFiles++;
          numOfSingleFile=i+1;
       }
    if(numOfFiles==1)
        singleFileNum=numOfSingleFile;

}

int CVarTableComp::giveFileNum(int row){
    return item(row,FILENUMCOL)->text().toInt();
}

void CVarTableComp::filterOutVars(QList <QString> varList){
 /*Questa funzione serve per eliminare dalla tabella variabili presenti che però non
  * sono anche presenti nella varLilst passata.
  * Essa è richiamata quando c'è un refresh in atto per aggiornare le variabili.
  * Infatti alcune variabili possono essere state aggiunte, alterando il numero di variabili nel
  * file corrispondente al nome scelto, e altre possono essere state rimosse.
  *
  * varList contiene la lista delle variabili nella varMenuTable: dovrò filtrare via
  * quindi  le variabili in "this" che non sono presenti in varList
  */
    int varIndex;
    for (int iRow=1; iRow<rowCount(); iRow++){
        if(item(iRow,FILENUMCOL)->text().toInt()!=currFileIdx+1)
            continue;
        if(item(iRow,XVARCOL)->text()=="x")
            continue;
        QString str=item(iRow,VARCOL)->text();
        varIndex=varList.indexOf(item(iRow,VARCOL)->text());
        if(varIndex>-1)
            // aggiorno il valore numerico a quello del file refreshato:
            item(iRow,VARNUMCOL)->setText(QString::number(varIndex+1));
         else
          myClicked(iRow,VARCOL);
    }
}

void CVarTableComp::getFont(QFont font_){
    cellFont=font_;
    for(int i=0; i<rowCount(); i++)
      for(int j=0; j<columnCount(); j++)
         item(i,j)->setFont(cellFont);
}

void CVarTableComp::getColorScheme(bool useOldColors_){

  /* Il numero massimo di variabili in tabella, incluso il tempo, è TOTROWS (attualmente,
   * e probabilmente per sempre) pari a 16). I colori differenti sono 8; i colori andranno
   * quindi mappati in blocchi di 8 distinti colori.
   */
  for(int i=0;i<=rowCount()-8;i+=8){
    if(useOldColors_){
      colors[2+i]=Qt::red;
      colors[3+i]=Qt::green;  colors[3+i]=colors[3+i].darker(145);
      colors[4+i]=Qt::blue;
    }else{
      colors[2+i]=Qt::blue;
      colors[3+i]=Qt::red;
      colors[4+i]=Qt::green;  colors[4+i]=colors[4+i].darker(145);
    }
    colors[5+i]=Qt::darkBlue;
    colors[6+i]=Qt::darkRed;
    colors[7+i]=Qt::darkGreen;
    colors[8+i]=Qt::magenta;
  }
}

void CVarTableComp::getState(QStringList &list, QVector <QRgb> varColRgb, int styleData_, bool xIsFunction_, int xInfoIdx_, bool multiFileMode_ ){
  /* In questa routine si ripristina lo stato salvato sulla baase dei dati salvati in
   * precedenza sul registro. In realtà passo solo
   * il testo da mettere nelle celle e i colori delll variabili; sulla base dei contenuti
   * del testo vengono poi ricomposte le altre variabili strutturate che vengono
   * compilate durante il normale funzionamento del programma.
*/
  int r,c;
  int i=-1;
  int xVarRow=0;
  commonXSet=false;
  xInfo.isFunction=xIsFunction_;
  xInfo.idx=xInfoIdx_;
  multiFile=multiFileMode_;
  styleData=styleData_;
  tabFileNums.clear();
  funSet.clear();
  for(r=1; r<TOTROWS; r++){
    item(r,COLORCOL)->setBackgroundColor(QColor(varColRgb[r-1]));
    if (styleData_&1<<r)
        // il + 1 perché la riga 0 della tabella è l'header e ad essa non corrisponde alcun colore e alcun dash
        item(r+1,COLORCOL)->setText("-");
    for(c=1;c<TOTCOLS; c++){
      i++;
      item(r,c)->setText(list[i]);
    }
    if(item(r,XVARCOL)->text()=="x")
        xVarRow=r;
  }
  // Ora che ho messo i colori e i dash sulla tabella passo i relativi valori a customiseCol, in modo che alla prossima visualizzazione si parta dai valori recuperati:
  customiseCol->getStates(styleData_, varColRgb);
  // numOfTotVars è 0 se prima di cliccare sul bottone di caricamento dello stato non avevo ancora caricato la variabile tempo, 1 altrimenti.
  if(item(1,VARNUMCOL)->text()=="")
      numOfTotVars=0;
  else
      numOfTotVars=1;

  //Now we manage the background of cells below "f" column.
  // We start with white, then we add grey when we have rows with variables, which are not function plot definitions

  for(r=1; r<TOTROWS; r++){ //r=1 + la riga default della x
    item(r,FILENUMCOL)->setBackgroundColor(Qt::white);
    if(item(r,XVARCOL)->text()!="")
      item(r,FILENUMCOL)->setBackgroundColor(neCellBkColor);
    if(item(r,VARNUMCOL)->text()!=""){
      // Se c'è qualcosa sulla riga sotto quella default della x aumento di 1 il numero di
      // variabili; la casella sotto f diviene grigia solo se non si tratta di funzione di variabile.
      QString str=item(r,VARNUMCOL)->text();
      if(r>1)
        numOfTotVars++;
      if(r>1 &&item(r,VARNUMCOL)->text()[0]!='f')
        item(r,FILENUMCOL)->setBackgroundColor(neCellBkColor);
      if(item(r,VARNUMCOL)->text()[0]=='f')
        funStrInput->getStr(item(r,VARCOL)->text());
    }
//    if(item(r,XVARCOL)->text()=="")
    if(item(r,FILENUMCOL)->text().toInt()>0)
      tabFileNums.append(item(r,FILENUMCOL)->text().toInt());
    if(item(r,VARNUMCOL)->text().left(1) =="f")
      funSet.insert(item(r,VARNUMCOL)->text().mid(1,1).toInt());
  }
  if(numOfTotVars>1)
    allowSaving=true;
  else
    allowSaving=false;
  //Ora faccio lo scambio di colori di riga se la x non è in prima riga
  if(xVarRow!=0){
    QColor color=item(1,0)->backgroundColor();
    QBrush brush=item(1,0)->background();
    item(1,0)->setBackgroundColor(item(xVarRow,0)->backgroundColor());
    item(xVarRow,0)->setBackgroundColor(color);
    for(c=1; c<TOTCOLS; c++){
      item(1,c)->setForeground(item(xVarRow,0)->foreground());
      item(xVarRow,c)->setForeground(brush);
    }
  }
}

int CVarTableComp::givehighestUsedRowIdx(){
  return highestUsedRowIdx;
}


SVarTableState CVarTableComp::giveState(){
  /* Fornisce lo stato della tabella per il successivo dsalvataggio sul registro di
   * del sistema. Occorre notare che se si salva lo stato prima di aver cliccato
   * almeno una volta su uno sheet, nella corrispondente tabella mancano le informazioni
   * default di prima riga. Infatti tali informazioni vengono create proprio
   * quando si clicca sullo sheet considerato: CDataSelWin::
   * on_tabWidget_currentChanged(int index) chiama in esecuzione setCommonX(),
   * ma solo se siamo in multifile e questa operazione non è stata ancora fatta,
   * il che è riconosciuto dal valore pari a -1 di xInfo.idx.
   * La questione è comunque ben organizzata in quanto nello stato si salva
   * anche xInfo.idx.
*/
  int r,c;
  struct SVarTableState s;
  for(r=1; r<TOTROWS; r++){
    QColor color=item(r,COLORCOL)->backgroundColor();
    s.varColors.append(color.rgb());
    if(item(r,COLORCOL)->text()=="-")
      s.styles.append(Qt::DashLine);
    else  // in questo caso text = ""
      s.styles.append(Qt::SolidLine);
    for(c=1;c<TOTCOLS; c++)
      s.allNames.append(item(r,c)->text());
  }
  s.fileList=tabFileNums;
  s.funSet=funSet;
  s.xInfoIdx=xInfo.idx;
  s.xIsFunction=xInfo.isFunction;
  return s;
}


QList <SXYNameData> CVarTableComp::giveFunInfo(){
 /* La presente funzione serve per eliminare nomi apparentemente differenti, ma che puntano alla medesima variabile, quale ad esempio f1v3 se è già presente v3 e il file corrente è il n. 1
NOTA Ora la funzione è sostanzialmente disabilitata perché la rimozione dei f# superflui è fatta ex-ante
  */

    return funInfoLst;  //compilata in analyse()
 /*
  bool dataChanged;
  QList <SXYNameData> list=funInfoLst;
  //ora rimuovo nomi duplicati del tipo f1v3 e v3 se f1 è il file corrente:
  SXYNameData data;
  for (int item=0; item<list.count(); item++){
    dataChanged=false;
    data=list[item];
    //tolgo "data" dalla lista, lo processo (in molti casi resterà invariato) e alla fine sel corpo di questo foreach lo rimetto dentro
    foreach (QString varName, data.varNames){
      if(varName[0]=='f'){
        QString shortName=varName, fileNumStr=varName;
        fileNumStr.remove(0,1); //tolgo la "f" a inizio nome
        fileNumStr.truncate(fileNumStr.indexOf("v")); //tolgo quello che c'è a destra del nuumero
        int file=fileNumStr.toInt();
        if (file!=currFileIdx+1) break;  //se il numero di file non è quello del file corrente non vi sono duplicazioni da rimuovere, e quindi passo alla successiva variabile
        shortName.remove(0,shortName.indexOf('v')); //tolgo la f e il relativo numero
        if(data.varNames.contains(shortName)){
          data.varNames.removeOne(varName);  //se c'è duplicazione rimuovo il nome più lungo dalla lista

          dataChanged=true;
        }
      }
    }
    if(dataChanged){
        list.removeAt(item);
        list.append(data);
    }
  }
  return list;

*/
}

void CVarTableComp::fillFunNames(void){
    /* Questa funzione serve per completare i nomi delle variabili nelle funzioni di
     * variabile, con l'indicazione del file. Ad es. v23 diviene f2v23 nel caso in cui
     * il file la cui lista di variabili visualizzata (cioè il file "default") è il 2.
     * Questo tipo di conversione viene usata quando l'utente cammbia il file default,
     * per non creare difficoltà di interpretazione delle stringhe di definizione delle
     * funzioni di variabile.
*/
    QString filled;
    for(int r=0; r<rowCount(); r++){
        if(item(r,VARNUMCOL)->text()[0]=='f'){
            filled=fillNames(item(r,VARCOL)->text(),currFileIdx+1);
            item(r,VARCOL)->setText(filled);
        }
    }
}


void CVarTableComp::blankCell()  {
//    int t=TOTROWS, v=VARCOL;
//    item(TOTROWS-1,VARCOL)->setBackground(Qt::white);
}

void CVarTableComp::myClicked(int r, int c){
/*Questa funzione è usata solo per chiamata diretta da mouseReleaseEvent: quest'ultima
 * gestisce il click destro e per il click sinistro rimanda qui.
*/

//    QSet <int> testSet;
  int j, nextFun, oldXVarRow=xVarRow;
  QString str, ret;
  CLineCalc myLineCalc;
  // Chiedo a CDataSelWin informazioni sui files e le passo a myLineCalc:
  emit queryFileInfo(allFileNums,allFileNames,varMaxNumsLst);
  myLineCalc.getFileInfo(allFileNums,allFileNames,varMaxNumsLst);

  SXYNameData calcData;
  QString defaultStr=item(r,VARCOL)->text();
  if(r==0) return;  //si è cliccato sull'intestazione

  switch(c){
    case COLORCOL:
      if(customiseCol->exec()==QDialog::Accepted){
        QList <QColor> setColors=customiseCol->setColorf();
        QVector <Qt::CheckState> checkStates=customiseCol->checkStatesf();
        QBrush myBrush;
        for (int  row=1; row<TOTROWS; row++){
          //remember that first row is the header in our table!
          int  row_1=row-1;
          myBrush.setColor(setColors[row_1]);
          item(row,COLORCOL)->setBackgroundColor(setColors[row_1]);
          item(row,FILENUMCOL)->setForeground(myBrush);
          item(row,VARNUMCOL)->setForeground(myBrush);
          item(row,VARCOL)->setForeground(myBrush);
          colors[row]=customiseCol->setColorf()[row_1];
          if(checkStates[row_1]==Qt::Checked){
            item(row,COLORCOL)->setText("-");
          }else{
           item(row,COLORCOL)->setText("");
          }
        }
//        int iii=0;
      }
      break;
    case VARCOL:
      /* Se si è cliccato su un nome di variabile vengono posti uguali a "" (semplice
       * spazio) tutti gli items della riga e il tooltip della variabile
      */
      if(item(r,XVARCOL)->text()=="x"){
        QMessageBox::warning(this,"CVarTableComp","Removing x variable not allowed");
        return;
      }
      if(item(r,c)->text()!=""){
         numOfTotVars--;
         if(item(r,VARNUMCOL)->text()[0]=='f'){
           funSet.remove(item(r,VARNUMCOL)->text().mid(1,1).toInt());
         }
         if(item(r,VARNUMCOL)->text()!="")
             item(r,FILENUMCOL)->setBackgroundColor(Qt::white);
      }
      tabFileNums.removeOne(item(r,FILENUMCOL)->text().toInt());
      for(j=0;j<TOTCOLS;j++)
        item(r,j)->setText("");
      item(r,VARCOL)->setToolTip("");
      //Se ho rimosso l'ultima riga visuaalizzata devo ricarlcolare quale è ora l'ultima:
      if(r==highestUsedRowIdx){
        highestUsedRowIdx=0;
        for (int i=1; i<rowCount(); i++)
          if(item(i,1)->text()!="")
            highestUsedRowIdx=i;
      }
      allowSaving=true;
//      testSet=tabFileNums.toSet();
//      j=tabFileNums.toSet().size();
      if(funSet.size()>0 || tabFileNums.toSet().size()>1)
          allowSaving=false;
      // Chiedo a CDataSelWin di aggiornare le informazioni sulla tabella myLineCalc.
      // Gestisce solo l'attivazione dei bottoni sotto la varTable stessa
      emit contentChanged();
/****  Qui manca l'analisi delle funzioni che eventualmente usano questa variabile!
       ****** se ve ne sono occorrerà deselezionare anch'esse!****/
      break;
    case FILENUMCOL:
      /* In questo caso devo richiedere la formula, richiamare la funzione di variabili.
       * Faccio anche una verifica sintattica per evitare di accettare una funzione inammissibile.
       * Il calcolo effettivo della funzione avverrà in risposta al click sul bottone di plot().
      */
      if(item(r,FILENUMCOL)->backgroundColor()!=Qt::white) break;
      while(1){
        int inpRet=funStrInput->exec();
        if (inpRet==QDialog::Rejected) return;
        str=funStrInput->giveStr();

        defaultStr=str;
        ret=myLineCalc.getLine(str,currFileIdx+1);
        if(ret.length()>0){
          QMessageBox::warning(this,"",ret);
          continue;
        }
        //Analizzo la stringa:
        calcData=myLineCalc.checkAndFindNames();
        //ora giveXYNameData può aver semplificato line. Quindi visualizzo in tabella la versione semplificata
        str=myLineCalc.giveLine("lineInt");
        ret=calcData.ret;
        if(ret.length()>0){
          QMessageBox::warning(this, "", "ERROR\n"+ret);
          continue;
        }
        // Qui devo fare il check sintattico della linea di myLineCalc, per evitare quindi di recepire una stringa palesemente errata.
        // Scelgo di farlo facendo fare il calcolo per il primo dei valori delle variabili-funzione. Prima quindi devo passare la matrice, assicurandomi che il numero di variabili non sia superiore al massimo assunto, pari a MAXVARSPERFUN:
        if(calcData.varNames.count()>MAXVARSPERFUN){
          QMessageBox::critical(this, "MC's PlotXY", "Error yLine size");
          continue;
        }
        ret=myLineCalc.getNamesAndMatrix(calcData.varNames,yLine);
        if(ret.length()>0){
          QMessageBox::critical(this, "MC's PlotXY", "ERROR\n"+ret);
          continue;
        }
        myLineCalc.compute(0);
        ret=myLineCalc.ret;
        if(ret.length()>0){
          QMessageBox::warning(this, "MC's PlotXY", "ERROR\n"+ret);
          continue;
        }else
          break;
      }
//Ora devo attribuire il nome alla funzione, cercando il primo disponibile a partire da 1.
//devo però preliminarmente verificare che non si stia ridefinendo una funzione, nel qual caso il nome non deve cambiare.
      nextFun=-1;
      if(item(r,VARNUMCOL)->text()[0]!='f'){
        for(int i=1;i<10; i++)
          if(!funSet.contains(i)){
            funSet<<i;
            numOfTotVars++;
            nextFun=i;
            break;
          }
      }
//ridefinisco il nome della funzione solo se non si stava ridefinendo una funzione:
      if(nextFun!=-1)
        item(r,VARNUMCOL)->setText("f"+QString::number(nextFun));
      item(r,VARCOL)->setText(str);
      fillNames(str,currFileIdx+1);
      allowSaving=true;
      if(funSet.size()>0 || tabFileNums.toSet().size()>1)
        allowSaving=false;
      emit contentChanged();
      break;
    case VARNUMCOL: //colonna del "#"
      return;
    case XVARCOL: //Colonna della "X"
      // Qui cambio qual è la riga da considerare come variabile x. Però fa fatto solo se il numero di variabili selezionate (incluso il tempo) è almeno pari a 2)
      if(numOfTotVars<3 && !timeVarReset){
        QMessageBox::warning(this,"CVarTableComp","Setting x-variable requires at least two variables to be selected");
        return;
      }
      if(multiFile){
        //In multifile I must be sure that all plots requested are from the same file.
        //For this I need an analyse().
        analyse();
        if(numOfPlotFiles>1){
          QMessageBox::warning(this,"PlotXY-varTableComp",
            "X-Y Plots require selected vars being all from the same file");
          return;
        }else{
          commonXSet=false;
        }
      }
      // If I chose as x a function plot I mustn-t have other function plots, otherwise system crashes (reasons not analysed yet):
      if(item(r,VARNUMCOL)->text()[0]=='f'){
        bool manyFunctionPlots=false;
        for (int i=0; i<rowCount(); i++){
          if(item(i,VARNUMCOL)->text()[0]=='f'&&i!=r){
            manyFunctionPlots=true;
            break;
          }
        }
        if(manyFunctionPlots){
          QMessageBox::warning(this,"PlotXY-varTableComp",
            "You can use a function-plot as an X variable only when no other function plots are requested");
          return;
        }
      }
      if(item(r,XVARCOL)->text()!="")return;
      if(item(r,VARCOL)->text()=="")return;
     /* The table colours must stay stable unless the user changes them.
      * Therefore, if the x variable is already different from the one on the first row,
      *  WITH THE NEXT ROW I first set again the first row as the x variable, then do che
      *  requested change */
      if(xVarRow!=1 && r!=1)
         myClicked(1,c);

      for(j=0;j<TOTCOLS;j++){
        item(xVarRow,j)->setForeground(item(r,j)->foreground());
        item(r,j)->setForeground(xVarBrush);
      }
      item(xVarRow,0)->setBackgroundColor(item(r,0)->backgroundColor());
      item(xVarRow,c)->setText("");
      xVarRow=r;
      item(r,c)->setText("x");
      item(r,0)->setBackgroundColor(colors[0]);
      allowSaving=true;
      // Now I unselect time variable. I do this only if it's in the first row and only once after each reset. In this way the user can add again time, if he wants so, and let it be plotted.
      if(!timeVarReset){
        timeVarReset=true;
        myClicked(oldXVarRow,VARCOL);
      }
      if(funSet.size()>0 || tabFileNums.toSet().size()>1)
        allowSaving=false;
      //Se la variabile su cui si è cliccato è una funzione di variabile devo prendere l'indice a partire dal secondo carattere;
       str=item(r,VARNUMCOL)->text();
      if(str[0]=='f'){
        xInfo.isFunction=true;
        xInfo.idx=str.mid(1,1).toInt()-1;
      }else{
        xInfo.isFunction=false;
        xInfo.idx=str.toInt()-1;
      }
      xInfo.isMonotonic=monotonic[r];
      emit contentChanged();
      break;
    default:
      return;
  }
  if (c!=VARCOL)return;
  //Aggiusto le dimensioni. Ad esempio posso aver rimosso una variabile con un numero di variabile a 3 cifre e le rimanenti hanno 2 cifre. In tal caso la larghezza della relativa colonna va ridotta.
  resizeEvent(nullptr);
}


bool CVarTableComp::isEmpty(){
  bool empty= item(1,2)->text()=="";
  return empty;
}
void CVarTableComp::mouseReleaseEvent(QMouseEvent * event){
    int r=indexAt(event->pos()).row();
    int c=indexAt(event->pos()).column();
    if(c<0 || r<0)
       return;
    if(event->button()!=Qt::RightButton){
        myClicked(r,c);
        return;
    }
    if(item(r,VARCOL)->text()=="")
      return;
    if(item(r,XVARCOL)->text()=="")
      item(r,XVARCOL)->setText("r");
    else if(item(r,XVARCOL)->text()=="r")
        item(r,XVARCOL)->setText("");
}



void CVarTableComp::setCurrFile(int fileIdx){
   currFileIdx=fileIdx;
}


void CVarTableComp::setNameNumber(int number_){ //sets the current index of the container tab
  tabNameNum=number_;
}

void CVarTableComp::setMultiFile(bool multiFile_){
    /* se multifile=true, il click sta chiedendo di entrare in multiFile.*/
    if(multiFile==multiFile_)return;
    multiFile=multiFile_;
    myReset();
    if(multiFile){
//      item(0,XVARCOL)->setText("a");
      monotonic[0]=true;
      xInfo.isMonotonic=true;
//      for(int i=0;i<rowCount();i++)
//          item(i,XVARCOL)->setBackgroundColor(neCellBkColor);
    }else{
//      item(0,XVARCOL)->setText("x");
//      for(int i=0;i<rowCount();i++)
//        item(i,XVARCOL)->setBackgroundColor(neCellBkColor);
    }
}


void  CVarTableComp::setPosition(int x, int y){
    /*Questa funzione riposiziona la tabella senza alterare larghezza e altezza.
      Infatti larghezza e altezza sono determinate nel costruttore sulla base dei contenuti della tabella
    */
    QRect rect=geometry();
    rect.moveTo(x,y);
    setGeometry(rect);

}

int CVarTableComp::setCommonX(QString str){
/* Nel caso di multifile la variabile x è sempre in prima riga e a comune fra i vari
 * files.
 * Faccio qui una funzione specializzata proprio per selezionare la variabile x comune
 * in multifile, per poter implementare alcuni check:
 * - non si possono selezionare variabili se non si è selezionata una commonX
 * - una variabile non commonX non potrà mai prendere la prima riga*/

  if(commonXSet) return 1;
  if(!multiFile) return 2;
  xInfo.idx=0;
  item(1,VARNUMCOL)->setText("");
  item(1,VARCOL)->setText(str);
  item(1,XVARCOL)->setText("x");
  item(1,FILENUMCOL)->setText("a");
  item(1,VARNUMCOL)->setText("1");
  commonXSet=true;
  numOfTotVars++;
    // nel momento che ho qualche variabile selezionata posso attivare la possibilità di fare variabili-funzione. Pertanto rendo chiare le celle della colonna f che sono divenute "cliccabili";
  for(int ii=2; ii<rowCount(); ii++){
    item(ii,FILENUMCOL)->setBackgroundColor(Qt::white);
  }
  emit contentChanged();
  return 0;
}

int CVarTableComp::setVar(QString varName, int varNum, int fileNum, bool rightScale, bool monotonic_, QString unit_){
  /* Questa funzione inserisce una variabile nella prima riga disponibile.
   * Per far questo individua la prima riga con item sulla casella del nome di variabile
   * non nullo e scrive su di essa. La funzione ritorna 0 in caso di funzionamento corretto.
   *
   * Questa funzione PUO' essere usata sia per singleFile che per multiFile. Nel caso di
   * singleFile non scrive alcun numero nella colonna del file.
   * Parametri passati:
   * - str: nome del file
   * - varNum: numero della variabile così come comparirà nella tabella
   * - fileNum: numero del file come comparirà nella tabella se siamo in Multifile,
   * altrimenti verrà memorizzato in singleFileNum
   * - rightScale: dice se la variabile dovrà essere plottata con scala numerica destra
   *
   * Per quanto riguarda l'ultimo parametro, "unit_", non sarebbe necessario a questo
   * oggetto ed in effetti è stato aggiunto solo a Ottobre 2017. La ragione perché ora viene
   * passato è che CDataSelWin quando deve richiedere un plot chiede a CVarTable l'intera
   * lista yInfo, ed è particolarmente agevole qui aggiungere a yInfo il valore di unitS
*/
  int i;
  QResizeEvent * event;

  //se in multiFile è richiesta la selezione di variabile senza aver prima selezionato l'x comune con setCommonX ritorno un errore:
//  if(multiFile && !commonXSet) return 1;

  //se in X-Y mode devo verificare che la variabile selezionata provenga dal file a cui appartengono le altre, altrimenti emetto un messaggio di errore ed esco
  if(tabFileNums.count()>0)
      //La seguente condizione non deve avere xInfo-idx!=0 in quanto se ho appena caricato lo stato con saveState e lo stato salvato non aveva alcuna operazione effettuata sulla tabella xInfo.idx è -1.
    if(xInfo.idx>0 && !tabFileNums.contains(fileNum)){
      QMessageBox::warning(this,"PlotXY-varTableComp",
        "X-Y Plots require selected vars being all from the same file");
      return 1;
    }

  //Se la tabella era vuota assegno alla variabile il ruolo di var. x:
  if(xInfo.idx==-1){
    item(1,XVARCOL)->setText("x");
    xInfo.idx=varNum;
  }
  if(multiFile && fileNum>=TOTROWS)
      return 2;
  //cerco la prima riga disponibile:
  for(i=1;i<TOTROWS;i++)
    if(item(i,VARCOL)->text()=="")
      break;
  //se accade la seguente condizione la tabella era piena:
  if(i==TOTROWS)
      return 3;
  item(i,VARCOL)->setText(varName);
  item(i,VARCOL)->setToolTip(varName);
  highestUsedRowIdx=qMax(i,highestUsedRowIdx);

  QString sNum;
  sNum=sNum.number(varNum);
  item(i,VARNUMCOL)->setText(sNum);
  if(rightScale)
    item(i,XVARCOL)->setText("r");
  if(multiFile)
    item(i,FILENUMCOL)->setText(varName.setNum(fileNum));
  else
    singleFileNum=fileNum;
  tabFileNums<<fileNum;
  monotonic[i]=monotonic_;
  numOfTotVars++;
  // nel momento che ho qualche variabile selezionata posso attivare la possibilità di fare variabili-funzione. Pertanto rendo chiare le celle della colonna f che sono divenute "cliccabili";
  for(int ii=1; ii<rowCount(); ii++){
    if(item(ii,FILENUMCOL)->text()==""&&item(ii,VARNUMCOL)->text()=="")
      item(ii,FILENUMCOL)->setBackgroundColor(Qt::white);
  }
  item(i,FILENUMCOL)->setBackgroundColor(neCellBkColor);
  item(i,XVARCOL)->setToolTip(unit_);
  if(numOfTotVars>1)
    allowSaving=true;
  if(funSet.size()>0 || tabFileNums.toSet().size()>1)
    allowSaving=false;
  emit contentChanged();
  resizeEvent(event=nullptr);
   return 0;
}

void CVarTableComp::showEvent(QShowEvent *){
    QString  s;
    s=item(1,3)->text();
//    int i=0; //only to allow to set breakpoint here
}

int CVarTableComp::unselectFileVars(int fileIndex){
  /* Questa funzione rimuove tutte le variabili selezionate relative ad un dato file, avente l'indice passato. E' utile quando l'utente scarica un file dalla tabella file in CDataSelWin
la funzione va richiamata solo in multifile*/
  int row, col;
  if(!multiFile) return 1;
  for(row=0; row<rowCount(); row++)
    if(item(row,1)->text().toInt()==fileIndex+1)
      for(col=1; col<columnCount(); col++)
        item(row,col)->setText("");
  numOfTotVars=0;
  for(row=0; row<rowCount(); row++)
    if(item(row,2)->text()!="")
      numOfTotVars++;

 /*** Qui andrà aggiunto il deselezionamento delle funzioni che usano il file da eliminare! ***/
  return 0;
}
int CVarTableComp::varColumn(){
    return VARCOL;
}

CVarTableComp::~CVarTableComp(){
    for(int i=0; i<10; i++){
        delete yLine[i];
    }
    delete[] yLine;
}
