#pragma once

#include "source/present/ResultTable.hpp"
#include <QAbstractTableModel>
#include <QHash>
#include <QStringList>
#include <QVariant>

namespace fin::gui
{
    // A result table as the window shows it: the strings the presentation layer formatted, never a
    // number reformatted here.
    class ResultTableModel
        : public QAbstractTableModel
    {
        Q_OBJECT
        Q_PROPERTY(QStringList headings READ Headings NOTIFY changed)
        Q_PROPERTY(int rows READ Rows NOTIFY changed)
        Q_PROPERTY(int columns READ Columns NOTIFY changed)

    public:
        explicit ResultTableModel(QObject* parent = nullptr);

        void Present(const present::ResultTable& table);
        const present::ResultTable& Shown() const;

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QHash<int, QByteArray> roleNames() const override;

        QStringList Headings() const;
        int Rows() const;
        int Columns() const;
        Q_INVOKABLE QString cell(int row, int column) const;

    signals:
        void changed();

    private:
        present::ResultTable table;
    };
}
