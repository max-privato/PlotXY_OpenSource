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

#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
//#include <QDesktopWidget>
#include <QScreen>
#include "CPlotWin.h"
#include "SuppFunctions.h"
#include "ui_CPlotWin.h"
#include "CDataSelWin.h"  //def. di sFileInfo
#include "CVarTableComp.h" //def di SCurveParam
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#define min(a, b)  (((a) < (b)) ? (a) : (b))


CPlotWin::CPlotWin(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CPlotWin)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
    QScreen *screen=QGuiApplication::primaryScreen();
    exactMatch=false;
    myDPI=int(screen->logicalDotsPerInch());

    /*rendo il minimumSize DPI-aware, e contemporaneamente aggiusto il size default (cioè in assenza di salvataggio da parte dell'utente delle dimensioni.
     Occorre osservare che la logica che ho scelto è quella di fissare il minimumsize di lineChart, non ti PlotWin; la gestione dei layout farà tutto il resto per nostro conto (cf.r CPlotWin.ui)
    */
     QSize minSize=ui->lineChart->minimumSize();
     labelFontBasePoints=ui->interpolateBox->font().pointSize();    
     if(myDPI>100){
       float factor=qMin(myDPI/96.0f,1.5f);
       ui->lineChart->setMinimumHeight(int(factor*minSize.height()));
       ui->lineChart->setMinimumWidth(int(factor*minSize.width()));
       minSize=ui->lineChart->minimumSize();
       // Qui sono nella fase della costruzione, quindi prima del caricamento del registry. Pertanto le modifiche al size che faccio qui non hanno influenza su quello che poi è salvato dall'utente. Mi posso quindi permettere la seguente riga:
       resize(int(factor*geometry().width()),int(factor*geometry().height()));
     }

    //rendo il maximumSize dei bottoni in basso DPI-aware
    if(myDPI>100){
      ui->titleBtn->setMaximumHeight(int(1.5*ui->titleBtn->maximumHeight()));
      ui->optionsBtn->setMaximumHeight(int(1.5*ui->optionsBtn->maximumHeight()));
      ui->scaleTBtn->setMaximumHeight(int(1.5*ui->scaleTBtn->maximumHeight()));
      ui->dataTBtn->setMaximumHeight(int(1.5*ui->dataTBtn->maximumHeight()));
      ui->diffTBtn->setMaximumHeight(int(1.5*ui->diffTBtn->maximumHeight()));
      ui->markBtn->setMaximumHeight(int(1.5*ui->markBtn->maximumHeight()));
      ui->copyBtn->setMaximumHeight(int(1.5*ui->copyBtn->maximumHeight()));
      ui->SVGBtn->setMaximumHeight(int(1.5*ui->SVGBtn->maximumHeight()));
      ui->printTBtn->setMaximumHeight(int(1.5*ui->printTBtn->maximumHeight()));

    }

    QFont myFont;
    myFont=ui->interpolateBox->font();
    // Per il solo Mac aggiusto il font
#if defined(Q_OS_MAC)
    myFont.setPointSize(12);
    ui->interpolateBox->setFont(myFont);
    ui->xValueLbl->setFont(myFont);
    ui->yValueLbl->setFont(myFont);
    labelFontBasePoints=ui->interpolateBox->font().pointSize();
