#include "source/gui/ResultTableModel.hpp"
#include <cstddef>

namespace fin::gui
{
    ResultTableModel::ResultTableModel(QObject* parent)
        : QAbstractTableModel(parent)
    {}

    void ResultTableModel::Present(const present::ResultTable& shown)
    {
        beginResetModel();
        table = shown;
        endResetModel();

        emit changed();
    }

    const present::ResultTable& ResultTableModel::Shown() const
    {
        return table;
    }

    int ResultTableModel::rowCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : static_cast<int>(table.rows.size());
    }

    int ResultTableModel::columnCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : static_cast<int>(table.headings.size());
    }

    QVariant ResultTableModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || role != Qt::DisplayRole)
            return {};

        return cell(index.row(), index.column());
    }

    QVariant ResultTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal || section < 0 || section >= Columns())
            return {};

        return QString::fromStdString(table.headings[static_cast<std::size_t>(section)]);
    }

    QHash<int, QByteArray> ResultTableModel::roleNames() const
    {
        return { { Qt::DisplayRole, "display" } };
    }

    QStringList ResultTableModel::Headings() const
    {
        QStringList headings;

        for (const auto& heading : table.headings)
            headings << QString::fromStdString(heading);

        return headings;
    }

    int ResultTableModel::Rows() const
    {
        return rowCount();
    }

    int ResultTableModel::Columns() const
    {
        return columnCount();
    }

    QString ResultTableModel::cell(int row, int column) const
    {
        if (row < 0 || row >= Rows() || column < 0 || column >= Columns())
            return {};

        return QString::fromStdString(table.rows[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)]);
    }
}
