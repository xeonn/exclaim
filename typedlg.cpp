#include "typedlg.h"
#include "ui_typedlg.h"

#include <QtSql/QSqlTableModel>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlQuery>
#include <QMessageBox>

TypeDlg::TypeDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TypeDlg)
{
    ui->setupUi(this);

    QSqlTableModel * model = new QSqlTableModel(this);
    model->setTable("c_trxtype");
    model->setEditStrategy(QSqlTableModel::OnRowChange);

    model->setHeaderData(0, Qt::Horizontal, "ID");
    model->setHeaderData(1, Qt::Horizontal, "Name");
    model->setHeaderData(2, Qt::Horizontal, "Notes");
    ui->tableView->setModel(model);

    initView();
}

void TypeDlg::initView()
{
    QSqlTableModel * pModel = static_cast<QSqlTableModel*>(ui->tableView->model());
    pModel->select();

    ui->tableView->hideColumn(0);
    ui->tableView->resizeColumnsToContents();
}

TypeDlg::~TypeDlg()
{
    delete ui;
}

void TypeDlg::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void TypeDlg::on_bnClose_clicked()
{
    this->close();
}

void TypeDlg::on_bnNew_clicked()
{
    QSqlTableModel * pModel = (QSqlTableModel *)ui->tableView->model();

    QSqlRecord rc = pModel->record(pModel->rowCount()-1);

    if( !rc.value("c_trxtype_id").toString().isEmpty() )
        pModel->insertRow(pModel->rowCount());
}

void TypeDlg::on_bnDelete_clicked()
{
    QSqlTableModel * pModel = static_cast<QSqlTableModel *>(ui->tableView->model());

    QString sql = "delete from c_trxtype where c_trxtype_id = \'";
    sql += pModel->record(ui->tableView->currentIndex().row()).value("c_trxtype_id").toString();
    sql += "\'";

    QSqlQuery qry;
    if(!qry.exec(sql))
    {
        QMessageBox bx;
        bx.setText(qry.lastError().text());
        bx.exec();
    }
    initView();
}