#endif
    QFontMetrics fm(myFont);
    int minimumWidth=fm.width("+#.##e+#")+3;
    ui->xValueLbl->setMinimumWidth(minimumWidth);
    ui->yValueLbl->setMinimumWidth(minimumWidth);
    minimumWidth=fm.width(ui->interpolateBox->text()+"XXX")+2;
    ui->interpolateBox->setMinimumWidth(minimumWidth);

    plotOpsWin = new CPlotOptions(this);
    printWOptsDlg = new CPrintWOptions(this);
    valuesWin = new CValuesWin(this);
    //Il titolo di valuesWin verrà aggiustato nell'evento showEvent. Infatti al momento della costruzione di CPlotWin non è ancora noto il digit finale, che sarà noto immediatamente dopo la sua costruzione.
    connect(ui->lineChart,SIGNAL(valuesChanged(SXYValues,bool,bool)),this, SLOT(chartValuesChanged(SXYValues,bool,bool)));
    connect(ui->lineChart,SIGNAL(chartResizeStopped()),this,
                            SLOT(XYchartResizeStopped()));
    ui->xValueLbl->setVisible(false);
    ui->yValueLbl->setVisible(false);
    ui->interpolateBox->setVisible(false);
    ui->lineChart->linearInterpolate=ui->interpolateBox->checkState();

    dataTBtnChecked=false;
    wasResizing=false;
    numOfTotPlotFiles=0;
    baseTitle="";
    numOfPlots.clear();
    numOfPoints=nullptr;
    ui->lineChart->plotType=ptLine;
    variableStep=nullptr;
    x=nullptr;
    y=nullptr;
    //Il titolo della finestra per le varie istanze è assegnato in CDataSelWin::CDataSelWin. Purtroppo in questa sede non vedo ancora la modifica, mentre vedo soltanto il titolo generico privo del numero di istanza.
    //Assegno alla finestra delle opzioni i valori predefiniti, funzione delle opzioni lette dal programma nel registro di sistema (o in registrazioni analoghe per i sistemi operativi non Windows.
    EPlotPenWidth defaultPenWidth=pwAuto;
//    if(programOptions.useThinLines)
//      defaultPenWidth=pwThin;
    plotOpsWin->prepare(programOptions.useGrids,ptLine,stLin, stLin,defaultPenWidth);
    myScaleDlg =new CScaleDlg(this);
    myScaleDlg->setWindowTitle("Plot scale setup");
  //Connessioni incrociate per far sì che lo stato di interpolazione sia consistente fra il caso di singola curva e curve multiple
    connect(valuesWin,&CValuesWin::interpolationChanged,this,&CPlotWin::on_interpolateBox_clicked);
    connect(this,&CPlotWin::setInterpolation,valuesWin,&CValuesWin::setInterpolation);

#ifdef Q_OS_MAC
    QMargins m=ui->frame->contentsMargins();
    m.setBottom(0);
    m.setTop(0);
    ui->frame->setContentsMargins(m);
#endif

}


void CPlotWin::chartValuesChanged(SXYValues values, bool hDifference, bool vDifference){
    QString msg;
//    msg=msg.setNum(values.X[0],'g',4+exactMatch);
    msg=smartSetNum(values.X[0],4+exactMatch);
    char buffer[17];

    // Qui alfuni tentativi di avere indicati i numeri di decimali giusti dopo la virgola.
    // L'idea è si scrivere sia con E e poi trattare io i digit pe rla rappresentazione
    // a numero di cifre significative giusto ('g' ha lo stesso problema del 'g' di setNum)
    //però dovrei provare l'algoritmo in un programmino a sé stante.
    sprintf(buffer,"%+12.5e",double(values.X[0]));


    if(hDifference)
      msg= "*"+msg;
    ui->xValueLbl->setText(msg);
//    msg=msg.setNum(values.Y[0][0],'g',4+exactMatch);
    msg=smartSetNum(values.Y[0][0],4+exactMatch);
    if(vDifference) msg= "*"+msg;
    ui->yValueLbl->setText(msg);
    valuesWin->updateVarValues(values.X, values.Y,hDifference, exactMatch);
}




void CPlotWin::enterEvent(QEvent *){
    /* Quando entro in una certa istanza della finestra devo fare in modo che la corrispondente tabella varSel venga evidenziata. Pertanto prelevo il digit che caratterizza il numero della finestra considerata, che è nel titolo, lo metto nell'int "i" e emetto sul segnale che verrà utilizzato da dataSelWin per attivare la corrispondente pagina del TabSheet.
  */
    if(!isActiveWindow())
        return;
    // La seguente riga non va bene quando l'opzione "/set" è selezionata. perché ovviamente cambia l'ultimo carattere.
    //    QChar c=windowTitle()[windowTitle().count()-1];
    QChar c=windowTitle()[5];
    QString s=QString(c);
    int i=s.toInt();
    emit winActivated(i);
}

