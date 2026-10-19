#pragma once

#include "source/present/ScenarioDocument.hpp"
#include <QAbstractListModel>
#include <QHash>
#include <QVariant>
#include <cstddef>

namespace fin::gui
{
    class UnderlyingsModel
        : public QAbstractListModel
    {
        Q_OBJECT
        Q_PROPERTY(int count READ Count NOTIFY countChanged)

    public:
        enum Role
        {
            ticker = Qt::UserRole + 1,
            spotCents,
            driftBasisPoints,
            volatilityBasisPoints,
            impliedVolatilityBasisPoints,
            dividendYieldBasisPoints,
            dividendWithholdingBasisPoints,
            strikeIncrementCents
        };
        Q_ENUM(Role)

        static constexpr int columns = 8;

        explicit UnderlyingsModel(present::ScenarioDocument& document, QObject* parent = nullptr);

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role) override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QHash<int, QByteArray> roleNames() const override;

        int Count() const;
        Q_INVOKABLE QVariant cell(int row, int column) const;
        Q_INVOKABLE bool setCell(int row, int column, const QVariant& value);
        Q_INVOKABLE QString heading(int column) const;
        Q_INVOKABLE QString unit(int column) const;

        void Reset();

    signals:
        void countChanged();
        void edited();

    private:
        static int RoleOf(int column);

        present::ScenarioDocument& document;
    };
}
