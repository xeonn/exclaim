#include <QSqlTableModel>
#include <QSqlError>
#include <QDataWidgetMapper>
#include <QMessageBox>

#include "settings.h"
#include "ui_settings.h"

Settings::Settings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settings)
{
    ui->setupUi(this);

    mpModel = new QSqlTableModel(this);

    readData();
}

Settings::~Settings()
{
    delete ui;
}

void Settings::changeEvent(QEvent *e)
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

void Settings::on_buttonBox_accepted()
{
    if(saveData())
        close();
}

/** Read user information from database. There should be only 1, and already exist.*/
void Settings::readData()
{
    mpModel->setTable("c_userinfo");
    mpModel->setEditStrategy(QSqlTableModel::OnManualSubmit);

    if(!mpModel->select())
    {
        QMessageBox bx;
        bx.setText(mpModel->lastError().text());
        bx.exec();
    }

    // Bind them to field
    QDataWidgetMapper * pBinder = new QDataWidgetMapper(this);
    pBinder->setModel(mpModel);
    pBinder->addMapping(ui->cName, mpModel->fieldIndex("name"));
    pBinder->addMapping(ui->cCompany, mpModel->fieldIndex("company"));
    pBinder->addMapping(ui->cAddrLine1, mpModel->fieldIndex("address_1"));
    pBinder->addMapping(ui->cAddrLine2, mpModel->fieldIndex("address_2"));
    pBinder->addMapping(ui->cPostcode,  mpModel->fieldIndex("postcode"));
    pBinder->addMapping(ui->cState,     mpModel->fieldIndex("state"));
    pBinder->addMapping(ui->cCountry,   mpModel->fieldIndex("country"));
    pBinder->addMapping(ui->cState,     mpModel->fieldIndex("state"));
    pBinder->addMapping(ui->cJobTitle,  mpModel->fieldIndex("jobtitle"));
    pBinder->addMapping(ui->cDepartment,    mpModel->fieldIndex("department"));
    pBinder->toFirst();
}

bool Settings::saveData()
{
    if(!mpModel->submitAll())
    {
        qWarning(mpModel->lastError().text().toStdString().c_str());
        QMessageBox bx;
        bx.setText(mpModel->lastError().text());
        bx.exec();
        return false;
    }
    return true;
}
