#include "CCustomiseCol.h"
#include "ui_CCustomiseCol.h"

CCustomiseCol::CCustomiseCol(QWidget *parent) :
  QDialog(parent), ui(new Ui::CCustomiseCol){
  ui->setupUi(this);
  for (int column=0; column<ui->customColorTable->columnCount()-1; column++){
    ui->customColorTable->item(0,column)->setCheckState(Qt::Checked);
    ui->customColorTable->item(0,column)->setCheckState(Qt::Unchecked);
  }
  for (int column=8; column<ui->customColorTable->columnCount()-1; column++){
    ui->customColorTable->item(0,column)->setCheckState(Qt::Checked);
  }
  for (int column=0; column<ui->customColorTable->columnCount()-1; column++){
    checkStates.append(ui->customColorTable->item(0,column)->checkState());
  }
  //By default, items are enabled, editable, selectable, checkable, and can be used both as the source of a drag and drop operation and as a drop target.
  int numCols=ui->customColorTable->columnCount();
  for (int column=0; column<numCols; column++){
    Qt::ItemFlags flags=ui->customColorTable->item(0,column)->flags();
    ui->customColorTable->item(0,column)->setFlags(flags&~Qt::ItemIsEditable);
    flags=ui->stdColorTable->item(0,column)->flags();
    ui->stdColorTable->item(0,column)->setFlags(flags|Qt::ItemIsDragEnabled);
  }
  ui->customColorTable->item(0,numCols-1)->setFlags(Qt::NoItemFlags);
  ui->customColorTable->item(0,numCols-1)->setForeground(QColor(0,0,0));
  ui->stdColorTable->item(0,numCols-1)->setFlags(Qt::NoItemFlags);
  ui->stdColorTable->item(0,numCols-1)->setForeground(QColor(0,0,0));
}

void CCustomiseCol::resizeEvent(QResizeEvent *){
  static bool initialResized=false;
  if(initialResized)
    return;
  initialResized=true;
  int coloredColumns=ui->stdColorTable->columnCount()-1;
  ui->stdColorTable->resizeColumnToContents(coloredColumns);
  ui->customColorTable->resizeColumnToContents(coloredColumns);
  int cWidth;
  cWidth=(ui->stdColorTable->width()-ui->stdColorTable->columnWidth(coloredColumns))/
               coloredColumns;
  for(int i=0; i<coloredColumns; i++){
      ui->stdColorTable->setColumnWidth(i,cWidth);
  }
  for(int i=0; i<coloredColumns; i++){
      ui->customColorTable->setColumnWidth(i,cWidth);
  }
  cWidth=ui->stdColorTable->columnWidth(coloredColumns);
  ui->stdColorTable->setColumnWidth(coloredColumns,cWidth+20);
  cWidth=ui->customColorTable->columnWidth(coloredColumns);
  ui->customColorTable->setColumnWidth(coloredColumns,cWidth+10);

  ui->stdColorTable->setRowHeight(0,int(1.5*ui->stdColorTable->rowHeight(0)));
  ui->customColorTable->setRowHeight(0,int(1.5*ui->customColorTable->rowHeight(0)));
  ui->customColorTable->setMaximumHeight(ui->customColorTable->rowHeight(0));
  ui->stdColorTable->setMaximumHeight(ui->stdColorTable->rowHeight(0));
  ui->customColorTable->setMaximumHeight(ui->customColorTable->rowHeight(0));

}

CCustomiseCol::~CCustomiseCol()
{
    delete ui;
}

QVector<Qt::CheckState> CCustomiseCol::checkStatesf(){
    for (int column=0; column<ui->customColorTable->columnCount()-1; column++){
      checkStates[column]=ui->customColorTable->item(0,column)->checkState();
    }
  return checkStates;
}

QList <QColor> CCustomiseCol::setColorf(){
  QList <QColor> colors;
  for(int column=0; column<ui->customColorTable->columnCount()-1; column++){
    colors.append(ui->customColorTable->item(0,column)->background().color());
  }
  return colors;
}


void  CCustomiseCol::getStates(int styleData, QVector <QRgb> varColRgb){
  for (int column=0; column<ui->customColorTable->columnCount()-1; column++){
//      int iii=styleData&1<<column;
    if((styleData&1<<column)!=0)
      ui->customColorTable->item(0,column)->setCheckState(Qt::Checked);
    else
      ui->customColorTable->item(0,column)->setCheckState(Qt::Unchecked);
    checkStates[column]=ui->customColorTable->item(0,column)->checkState();
//    ui->customColorTable->item(0,column)-> setBackgroundColor(QColor(varColRgb[column]));
    ui->customColorTable->item(0,column)-> setBackground(QColor(varColRgb[column]));


  }
}


void CCustomiseCol::on_stdColorTable_cellPressed(int row, int column)
{
    QPalette palette = ui->label->palette();
    setColor=ui->stdColorTable->item(row,column)->background().color();
    palette.setColor(ui->label->foregroundRole(),setColor);
    ui->label->setPalette(palette);
}


void CCustomiseCol::on_buttonBox_clicked(QAbstractButton *button){
  if(ui->buttonBox->buttonRole(button)==QDialogButtonBox::ResetRole){
    for (int column=0; column<ui->customColorTable->columnCount()-1; column++){
      /*  Devo copiare il colore di fondo dalla tabella dei colori standard a quella custom.
       *  Per far questo copio l'intera brush.
      */
      QBrush myBrush=ui->stdColorTable->item(0,column)->background();
      ui->customColorTable->item(0,column)->  setBackground(myBrush);
      ui->customColorTable->item(0,column)->setCheckState(checkStates[column]);
    }
  }
  ui->stdColorTable->clearSelection();
  ui->customColorTable->clearSelection();
}

void CCustomiseCol::on_customColorTable_entered(const QModelIndex) {
  /* può accadere che quando faccio un drop, se lo faccio in fondo alla tabella viene
   * inserita una riga aggiuntiva. Non si vuole assolutamente che ciò accada. Una maniera
   * seria di evitarlo consiste nel modificare l'evento"dropItem della tabella, ma per
   * far questo devo ereditare una versione custom della tabella. Per il momento adotto
   * una tecnica semplificata: manualmente riporto sempre a 1 il numero di colonne.
  */
    ui->stdColorTable->clearSelection();
   ui->customColorTable->setRowCount(1);
}


void CCustomiseCol::on_customColorTable_cellDoubleClicked(int row, int column)
{
    QColorDialog dialog;
    int ret=dialog.exec();
    if(ret==QDialog::Accepted){
      ui->customColorTable->item(0,column)-> setBackground(dialog.currentColor());
      ui->customColorTable->clearSelection();
    }
    /* Devo se faccio un doppio click devo annullare l'effetto del primo dei due click
     * sullo stato del checked:*/
  if(ui->customColorTable->item(row,column)->checkState()==Qt::Checked)
    ui->customColorTable->item(row,column)->setCheckState(Qt::Unchecked);
  else
     ui->customColorTable->item(row,column)->setCheckState(Qt::Checked);
  ui->customColorTable->clearSelection();
}

void CCustomiseCol::on_customColorTable_cellClicked(int row, int column){
    if (column==ui->customColorTable->columnCount()-1)
      return;
    if(ui->customColorTable->item(row,column)->checkState()!=Qt::Checked)
       ui->customColorTable->item(row,column)->setCheckState(Qt::Checked);
    else
       ui->customColorTable->item(row,column)->setCheckState(Qt::Unchecked);
    ui->customColorTable->clearSelection();
}
