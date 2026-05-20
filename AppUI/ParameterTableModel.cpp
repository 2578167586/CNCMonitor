#include "ParameterTableModel.h"

#include <QStringList>

namespace DncScada {

ParameterTableModel::ParameterTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    parameters = {
        {1, QStringLiteral("FeedRate"), 1200.0, 0.0, 6000.0, true},
        {1, QStringLiteral("SpindleLimit"), 5000.0, 0.0, 12000.0, true},
        {1, QStringLiteral("CoolantFlow"), 45.0, 0.0, 100.0, false}
    };
}

int ParameterTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : parameters.size();
}

int ParameterTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 5;
}

QVariant ParameterTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= parameters.size()) {
        return QVariant();
    }

    const ProcessParameter &parameter = parameters.at(index.row());
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case 0: return parameter.name;
        case 1: return parameter.value;
        case 2: return parameter.minimum;
        case 3: return parameter.maximum;
        case 4: return parameter.coreParameter ? QStringLiteral("Core") : QStringLiteral("Runtime");
        default: return QVariant();
        }
    }
    return QVariant();
}

QVariant ParameterTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }
    static const QStringList headers = {
        QStringLiteral("Name"),
        QStringLiteral("Value"),
        QStringLiteral("Min"),
        QStringLiteral("Max"),
        QStringLiteral("Type")
    };
    return headers.value(section);
}

Qt::ItemFlags ParameterTableModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags result = QAbstractTableModel::flags(index);
    if (index.isValid() && index.column() == 1) {
        result |= Qt::ItemIsEditable;
    }
    return result;
}

bool ParameterTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole || index.row() >= parameters.size()) {
        return false;
    }

    ProcessParameter &parameter = parameters[index.row()];
    if (index.column() == 1) {
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (!ok || number < parameter.minimum || number > parameter.maximum) {
            return false;
        }
        parameter.value = number;
    } else {
        return false;
    }

    emit dataChanged(index, index, {role, Qt::DisplayRole});
    return true;
}

ProcessParameter ParameterTableModel::parameterAt(int row) const
{
    return parameters.value(row);
}

void ParameterTableModel::setDeviceId(quint8 deviceId)
{
    for (ProcessParameter &parameter : parameters) {
        parameter.deviceId = deviceId;
    }
}

} // namespace DncScada
