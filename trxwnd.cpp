#include "trxwnd.h"
#include "ui_trxwnd.h"

#include <QUuid>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRelationalDelegate>
#include <QFileDialog>
#include <QMessageBox>
#include "helper.hxx"

TrxWnd::TrxWnd(QWidget *parent, QString submit_id, QSqlDatabase * pCnn) :
    QMainWindow(parent),
    ui(new Ui::TrxWnd),
    mstrSubmitID(submit_id)
{
    ui->setupUi(this);

    mpCnn = pCnn;

    // This code results to dangling pointer at mainwindow
    setAttribute(Qt::WA_DeleteOnClose);

    initRecords();
    initViews();
    calculateTotal();
}

TrxWnd::~TrxWnd()
{
    emit closedTrxWnd(mstrSubmitID);
    delete ui;
}

void TrxWnd::changeEvent(QEvent *e)
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

QSqlDatabase& TrxWnd::getConnection()
{
    return *mpCnn;
}

const QSqlDatabase& TrxWnd::getConnection() const
{
    return *mpCnn;
}

void TrxWnd::clearConnection()
{
    QItemSelectionModel *pm = ui->tableView->selectionModel();
    ui->tableView->setModel(0);
    delete pm;

    mpModelSubmit->clear();
    mpModelTrx->clear();
    mpSubmitMapper->clearMapping();
    mpTrxMapper->clearMapping();
}

void TrxWnd::initRecords()
{
    mpModelSubmit = new ModelFormatter<QSqlRelationalTableModel>(this, getConnection());
    mpModelTrx = new ModelFormatter<QSqlRelationalTableModel>(this,getConnection());

    mpModelSubmit->setTable("c_submission");
    mpModelSubmit->setEditStrategy(QSqlTableModel::OnManualSubmit);

    if( !mstrSubmitID.isEmpty() )
    {
        QString filter = QString("c_submission_id=\'") + mstrSubmitID + "\'";

        // Init parent record
        mpModelSubmit->setFilter(filter);
        mpModelSubmit->select();
    }
    initTrxRecords();
}

void TrxWnd::initTrxRecords()
{
    mpModelTrx->setTable("c_trx");
    mpModelTrx->setHeaderData(0,Qt::Horizontal,"ID");
    mpModelTrx->setHeaderData(1,Qt::Horizontal,"Date");
    mpModelTrx->setHeaderData(2,Qt::Horizontal,"Description");
    mpModelTrx->setHeaderData(3,Qt::Horizontal,"Type");
    mpModelTrx->setHeaderData(4,Qt::Horizontal,"Amount");
    mpModelTrx->setHeaderData(5,Qt::Horizontal,"Submit ID");

    mpModelTrx->setRelation(3,QSqlRelation("c_trxtype", "c_trxtype_id", "name"));
    mpModelTrx->setEditStrategy(QSqlTableModel::OnManualSubmit);

    if( !mstrSubmitID.isEmpty() )
    {
        QString filter = QString("c_submission_id=\'") + mstrSubmitID + "\'";

        mpModelTrx->setFilter(filter);
        mpModelTrx->select();
    }
}

void TrxWnd::initViews()
{
    ui->tableView->setModel(mpModelTrx);
    ui->tableView->hideColumn(0);
    ui->tableView->hideColumn(5);
    ui->tableView->setColumnWidth(2,250);

    ui->tableView->setItemDelegate(new QSqlRelationalDelegate(ui->tableView));

    // Setting Date format
    DateStyledDelegate * pDateEditor = new DateStyledDelegate(this);
    ui->tableView->setItemDelegateForColumn(1,(QAbstractItemDelegate*)pDateEditor);

    NumericStyledDelegate * pNumEditor = new NumericStyledDelegate(this);
    ui->tableView->setItemDelegateForColumn(mpModelTrx->fieldIndex("amount"),(QAbstractItemDelegate*)pNumEditor);

    // Expenses type
    ui->cTrxType->setModel(mpModelTrx->relationModel(3));
    ui->cTrxType->setModelColumn(mpModelTrx->relationModel(3)->fieldIndex("name"));

    // Bind widget to Submit record
    mpSubmitMapper = new QDataWidgetMapper(this);
    mpSubmitMapper->setModel(mpModelSubmit);
    mpSubmitMapper->addMapping(ui->cSubmitDate, mpModelSubmit->fieldIndex("datesubmit"));
    mpSubmitMapper->addMapping(ui->cSubmitTitle, mpModelSubmit->fieldIndex("name"));
    mpSubmitMapper->addMapping(ui->cAdvance, mpModelSubmit->fieldIndex("advance"));
    mpSubmitMapper->toFirst();

    // Bind widget to trx record
    mpTrxMapper = new QDataWidgetMapper(this);
    mpTrxMapper->setModel(mpModelTrx);
    mpTrxMapper->setItemDelegate(new QSqlRelationalDelegate(mpTrxMapper));
    mpTrxMapper->addMapping(ui->cTrxDate, mpModelTrx->fieldIndex("datetrx"));
    mpTrxMapper->addMapping(ui->cTrxDesc,2);
    mpTrxMapper->addMapping(ui->cTrxType,3);
    mpTrxMapper->addMapping(ui->cTrxAmount,4);
    mpTrxMapper->toFirst();

    QObject::connect(ui->cAdvance, SIGNAL(textChanged (const QString &)),
                     this, SLOT(calculateTotal()));
    QObject::connect(ui->cTrxAmount, SIGNAL(textChanged(const QString &)),
                     this, SLOT(calculateTotal()));
}

