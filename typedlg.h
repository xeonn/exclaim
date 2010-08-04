#ifndef TYPEDLG_H
#define TYPEDLG_H

#include <QDialog>

namespace Ui {
    class TypeDlg;
}

class TypeDlg : public QDialog {
    Q_OBJECT
public:
    TypeDlg(QWidget *parent = 0);
    ~TypeDlg();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::TypeDlg *ui;
    void initView();

private slots:
    void on_bnDelete_clicked();
    void on_bnNew_clicked();
    void on_bnClose_clicked();
};

#endif // TYPEDLG_H
