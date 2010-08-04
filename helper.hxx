#ifndef HELPER_HXX
#define HELPER_HXX

#include <iostream>

#include <cassert>
#include <QStyledItemDelegate>
#include <QDate>
#include <QDateEdit>
#include <QLineEdit>
#include <QSqlDatabase>
#include <QSqlQueryModel>
/*
  Model formatter for use with QAbstractItemModel derivatives.
 */
template < typename T>
        class ModelFormatter : public T
{
public:
    explicit ModelFormatter(QObject *parent = 0)
        :T(parent)
    {
    }
    explicit ModelFormatter(QObject *parent, QSqlDatabase db)
        :T(parent,db)
    {
    }

    virtual ~ModelFormatter()
    {

    }

    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const
    {
        QVariant val = T::data(item,role);

        if ( role == Qt::DisplayRole )
        {
            switch (val.type()) {
            case QVariant::DateTime:
            case QVariant::Date:
                {
                    QDate date = val.toDate();
                    QVariant str = date.toString("dd-MM-yyyy");
                    return str;
                }
            case QVariant::Double:
                {
                    QLocale locale(QLocale::English, QLocale::UnitedStates);
                    QString result;
                    bool success = false;
                    result = locale.toString(val.toDouble(&success),'f',2);
                    if (success) return QVariant(result);
                    //QString::number()
                    else return val;
                }
            default:
                ;
            }
        }
        
        if ( item.isValid() && role == Qt::TextAlignmentRole )
        {
            // Double is stored as string, so we will make a test using QString
            bool success = false;
            QLocale locale(QLocale::English, QLocale::UnitedStates);
            locale.toDouble(item.data().toString(),&success);
            if ( success )
                return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        }

        return val;
    }

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole)
    {
        if( value.type() == QVariant::String )
        {
            QDateTime newVal;
            newVal.fromString(value.toString(), "dd-MM-yyyy");

            if (newVal.isValid())
            {
                QVariant newVar(newVal);
                return T::setData(index, newVar, role);
            }
        }

        return T::setData(index,value,role);
    }
};


// Delegate class for Date control
class DateStyledDelegate : public QStyledItemDelegate
{
public:
    DateStyledDelegate(QWidget *parent = 0):QStyledItemDelegate(parent)
    {
    }
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex & index) const
    {
        QDateEdit *editor = new QDateEdit(parent);
        editor->setDateTime(index.data().toDateTime());
        editor->setDisplayFormat("dd-MM-yyyy");
        editor->setCalendarPopup(true);
        return editor;
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const
    {
        editor->setGeometry(option.rect);
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                                       const QModelIndex &index) const
    {
        QDateEdit *dateEdit = static_cast<QDateEdit*>(editor);
        dateEdit->setDisplayFormat("dd-MM-yyyy");
        dateEdit->interpretText();
        QDate value = dateEdit->date();
        model->setData(index, value, Qt::EditRole);
    }
};

// Delegate class for Numeric control
class NumericStyledDelegate : public QStyledItemDelegate
{
public:
    NumericStyledDelegate(QWidget* parent = 0):QStyledItemDelegate(parent){}

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex & index) const
    {
        QLineEdit * editor = new QLineEdit(parent);
        editor->setAlignment(Qt::AlignRight);
        editor->setText(index.data().toString());
        return editor;
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const
    {
        editor->setGeometry(option.rect);
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
    {
        QLineEdit * pLineEdit = static_cast<QLineEdit*>(editor);
        QLocale locale(QLocale::English,QLocale::UnitedStates);

        bool success = false;
        double dbl = locale.toDouble(pLineEdit->text(), &success);
        if( success )
            model->setData(index, QVariant(dbl), Qt::EditRole);
        else
            model->setData(index, QVariant(pLineEdit->text()), Qt::EditRole);
    }
};

#endif // HELPER_HXX
