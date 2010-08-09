#ifndef ABOUTDLG_H
#define ABOUTDLG_H

#include <QDialog>

namespace Ui {
    class AboutDlg;
}

class AboutDlg : public QDialog {
    Q_OBJECT
public:
    AboutDlg(QWidget *parent = 0);
    ~AboutDlg();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::AboutDlg *ui;

private slots:
    void on_bnOK_clicked();
};

#endif // ABOUTDLG_H