void TrxWnd::calculateTotal()
{
    double dbl = 0.0;
    bool success = false;

    // Calculate gross total
    int col=mpModelTrx->fieldIndex("amount");

    for (int row = 0;row < mpModelTrx->rowCount(); ++row)
        dbl += mpModelTrx->data(mpModelTrx->index(row,col),Qt::EditRole).toDouble(&success);

    ui->cSubTotal->setText(QLocale().toString(dbl,'f',2));

    // Minus Advance
    dbl -= ui->cAdvance->text().toDouble(&success);

    // Net claim
    ui->cNetClaim->setText(QLocale().toString(dbl,'f',2));
}

bool TrxWnd::saveRecord()
{
    // Adding new record
    if(mstrSubmitID.isEmpty())
    {
        // Generate UUID for submission
        QVariant newSubmitID(QUuid::createUuid().toString());

        QSqlRecord rec = mpModelSubmit->record();
        rec.setValue("name",QVariant(ui->cSubmitTitle->text()));
        rec.setValue("datesubmit",QVariant(ui->cSubmitDate->date()));
        rec.setValue("c_submission_id",newSubmitID);
        rec.setValue("advance", QVariant(QLocale().toDouble(ui->cAdvance->text(), 0)));

        if (!mpModelSubmit->insertRecord(-1, rec)) return false;
        if (!mpModelSubmit->submitAll()) return false;

        // Save the submit ID. This is no longer a new record
        mstrSubmitID = newSubmitID.toString();
        initRecords();
        initViews();
    }
    else
    {
        mpSubmitMapper->submit();
        if(!mpModelSubmit->submitAll()) return false;

        mpTrxMapper->submit();
        if(!mpModelTrx->submitAll()) return false;
    }
    calculateTotal();
    return true;
}

void TrxWnd::on_bnSaveHeader_clicked()
{
    if(!saveRecord())
    {
        QMessageBox box;
        box.setText(mpModelSubmit->lastError().text());
        box.exec();
    }
    emit dataUpdated();
}

void TrxWnd::on_bnCancelHeader_clicked()
{
    clearConnection();
    this->close();
}

void TrxWnd::on_bnAdd_clicked()
{
    QVariant var = mstrSubmitID;
    int row = mpModelTrx->rowCount();

    if(!mpModelTrx->insertRow(row))
    {
        QMessageBox box;
        box.setText(mpModelTrx->lastError().text());
        box.exec();
    }

    mpModelTrx->setData(mpModelTrx->index(row,mpModelTrx->record(row).indexOf("c_submission_id")),var);
    mpTrxMapper->setCurrentIndex(row);
}

void TrxWnd::on_bnDelete_clicked()
{
    QString title = mpModelSubmit->record(ui->tableView->currentIndex().row()).value("name").toString();
    QString msg = "This will permanently remove ";
    msg += title + "\nAre you really sure you want to do this?";
    QMessageBox box;
    box.addButton("OK",QMessageBox::AcceptRole);
    box.addButton("Cancel", QMessageBox::RejectRole);
    box.setText(msg);
    if(box.exec() == QMessageBox::AcceptRole)
    {
       QSqlTableModel * pModel = static_cast<QSqlTableModel*>(mpModelTrx);

        QVariant var = pModel->record(ui->tableView->currentIndex().row()).value("c_trx_id");
        QString sql = "delete from c_trx where c_trx_id = \'";
        sql += var.toString() + "\'";

        QSqlQuery qry;
        if(!qry.exec(sql))
        {
            QMessageBox bx;
            bx.setText(qry.lastError().text());
        }

        initTrxRecords();
        calculateTotal();
        emit dataUpdated();
    }

}
