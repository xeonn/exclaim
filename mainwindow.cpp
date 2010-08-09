
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "trxwnd.h"
#include "typedlg.h"
#include "settings.h"

#include <QtSql/QSqlTableModel>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>
#include <QSqlQuery>
#include <QSqlResult>
#include <QMessageBox>
#include "helper.hxx"

#include <QPrinter>
#include <QPrintDialog>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextTable>
#include <QTextTableCell>
#include <QTextTableCellFormat>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    updateUI();

    QObject::connect(ui->tblMain, SIGNAL(clicked( QModelIndex ) ),
                     this, SLOT(updateUI()));
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

// Private slot
void MainWindow::updateUI()
{
    ui->bnPrintSubmission->setEnabled(ui->tblMain->currentIndex().isValid());
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
        pm->setQuery("select c_submission_id,datesubmit,name,advance from c_submission order by datesubmit desc");

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

void MainWindow::on_bnSettings_clicked()
{
    Settings dlg(this);
    dlg.exec();
}

void MainWindow::generateDocument(QTextCursor * cursor)
{ 
    printHeader(cursor);
    printBody(cursor);
    printFooter(cursor);
}

void MainWindow::printHeader(QTextCursor * cursor)
{
    QTextCharFormat normalFormat = cursor->blockCharFormat();
    QTextCharFormat fontFormat;
    fontFormat.setFontPointSize(24);
    fontFormat.setFontWeight(QFont::Bold);

    QModelIndex index(ui->tblMain->currentIndex());
    cursor->setBlockCharFormat(fontFormat);
    cursor->insertText("Expenses Claim Report\n\n");
    cursor->setBlockCharFormat(normalFormat);
    cursor->insertText(QString("Submission date :") + index.sibling(index.row(),1).data().toString() + "\n" +
                       "\n" + index.sibling(index.row(),2).data().toString() +
                       "\n\n");

    QSqlQuery qry;
    if(qry.exec("select name, company from c_userinfo") && qry.next())
    {
        cursor->insertText("Name\t: " + qry.record().value("name").toString() + "\n");
        cursor->insertText("Company\t: " +qry.record().value("company").toString() + "\n");
    }
}

void MainWindow::printBody(QTextCursor *cursor)
{
    QSqlTableModel * pModel = static_cast<QSqlTableModel*>(ui->tblMain->model());
    QString sSubmissionID = pModel->record(ui->tblMain->currentIndex().row()).value("c_submission_id").toString();

    QString sql = "SELECT datetrx, description, amount, name FROM c_trx join c_trxtype on c_trx.c_trxtype_id = c_trxtype.c_trxtype_id WHERE c_submission_id = \'"
        + sSubmissionID + "\';";
    QString tab = "\t\t\t";

    QSqlQuery qry;
    if(!qry.exec(sql))
    {
        QMessageBox bx;
        bx.setText(qry.lastError().text());
        bx.exec();
    }

    QTextTableFormat tableFormat;
         tableFormat.setAlignment(Qt::AlignLeft);
         tableFormat.setCellPadding(1);
         tableFormat.setCellSpacing(1);

     QVector<QTextLength> constraints;
     constraints << QTextLength(QTextLength::FixedLength, 40)
                      << QTextLength(QTextLength::FixedLength, 80)
                      << QTextLength(QTextLength::FixedLength, 130)
                      << QTextLength(QTextLength::FixedLength, 240)
                      << QTextLength(QTextLength::FixedLength, 90);
          tableFormat.setColumnWidthConstraints(constraints);

    cursor->insertText("\n\n\n");
    cursor->insertTable(1,5,tableFormat);
    int row=0;
    int colNo = 0;
    int colDate = 1;
    int colType = 2;
    int colDesc = 3;
    int colAmt = 4;

    // Column header
    cursor->currentTable()->begin();

    QBrush brush(Qt::blue);

    QTextBlockFormat headerFormat;
    headerFormat.setBackground(brush);

    QTextCharFormat fontFormat;
    fontFormat.setFontPointSize(12);
    fontFormat.setFontWeight(QFont::Bold);
    fontFormat.setForeground(QBrush(Qt::white));

    cursor->currentTable()->cellAt(row,colNo).begin();
    cursor->currentTable()->cellAt(row,colNo).firstCursorPosition().setBlockFormat(headerFormat);
    cursor->currentTable()->cellAt(row,colNo).firstCursorPosition().setBlockCharFormat(fontFormat);
    cursor->currentTable()->cellAt(row,colNo).lastCursorPosition().insertText("No.");
    cursor->currentTable()->cellAt(row,colNo).end();

    cursor->currentTable()->cellAt(row,colDate).begin();
    cursor->currentTable()->cellAt(row,colDate).firstCursorPosition().setBlockFormat(headerFormat);
    cursor->currentTable()->cellAt(row,colDate).firstCursorPosition().setBlockCharFormat(fontFormat);
    cursor->currentTable()->cellAt(row,colDate).lastCursorPosition().insertText("Date");
    cursor->currentTable()->cellAt(row,colDate).end();

    cursor->currentTable()->cellAt(row,colType).begin();
    cursor->currentTable()->cellAt(row,colType).firstCursorPosition().setBlockFormat(headerFormat);
    cursor->currentTable()->cellAt(row,colType).firstCursorPosition().setBlockCharFormat(fontFormat);
    cursor->currentTable()->cellAt(row,colType).lastCursorPosition().insertText("Type");
    cursor->currentTable()->cellAt(row,colType).end();

    cursor->currentTable()->cellAt(row,colDesc).begin();
    cursor->currentTable()->cellAt(row,colDesc).firstCursorPosition().setBlockFormat(headerFormat);
    cursor->currentTable()->cellAt(row,colDesc).firstCursorPosition().setBlockCharFormat(fontFormat);
    cursor->currentTable()->cellAt(row,colDesc).lastCursorPosition().insertText("Description");
    cursor->currentTable()->cellAt(row,colDesc).end();

    headerFormat.setAlignment(Qt::AlignRight);

    cursor->currentTable()->cellAt(row,colAmt).begin();
    cursor->currentTable()->cellAt(row,colAmt).firstCursorPosition().setBlockFormat(headerFormat);
    cursor->currentTable()->cellAt(row,colAmt).firstCursorPosition().setBlockCharFormat(fontFormat);
    cursor->currentTable()->cellAt(row,colAmt).lastCursorPosition().insertText("Amount");
    cursor->currentTable()->cellAt(row,colAmt).end();

    // put in the details

    QTextBlockFormat rightAlignment;
    rightAlignment.setAlignment(Qt::AlignRight);

    while( qry.next() )
    {
        ++row;

        // Format date
        QDate datetrx = qry.value(0).toDate();
        
        // Format currency
        QLocale locale(QLocale::English, QLocale::UnitedStates);
        QString amount;

        bool success = false;
        double dbl = qry.value(2).toDouble(&success);
        amount = locale.toString(dbl,'f',2);

        cursor->currentTable()->appendRows(1);
        cursor->currentTable()->cellAt(row,colNo).begin();
        cursor->currentTable()->cellAt(row,colNo).firstCursorPosition().insertText(locale.toString(row));
        cursor->currentTable()->cellAt(row,colNo).end();

        cursor->currentTable()->cellAt(row,colDate).begin();
        cursor->currentTable()->cellAt(row,colDate).firstCursorPosition().insertText(datetrx.toString("dd-MM-yyyy"));
        cursor->currentTable()->cellAt(row,colDate).end();

        cursor->currentTable()->cellAt(row,colType).begin();
        cursor->currentTable()->cellAt(row,colType).firstCursorPosition().insertText(qry.value(3).toString());
        cursor->currentTable()->cellAt(row,colType).end();

        cursor->currentTable()->cellAt(row,colDesc).begin();
        cursor->currentTable()->cellAt(row,colDesc).firstCursorPosition().insertText(qry.value(1).toString());
        cursor->currentTable()->cellAt(row,colDesc).end();

        cursor->currentTable()->cellAt(row,colAmt).begin();
        cursor->currentTable()->cellAt(row,colAmt).firstCursorPosition().setBlockFormat(rightAlignment);
        cursor->currentTable()->cellAt(row,colAmt).lastCursorPosition().insertText( amount);
        cursor->currentTable()->cellAt(row,colAmt).end();
    }
    cursor->currentTable()->end();
    cursor->movePosition(QTextCursor::End);
}

void MainWindow::printFooter(QTextCursor *cursor)
{
    QModelIndex index(ui->tblMain->currentIndex());

    QTextTableFormat tableFormat;
    tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_None);
    tableFormat.setAlignment(Qt::AlignLeft);
         tableFormat.setCellPadding(4);
         tableFormat.setCellSpacing(4);

     QVector<QTextLength> constraints;
     constraints << QTextLength(QTextLength::FixedLength, 50)
                      << QTextLength(QTextLength::FixedLength, 120)
                      << QTextLength(QTextLength::FixedLength, 240)
                      << QTextLength(QTextLength::FixedLength, 120);
      tableFormat.setColumnWidthConstraints(constraints);

      QBrush brush(Qt::red);
      QTextBlockFormat negFormat;
      negFormat.setAlignment(Qt::AlignRight);
      negFormat.setForeground(brush);

     cursor->insertTable(1,4,tableFormat);

     cursor->currentTable()->begin();

     // Total amount - advance
     QSqlTableModel * pModel = static_cast<QSqlTableModel*>(ui->tblMain->model());
     QString sSubmissionID = pModel->record(ui->tblMain->currentIndex().row()).value("c_submission_id").toString();

     QString sql = "SELECT sum(amount) FROM c_trx WHERE c_submission_id = \'" + sSubmissionID + "\';";

     QSqlQuery qry;
     if(!qry.exec(sql))
     {
         QMessageBox bx;
         bx.setText(qry.lastError().text());
         bx.exec();
     }
     qry.next();
     bool success = false;
     double total = qry.value(0).toDouble(&success);
     double advance = index.sibling(index.row(),3).data().toDouble(&success);
    qry.clear();
     QLocale locale(QLocale::English, QLocale::UnitedStates);
     QString amount;

     cursor->currentTable()->cellAt(0,2).begin();
     cursor->currentTable()->cellAt(0,2).firstCursorPosition().insertText("Gross Total");
     cursor->currentTable()->cellAt(0,2).end();

     amount = locale.toString(total,'f',2);
     cursor->currentTable()->cellAt(0,3).begin();
     cursor->currentTable()->cellAt(0,3).firstCursorPosition().setBlockFormat(negFormat);
     cursor->currentTable()->cellAt(0,3).lastCursorPosition().insertText(amount);
     cursor->currentTable()->cellAt(0,3).end();

     cursor->currentTable()->appendRows(1);

     cursor->currentTable()->cellAt(1,2).begin();
     cursor->currentTable()->cellAt(1,2).firstCursorPosition().insertText("Less : Advance :");
     cursor->currentTable()->cellAt(1,2).end();

     cursor->currentTable()->cellAt(1,3).begin();
     cursor->currentTable()->cellAt(1,3).firstCursorPosition().setBlockFormat(negFormat);
     cursor->currentTable()->cellAt(1,3).lastCursorPosition().insertText("(" + index.sibling(index.row(),3).data(Qt::DisplayRole).toString() + ")");
     cursor->currentTable()->cellAt(1,3).end();

     total -= advance;
amount = locale.toString(total,'f',2);
     cursor->currentTable()->appendRows(1);

     QTextCharFormat fontFormat;
     fontFormat.setFontWeight(QFont::Bold);

     cursor->currentTable()->cellAt(2,2).begin();
     cursor->currentTable()->cellAt(2,2).firstCursorPosition().setBlockCharFormat(fontFormat);
     cursor->currentTable()->cellAt(2,2).lastCursorPosition().insertText("Net Claim");
     cursor->currentTable()->cellAt(2,2).end();

     cursor->currentTable()->cellAt(2,3).begin();
     cursor->currentTable()->cellAt(2,3).firstCursorPosition().setBlockFormat(negFormat);
     cursor->currentTable()->cellAt(2,3).firstCursorPosition().setBlockCharFormat(fontFormat);
     cursor->currentTable()->cellAt(2,3).lastCursorPosition().insertText(amount);
     cursor->currentTable()->cellAt(2,3).end();

     cursor->currentTable()->end();

     // Print submitted by, approved by etc...
     cursor->movePosition(QTextCursor::End);
     cursor->insertText("\n\n\n\n\n");

     QTextTableFormat signFormat;
     signFormat.setBorderStyle(QTextFrameFormat::BorderStyle_None);
     signFormat.setAlignment(Qt::AlignLeft);
    signFormat.setCellPadding(4);
    signFormat.setCellSpacing(4);

    QVector<QTextLength> signConstraints;
    signConstraints << QTextLength(QTextLength::FixedLength, 340)
               << QTextLength(QTextLength::FixedLength, 340);
    signFormat.setColumnWidthConstraints(signConstraints);

    /*QString*/ sql = "SELECT name,jobtitle,department FROM c_userinfo";


    if(!qry.exec(sql))
    {
        QMessageBox bx;
        bx.setText(qry.lastError().text());
        bx.exec();
    }
    qry.next();
     cursor->insertTable(1,2,signFormat);
     cursor->currentTable()->cellAt(0,0).firstCursorPosition().insertText("Prepared by:");
     cursor->currentTable()->cellAt(0,1).firstCursorPosition().insertText("Approved by:");
     cursor->currentTable()->appendRows(4);

     cursor->currentTable()->cellAt(3,0).firstCursorPosition().insertText(qry.value(0).toString() + "\n" +
                                                                          "Position\t: " + qry.value(1).toString() + "\n" +
                                                                          "Department\t: " + qry.value(2).toString());

     cursor->currentTable()->cellAt(3,1).firstCursorPosition().insertText("Head of Department\n" +
                                                                          qry.value(2).toString());

     cursor->currentTable()->end();
}

void MainWindow::on_bnPrintSubmission_clicked()
{
    QPrinter printer;
    QTextDocument * document = new QTextDocument(this);
    QTextCursor * cursor = new QTextCursor(document);
    generateDocument(cursor);

     QPrintDialog *dialog = new QPrintDialog(&printer, this);
     dialog->setWindowTitle(tr("Print Document"));
     if (dialog->exec() == QDialog::Accepted)
     {
         document->print(&printer);
     }
}