void CPlotWin::getData(float **x1, float*** y1, SCurveParam &x1Info, QList <SCurveParam> *y1Info, QList <SFileInfo> filesInfo){
/* Funzione per la ricezione dei dati dall'esterno prima dell'esecuzione del grafico
Di tutti i dati passati devo creare una copia locale. Infatti CDataSelWin gestisce diverse finestre di grafico e passa i dati a tutte usando i medesimi vettori. Se non si fanno le copie locali, si genera un'interazione indesiderata fra le varie finestre di plot.
Con l'occasione della creazione di queste copie locali, converto il formato dei dati che proviene da CDataSelWin, orientato alle liste (più elegante), a quello utilizzato in CLinechart, orientato ai vettori (più compatibile con la versione BCB).

Per quanto riguarda x1 e y1 potrei copiare soltanto i puntatori ai vettori, visto che i dati sono allocati all'interno di mySO[]. Però questo non andrebbe bene per i grafici che sono ottenuti per elaborazione di dati di input attraverso funzioni.
(Ultima parte a destra di y1: vedere Developer.odt per dettagli).

Pertanto, in analogia con quanto fatto con l'analoga funzione di BCB, preferisco allocare memoria per le complete matrici x e y, e copiare tutti i dati, con ciò effettuando una certa ridondanza di operazioni e allocazioni.

Significato delle variabili passate:
 - x1 e y1 descritte qui sopra e in Developer.odt
 - x1Info contiene informazioni sulla variabile x
 - y1Info è un vettore di liste strutture; ogni elemento del vettore è relativo ad un file di input, e in ogni elemento della lista è una struttura contenente informazioni ad un grafico relativo a quel dato file
 filesInfo è il giusto complemento di y1Info; dà le informazioni attribuibili ai singoli files: nome, indice, numero di punti, se è a passo variabile.

NOTA il numero totale di plot da fare è la somma del numero di elementi contenuti y1Info[] per  ognuno dei files (il numero dei files è pari a filesInfo->count()).

*/

    int i, j, iVarTot, nVarTot;
    int size;
    struct SXVarParam xParam;

    delete[] variableStep;
//    delete[] numOfPlots;
    numOfPlots.clear();
    //Cancello la matrice x. Questa matrice contiene valori locali in CPlotWin dei valori sull'asse x. Viene allocata più sotto non con CreateFMatrix(), ma direttamente allocando array individuali per le singole righe, che pertanto possono non essere adiacenti. La sua cancellazione deve seguire lo stesso criterio.
    if (x!=nullptr){
        for (i=0;i<numOfTotPlotFiles; i++)
           delete x[i];
        delete[] x;
    }
    delete[] numOfPoints;

    ui->dataTBtn->setEnabled(x1Info.isMonotonic);
    //Se xInfo è una funzione il monotonic potrebbe non essere stato attivato ma vale per default:
    if(x1Info.isFunction)
        ui->dataTBtn->setEnabled(false);
    //A questo punto numOfPlotFiles contiene il valore della precedente esecuzione di
    //getData o, se l'esecuzione è la prima, il valore 0. Dopo aver cancellato le vecchie matrici definisco il nuovo valore di numOfFiles e numOfFuns:
    for(i=0; i<numOfTotPlotFiles; i++) if(y[i]!=nullptr)
        DeleteFMatrix(y[i]);
    delete [] y ;
    numOfTotPlotFiles=filesInfo.count();

    variableStep=new bool[filesInfo.count()];
    numOfPoints =new int [filesInfo.count()];

    // Prima di allocare curveParam devo calcolare il numero totale di variabili:
    nVarTot=0;
    for(i=0; i<filesInfo.count(); i++){
      nVarTot+=y1Info[i].count();
    }
    y=new float**[numOfTotPlotFiles];
    x=new float* [numOfTotPlotFiles];
    iVarTot=0;
    lCurveParam.clear();
    for (i=0;i<numOfTotPlotFiles; i++){
      y[i]=CreateFMatrix(y1Info[i].count(),filesInfo[i].numOfPoints);
      numOfPlots.append(y1Info[i].count());
      numOfPoints[i]=filesInfo[i].numOfPoints;
      size=int(sizeof(float))*numOfPoints[i];
      for (j=0; j<y1Info[i].count(); j++){
        lCurveParam.append(y1Info[i][j]);
        iVarTot++;
   //void * memcpy (void *destination, const void *source, size_t num);
        memcpy(y[i][j],y1[i][j], size_t(size));
      }
      x[i]=new float[numOfPoints[i]];
      size=int(sizeof(float))*numOfPoints[i];
      memcpy(x[i],x1[i],size_t(size));
      for(int j=0; j<numOfPoints[i]; j++)
        x[i][j]+=filesInfo[i].timeShift;
    }
    numOfTotPlots=iVarTot;
    xParam.isVariableStep=true;
    xParam.isMonotonic= x1Info.isMonotonic;
    xParam.unitS=x1Info.unitS;
    xParam.name=x1Info.name;
    /***********  inizio Software per debug **************
        float x_dbg[3], y_dbg[3][3];
        int
            var_dbg=min(3,y1Info[0].count()),
            pt_dbg=min(3,numOfPoints[0]);
        for(int iPt=0; iPt<pt_dbg; iPt++)
           for(int iVar=0; iVar<var_dbg; iVar++){
               x_dbg[iPt]=x1[0][iPt];
               y_dbg[iVar][iPt]=y[0][iVar][iPt];
           }
    ***********  fine Software per debug ****************/
    currFileInfo=filesInfo[0];
    if(numOfTotPlots==1 && currFileInfo.frequencyScan && programOptions.barChartForFS)
      ui->lineChart->plotType=ptBar;
    ui->lineChart->getData(filesInfo, numOfPlots, xParam, lCurveParam,x,y);

    //In futuro l'array curveParamm sarà completamente rimosso e quindi lCurveParam sarà settato in PlotWin direttamente nella funzione getData().
    valuesWin->setUp(numOfTotPlotFiles, numOfPlots, lCurveParam, filesInfo);
}

