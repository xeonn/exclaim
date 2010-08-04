#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>

#include <QStyledItemDelegate>
#include <QDateEdit>

#include <QtSql/QSqlDatabase>

#include <map>

typedef std::map<QString,QWidget*> MapTrx;


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void initData();

signals:
//    void dataUpdated();


public slots:
    void updateList();
    void onCloseTrxWnd(QString strTrxID);

protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;
    MapTrx          mMapTrx;

    QSqlDatabase mCnn;

private slots:
    void on_bnNewSubmission_clicked();
    void on_bnDelSubmission_clicked();
    void on_tblMain_doubleClicked(QModelIndex index);
    void on_bnClose_clicked();
    void on_bnExpType_clicked();
};

#endif // MAINWINDOW_H
