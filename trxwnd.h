#ifndef TRXWND_H
#define TRXWND_H

#include <QMainWindow>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlRelationalTableModel>
#include <QDataWidgetMapper>

#include "helper.hxx"

namespace Ui {
    class TrxWnd;
}


class TrxWnd : public QMainWindow
{
Q_OBJECT
public:
    explicit TrxWnd(QWidget *parent, QString submit_ide, QSqlDatabase * pCnn);
    ~TrxWnd();

signals:
    void dataUpdated();
    void closedTrxWnd(QString strTrxID);

protected:
    void changeEvent(QEvent *e);


private:
    Ui::TrxWnd *ui;
    QString mstrSubmitID;

    ModelFormatter<QSqlRelationalTableModel>* mpModelSubmit;
    ModelFormatter<QSqlRelationalTableModel>* mpModelTrx;
    QDataWidgetMapper * mpTrxMapper;
    QDataWidgetMapper * mpSubmitMapper;
    QSqlDatabase *   mpCnn;       // Need own connection to implement transactions.

    void initRecords();
    void initTrxRecords();
    void initViews();

    QSqlDatabase& getConnection();
    const QSqlDatabase& getConnection() const;
    void clearConnection();
    bool saveRecord();


private slots:
    void on_bnDelete_clicked();
    void on_bnAdd_clicked();
    void on_bnCancelHeader_clicked();
    void on_bnSaveHeader_clicked();

    void calculateTotal();
};

#endif // TRXWND_H