struct SFourData CPlotWin::giveFourData(){
    /* returns data needed to perform a four chart: when this is called we are sure that PlotWin is displaying a single curve.*/
    int indexLeft, indexRight;
    float timeLeft, timeRight, stepsPerSecond;
    SFourData data;
    data.ret="";
    data.x=x[0];
    data.y=y[0][0];
    data.variableStep=variableStep;
    data.numOfPoints=numOfPoints[0];
    timeRight=x[0][numOfPoints[0]-1];
    timeLeft=timeRight-1.0f/float(programOptions.defaultFreq);
    data.opt.initialTime=timeLeft;
    data.opt.finalTime=timeRight;
    stepsPerSecond=(numOfPoints[0]-1) / (x[0][numOfPoints[0]-1]-x[0][0]);
    indexLeft= int((timeLeft-x[0][0])*stepsPerSecond +1);
    indexRight= int((timeRight-x[0][0])*stepsPerSecond);
    if(timeLeft<x[0][0] || timeRight>x[0][numOfPoints[0]-1] || timeLeft>=timeRight
|| indexRight < indexLeft){
        data.opt.initialTime=x[0][0];
      data.ret="Unable to determine \"Fourier\" time-range based on default frequency.\nSelected Fourier analisys on the full time-range";
    }
    data.varName=lCurveParam[0].name;
    data.fileName=currFileInfo.name;
    return data;
}


void CPlotWin::plot(bool update){
    ui->lineChart->resetMarkData();

    ui->lineChart->useSmartUnits=myScaleDlg->useSmartUnits;
    if(!update)
      ui->lineChart->useUserUnits=false;
    //Nella seguente chiamata devo passare autoScale, che sarà true <=> update è false
    ui->lineChart->plot(!update);
    //La seguente chiamata se del caso aggiorna le informazioni sul tnmpo di esecuzione e sul numero dipunti tracciati sulla barra del titolo:
    XYchartResizeStopped();
}


