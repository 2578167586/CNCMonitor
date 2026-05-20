#include "NumericDelegate.h"

#include <QDoubleSpinBox>

namespace DncScada {

NumericDelegate::NumericDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *NumericDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                       const QModelIndex &index) const
{
    Q_UNUSED(option)
    if (index.column() == 1) {
        QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
        editor->setDecimals(2);
        editor->setRange(0.0, 12000.0);
        editor->setSingleStep(10.0);
        return editor;
    }

    return QStyledItemDelegate::createEditor(parent, option, index);
}

} // namespace DncScada
