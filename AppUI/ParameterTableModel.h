#ifndef DNC_SCADA_PARAMETER_TABLE_MODEL_H
#define DNC_SCADA_PARAMETER_TABLE_MODEL_H

#include "Types.h"

#include <QAbstractTableModel>
#include <QVector>

namespace DncScada {

class ParameterTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ParameterTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    ProcessParameter parameterAt(int row) const;
    void setDeviceId(quint8 deviceId);

private:
    QVector<ProcessParameter> parameters;
};

} // namespace DncScada

#endif // DNC_SCADA_PARAMETER_TABLE_MODEL_H