void CPlotWin::XYchartResizeStopped(void){
  if (!programOptions.showElapsTime)
    return;
  //Questa funzione viene chiamata anche prima che CPlotWin::show() sia andato in esecuzione. In questo caso basetitle = "" e non deve fare nulla.
  if(baseTitle=="")
      return;
  QString str;
  str=str.setNum(ui->lineChart->drawTimeUs/1000.0,'g',4)+ " ms";
  QString ptsStr;
  ptsStr.setNum(ui->lineChart->pointsDrawn);
  str+=" points: "+ptsStr;
  if(programOptions.showElapsTime)
      setWindowTitle(baseTitle+"; t="+str);
}

CPlotWin::~CPlotWin()
{
    delete ui;
    delete valuesWin;
    delete myScaleDlg;
    delete plotOpsWin;
    delete printWOptsDlg;

}

//struct SXVarParam{bool isMonotonic;  bool isVariableStep;  char unit[3];};
//Funzione per il passaggio dati all'oggetto relativa al caso di file multipli:
//void getData(int nFiles, bool *variableStep, int*nPlots, int *nPoints, SXVarParam xVarParam,SCurveParam *curveParam,float **px, float ***py);

void CPlotWin::hideEvent(QHideEvent *){
    on_dataTBtn_clicked(false);
    ui->dataTBtn->setChecked(false);
}


void CPlotWin::mouseReleaseEvent(QMouseEvent *){
    //bool b=wasResizing;
}
void CPlotWin::on_dataTBtn_clicked(bool checked)
{
  dataTBtnChecked=checked;
  if(checked){
    ui->lineChart->setActiveDataCurs(1);
    if(numOfTotPlots>1){
      emit setInterpolation(ui->lineChart->linearInterpolate);
      int screenCount=QGuiApplication::screens().count();
      //Lo spazio complessivamente a disposizione è il right() dell'availableGeometry
      // dell'ultimo schermo.
      QScreen *lastScreen=QGuiApplication::screens()[screenCount-1];
      QRect availableGeometry=lastScreen->availableGeometry();
      int availableWidth=availableGeometry.right();
      valuesWin->move(qMin(availableWidth-valuesWin->width()-1,
                   pos().x()+frameGeometry().width()+1),  pos().y());
      valuesWin->show();
      ui->xValueLbl->setVisible(false);
      ui->yValueLbl->setVisible(false);
      ui->interpolateBox->setVisible(false);
      ui->SVGBtn->setVisible(true);
      ui->copyBtn->setVisible(true);
    }else{
      ui->xValueLbl->setVisible(true);
      ui->yValueLbl->setVisible(true);
      ui->interpolateBox->setVisible(true);
      ui->SVGBtn->setVisible(false);
      ui->copyBtn->setVisible(false);
      valuesWin->hide();
    }
  }else{
    ui->lineChart->setActiveDataCurs(0);
    ui->xValueLbl->setVisible(false);
    ui->yValueLbl->setVisible(false);
    ui->interpolateBox->setVisible(false);
    ui->SVGBtn->setVisible(true);
    ui->copyBtn->setVisible(true);
    valuesWin->hide();
  }
  //Con le seguenti due righe faccio in modo che l'utente può premere i tasti freccia e muovere il cursore. Se non le usassi l'utente prima dovrebbe cliccare nell'area di LineChart per attribuirle l'input focus.
  //La prima delle due righe serve nel caso in cui ho più di una variabile e quindi oho visualizzato valuesWin: infatti da una parte la finestra attuale deve essere attiva, dall'altra lineChart deve avere il focus di input.
  activateWindow();
  ui->lineChart->setFocus();
  ui->diffTBtn->setEnabled(checked);
}

void CPlotWin::on_interpolateBox_clicked(bool checked)
{
    ui->lineChart->linearInterpolate=checked;
}

void CPlotWin::on_diffTBtn_clicked()
{
    if(ui->lineChart->giveActiveDataCurs()==0)
        return;
    if(ui->lineChart->giveActiveDataCurs()==1){
        ui->lineChart->setActiveDataCurs(2);
        return;
    }
    if(ui->lineChart->giveActiveDataCurs()==2){
        ui->lineChart->setActiveDataCurs(1);
    }
}

