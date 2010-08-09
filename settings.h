#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>

namespace Ui {
    class Settings;
}

// Forward declaration
class QSqlTableModel;

class Settings : public QDialog {
    Q_OBJECT
public:
    Settings(QWidget *parent = 0);
    ~Settings();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::Settings *ui;
    QSqlTableModel * mpModel;

    void readData();
    bool saveData();

private slots:
    void on_buttonBox_accepted();
};

#endif // SETTINGS_H
