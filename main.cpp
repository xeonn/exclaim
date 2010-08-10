#include <QtGui/QApplication>
#include <QModelIndex>
#include "mainwindow.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QMessageBox>

#include "settings.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    w.initData();

    // Check for existence of user information. Cannot be blank.

    QSqlQuery qry;
    qry.exec("select name, jobtitle, department from c_userinfo");
    qry.next();
    if( qry.record().value("name").isNull() )
    {
        QMessageBox bx;
        bx.setText("Your Name, Job Title information is missing.\nPlease update.");
        bx.exec();

        Settings dlg;
        if(!dlg.exec())
            return 0;
    }

    w.show();
    return a.exec();
}