void CPlotWin::showEvent(QShowEvent *){
    /* In questa routine effettuo operazioni varie.
Occorre ricordarsi che questa routine è richiamata ogni qual volta che la finestra è chiusa  (ad esempio cliccando sul bottone nella barra di sistema) e poi riaperta.*/
    QString valuesWinTitle;
    /* Aggiusto il titolo della finestra Values che appartiene a questa istanza dell'oggetto aggiungendo il numero della finestra win di appartenenza (a 1 a 4):
La finestra valuesWin ha un titolo differente a seconda che si sia in Mac o no.
In mac non si riescono a sopprimere i bottoni di sistema, che sprecano spazio, e quindi il titolo deve essere più conciso; in Win, in cui ho più spazio, mi posso permettere un titolo più espressivo.*/
#if defined(Q_OS_MAC)
    valuesWinTitle="p.1";
#else
    valuesWinTitle="plot 1";
#endif
    //Il baseTitle è un titolo a cui poi posso aggiungere l'indicazione del tempo di esecuzione. Lo devo assegnare qui in quanto nella funzione CPlotWin::CPlotWin non contiene ancora il numero di istanza che è aggiunto in CDataSelWin::CDataSelWin.
    //Però devo evitare che a ogni riapertura della finestra le stringhe contenenti i tempi si accumulino. In precedenza usavo un flag con una variabile statica "baseTitleSet". E' stato però visto che la variabile statica (evitentemente come tutte le variabli statiche, ma non lo sapevo) mantiene il suo valore da un'istanza all'altra. Pertanto in quel caso dalla seconda finestra plot in poi avevo una visualizzazione scorretta.
    // Pertanto sono passato ad individuare la necessità di scrivere basetitle sulla base della lunghezza del medesimo titolo della finestra.
    if(windowTitle().count()<7){
        baseTitle=windowTitle();
    }
    // Quando si attiva questa finestra può essere che il box "interpolate" non sia allineato con lineChart, in quanto lo stato di interpolazione di lineChart può essere stato modificato agendo sulla casella di iinterpolazione di winvalues.
    //La cosa si risolve brillantemente con la seguente riga:
    ui->interpolateBox->setChecked(ui->lineChart->linearInterpolate);

    valuesWinTitle[valuesWinTitle.count()-1] = windowTitle()[windowTitle().count()-1];
    valuesWin->setWindowTitle(valuesWinTitle);
    valuesWin->move(pos().x()+frameGeometry().width()+1,  pos().y());
    //DrawType è passata a questa classe subito dopo la costruzione. Non lo posso quindi passare a lineChart nel costruttore, ma posso qui, dopo la costruzione e prima della visualizzazione.
    ui->lineChart->drawType=EDrawType(drawType);

}

void CPlotWin::updateChartOptions(SOptions opts_){
  programOptions=opts_;
  EPlotPenWidth eppw;
  if(programOptions.plotPenWidth==0)
    eppw=pwThin;
  if(programOptions.plotPenWidth==1)
    eppw=pwThick;
  if(programOptions.plotPenWidth==2)
    eppw=pwAuto;
  ui->lineChart->setPlotPenWidth(eppw);
  ui->lineChart->autoLabelXY=programOptions.autoLabelXY;
  ui->lineChart->useBrackets=programOptions.useBrackets;
  ui->lineChart->xGrid=ui->lineChart->yGrid=programOptions.useGrids;
  if(programOptions.onlyPoints)
    ui->lineChart->plotType=ptSwarm;
  else
    ui->lineChart->plotType=ptLine;
  plotOpsWin->prepare(programOptions.useGrids,ptLine,stLin, stLin,eppw);
  myScaleDlg->setUseBrackets(opts_.useBrackets);
}


