#ifndef CCUSTOMISECOL_H
#define CCUSTOMISECOL_H

#include <QDialog>
#include <qcolordialog.h>
#include <qtablewidget.h>
#include <QButtonGroup>

namespace Ui {
class CCustomiseCol;
}

class CCustomiseCol : public QDialog
{
    Q_OBJECT

public:
    explicit CCustomiseCol(QWidget *parent = nullptr);
    ~CCustomiseCol();
    void getStates(int styleData, QVector<QRgb> varColRgb);
    QList<QColor> setColorf();
    QVector<Qt::CheckState> checkStatesf();


private slots:
    void on_stdColorTable_cellPressed(int row, int column);
    void on_buttonBox_clicked(QAbstractButton *button);
    void on_customColorTable_entered(const QModelIndex);
    void on_customColorTable_cellDoubleClicked(int row, int column);
    void on_customColorTable_cellClicked(int row, int column);

private:
    QVector <Qt::CheckState> checkStates;
    Ui::CCustomiseCol *ui;
    void resizeEvent(QResizeEvent *);
    QColor setColor;
};

#endif // CCUSTOMISECOL_H
