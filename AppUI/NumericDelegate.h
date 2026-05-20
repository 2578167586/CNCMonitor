#ifndef DNC_SCADA_NUMERIC_DELEGATE_H
#define DNC_SCADA_NUMERIC_DELEGATE_H

#include <QStyledItemDelegate>

namespace DncScada {

class NumericDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit NumericDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
};

} // namespace DncScada

#endif // DNC_SCADA_NUMERIC_DELEGATE_H