void CPlotWin::on_SVGBtn_clicked()
{
    bool svgCreated=false, pngCreated=false;
    int index=-1;
    QString fileName, msg;
    fileName = QFileDialog::getSaveFileName(this, "Save File", QDir::currentPath(),
        "SVG and PNG (*.svg *.png)");
    // Se l'utente ha cliccato "ESC" o "annulla" non faccio nulla;
    if (fileName=="")
      return;
    //    devo togliere l'estensione per poi aggiungere "svg" e "png":'
    if(fileName.contains('.')){
        index=fileName.lastIndexOf('.');
        fileName=fileName.left(index);
    }
    msg=ui->lineChart->makeSvg(fileName+".svg",false);
    if(msg.size()==0)
      svgCreated=true;
    ui->lineChart->makePng(fileName+".png", false);
    if(msg.size()==0)
      pngCreated=true;
   //Messaggi
    if(svgCreated && pngCreated){
      QFileInfo FI (fileName);
      fileName=FI.baseName();
      msg="SVG and PNG drawings successfully created and saved"
        "\ninto files "+fileName+".svg and "+fileName+".png.";
    }else if (svgCreated)
      msg="unable to create PNG file";
    else
      msg="unable to create SVG file";
    QMessageBox::information(this,"CPlotWin",msg);
}


void CPlotWin::on_copyBtn_clicked()
{
    ui->lineChart->copy();
}
void CPlotWin::resizeEvent(QResizeEvent *){
    wasResizing=true;
    //Il font di InterpolateLabel per le finestre piccole è un pò sproporzionato
    QFont myFont;
    myFont=ui->interpolateBox->font();
    if(width()<400.0*myDPI/96)
      myFont.setPointSize(labelFontBasePoints-4);
    else if(width()<600.0*myDPI/96)
      myFont.setPointSize(labelFontBasePoints-2);
    else if(width()>800.0*myDPI/96)
      myFont.setPointSize(labelFontBasePoints+2);
    else
      myFont.setPointSize(labelFontBasePoints);
    ui->interpolateBox->setFont(myFont);
    ui->xValueLbl->setFont(myFont);
    ui->yValueLbl->setFont(myFont);
    QFontMetrics fm(myFont);
    int minimumWidth=fm.width("+#.###e+#")+3;
    ui->xValueLbl->setMinimumWidth(minimumWidth);
    ui->yValueLbl->setMinimumWidth(minimumWidth);
    minimumWidth=fm.width(ui->interpolateBox->text()+"XXX")+2;
    ui->interpolateBox->setMinimumWidth(minimumWidth);

}


void CPlotWin::on_optionsBtn_clicked()
{
    SPlotOptions opt;
    plotOpsWin->exec();
    //recupero i dati scelti dall'utente:'
    opt=plotOpsWin->giveData();
    ui->lineChart->setXScaleType(opt.xScaleType); //{stLin, stDB, stLog}
    ui->lineChart->setYScaleType(opt.yScaleType); //{stLin, stDB, stLog}
    ui->lineChart->xGrid=opt.grids;
    ui->lineChart->yGrid=opt.grids;
    ui->lineChart->plotType=opt.plotType; //{ptLine,ptBar, ptSwarm}
    ui->lineChart->setPlotPenWidth(opt.penWidth); //{pwThin, pwThick, pwAuto}
    if(plotOpsWin->swSizeIsPixel)
      ui->lineChart->swarmPointSize=ssPixel;
    else
      ui->lineChart->swarmPointSize=ssSquare;
    if(!plotOpsWin->accepted)return;
    if(!opt.autoAxisFontSize){
      ui->lineChart->fontSizeType=CLineChart::fsFixed;
      ui->lineChart->fixedFontPx=opt.axisFontSize;
    } else{
        ui->lineChart->fontSizeType=CLineChart::fsAuto;
    }
    //Operazioni di ritracciamento del grafico
    if(ui->lineChart->isZoomed())
      ui->lineChart->plot(false);
    else
      ui->lineChart->plot(true);
}


void CPlotWin::on_markBtn_clicked()
{
    if(ui->dataTBtn->isChecked()){
      ui->lineChart->mark();
    }else{
      ui->lineChart->markAuto();
    }

}

void CPlotWin::on_titleBtn_clicked(bool checked)
{
  if(checked)
    ui->lineChart->enableTitle();
  else
    ui->lineChart->disableTitle();
}


