
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "trxwnd.h"
#include "typedlg.h"

#include <QtSql/QSqlTableModel>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>
#include <QSqlQuery>
#include <QSqlResult>
#include <QMessageBox>
#include "helper.hxx"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

void MainWindow::initData()
{
    mCnn = QSqlDatabase::addDatabase("QPSQL");
    mCnn.setHostName("localhost");
    mCnn.setDatabaseName("exclaim");
    mCnn.setUserName("exclaim");
    mCnn.setPassword("exclaim");

    updateList();
}

MainWindow::~MainWindow()
{
    mCnn.close();
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::on_bnExpType_clicked()
{
    TypeDlg dlg(this);
    dlg.setModal(true);
    dlg.exec();
}

void MainWindow::on_bnClose_clicked()
{
    this->close();
}

#include <algorithm>

// Opens a new transaction window upon selection by user. Else, if window already shown, activate the window
void MainWindow::on_tblMain_doubleClicked(QModelIndex index)
{
    // Executed when user edit existing record.

    QString sSubmitID;
    QSqlTableModel * pModel = (QSqlTableModel*)ui->tblMain->model();
    QVariant var = pModel->record(index.row()).value("c_submission_id");

    if( !var.isNull() )
    {
        sSubmitID = var.toString();

        MapTrx::iterator iter = mMapTrx.begin();

        for ( ; iter != mMapTrx.end(); ++iter )
        {
            if( iter->first == sSubmitID )
            {
                TrxWnd * xwnd = dynamic_cast<TrxWnd*>(iter->second);

                if( xwnd != 0 )
                {
                    xwnd->show();
                    xwnd->activateWindow();
                    xwnd->raise();
                }
                else
                {
                    mMapTrx.erase(iter);
                    break;
                }
                return;
            }
        }

        // Editing existing window
        TrxWnd * wnd = new TrxWnd(this, sSubmitID, &mCnn);

        // Connector to update main window record
        QObject::connect(wnd, SIGNAL(dataUpdated()),
                     this, SLOT(updateList()));

        wnd->connect(wnd, SIGNAL(closedTrxWnd(QString)),
                     this, SLOT(onCloseTrxWnd(QString)));

        wnd->show();

        mMapTrx[sSubmitID] = wnd;
    }
}

// Slots called when transaction window is closed
void MainWindow::onCloseTrxWnd(QString strTrxID)
{
    MapTrx::iterator iter = mMapTrx.begin();

    for ( ; iter != mMapTrx.end(); ++iter )
    {
        if( iter->first == strTrxID )
        {
                mMapTrx.erase(iter);
                break;
        }
    }
}

void MainWindow::on_bnDelSubmission_clicked()
{
    //TODO: Add confirmation dialog before delete

    QSqlTableModel * pModel = static_cast<QSqlTableModel*>(ui->tblMain->model());
    QString str = pModel->record(ui->tblMain->currentIndex().row()).value("c_submission_id").toString();

    QString sql = "DELETE FROM c_submission WHERE c_submission_id = \'"
        + str
        + "\';";

    QSqlQuery qry;
    if(!qry.exec(sql))
    {
        QMessageBox bx;
        bx.setText(qry.lastError().text());
        bx.exec();
    }

    updateList();
}

void MainWindow::on_bnNewSubmission_clicked()
{
    TrxWnd * wnd = new TrxWnd(this, QString(""), &mCnn);

    // Connector to update main window record
    QObject::connect(wnd, SIGNAL(dataUpdated()),
                 this, SLOT(updateList()));

    wnd->connect(wnd, SIGNAL(closedTrxWnd(QString)),
                 this, SLOT(onCloseTrxWnd(QString)));

    wnd->show();
}

// Public slot to update database list
void MainWindow::updateList()
{

    if(mCnn.open())
    {
        ModelFormatter<QSqlQueryModel> *pm = dynamic_cast< ModelFormatter<QSqlQueryModel> *> (ui->tblMain->model());

        if ( pm != 0 )
            pm->setQuery("");
        else
            pm = new ModelFormatter<QSqlQueryModel>(this);

        ui->tblMain->reset();
        ui->tblMain->setModel(pm);
        pm->setQuery("select c_submission_id,datesubmit,name from c_submission order by datesubmit desc");

        pm->setHeaderData(0, Qt::Horizontal, "ID");
        pm->setHeaderData(1, Qt::Horizontal, "Date");
        pm->setHeaderData(2, Qt::Horizontal, "Title");

        ui->tblMain->hideColumn(0);
        DateStyledDelegate * pDateEdit = new DateStyledDelegate(this);

        ui->tblMain->setItemDelegateForColumn(1,(QAbstractItemDelegate*) pDateEdit);
        ui->tblMain->setColumnWidth(2, ui->tblMain->columnWidth(2) * 2.5);
    }

    else
    {
        QMessageBox bx;
        bx.setText(mCnn.lastError().text());
        bx.exec();
    }
}