void CPlotWin::on_scaleTBtn_clicked(){
  //Preparazione prima della visualizzazione della scheda:
  myScaleDlg->getInfo(ui->lineChart->giveDispRect(),ui->lineChart->twinScale);
  myScaleDlg->getFullLimits(ui->lineChart->giveFullLimits(),ui->lineChart->cutsLimits);
  myScaleDlg->xUnit=ui->lineChart->givexUnit();
  //Visualizzazione della scheda fino a che non metto dati corretti o faccio Cancel:
  QString ret;
  do{
    int result=myScaleDlg->exec();
    if (result==QDialog::Rejected)return;
    ret=myScaleDlg->validDispRect();
    if(ret!="")
      QMessageBox::warning(this," ",ret);
  }while (ret!="");
  ui->lineChart->setDispRect(myScaleDlg->giveDispRect());

  exactMatch=myScaleDlg->giveExactMatch();

  if(myScaleDlg->useUserUnits){
    ui->lineChart->getUserUnits(myScaleDlg->xUnit, myScaleDlg->yUnit, myScaleDlg->ryUnit);
    ui->lineChart->useUserUnits=true;
  }else{
    ui->lineChart->useUserUnits=false;
  }
  ui->lineChart->useBrackets=myScaleDlg->useBrackets;
  ui->lineChart->exactMatch=myScaleDlg->exactMatch;
  ui->lineChart->useSmartUnits=myScaleDlg->useSmartUnits;
  ui->lineChart->plot(false);
//    ui->lineChart->markAll();
}

void CPlotWin::on_printTBtn_clicked()
{
   /* Funzione per selezionare le opzioni di stampa e comandare la stampa*/

    /*Ormai la stampa cartacea da PlotXY è da considerarsi operazione rara. Molto più frequentemente l'utente esporterà i grafici in formato informatico (SVG, PNG), e li importerà in un programma di gtestione documenti.
    Di conseguenza creo la finestra di plot qui, anzicché all'ingresso di CPlotWin.
    */
//  bool ok;
  QPrinter myPrinter;
  QRect prnRect=myPrinter.pageRect();
  QString pdfFileName, fullFileName, ret;
  /* A differenza di BCB non ho una stampante generale di sistema. Creo quindi qui un oggetto printer in quanto alcune opzioni (ad esempio l'orientazione, l'hardware fisico da usare, ecc.) possono essere selezionati all'interno di printWOptsDlg*/
  printWOptsDlg->setPrinter(&myPrinter);
  printWOptsDlg->exec();
  prnRect=myPrinter.pageRect();
  if(printWOptsDlg->doPrint){
    myPrinter.setDocName("MC's PlotXY - Plot Print");
    ui->lineChart->blackWhite=printWOptsDlg->bwPrint;
    pdfFileName="";
    if (printWOptsDlg->pdfOutput){

        pdfFileName = QFileDialog::getSaveFileName(this, "Save File", QDir::currentPath(),
            "PDF  (*.pdf)");
/*
        pdfFileName= QInputDialog::getText(this, "pdf File Name",
            "File name (without \".pdf\"):", QLineEdit::Normal,"pdfFile" , &ok);
        if (ok && !pdfFileName.isEmpty()){
*/
      if (pdfFileName!=""){
        myPrinter.setOutputFileName(pdfFileName);
        QFileInfo FI(pdfFileName);
        FI.makeAbsolute();
        fullFileName=FI.filePath();
        ret=ui->lineChart->print(&myPrinter, printWOptsDlg->thinPrint);
        QMessageBox::information(this,"CPlotWin","Diagram printed on file\n"+fullFileName);
      }
    }else{
      ret=ui->lineChart->print(&myPrinter, printWOptsDlg->thinPrint);
      //Ricordarsi che se ret!="" un messaggio di errore è già stato emesso all'interno di CLineChart
      QMessageBox::information(this,"CPlotWin","Diagram printed on the system printer");
    }
  }
}
void CPlotWin::setDrawType(int drawType_){
    drawType=drawType_;
}


