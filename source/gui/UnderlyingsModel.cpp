#include "source/gui/UnderlyingsModel.hpp"
#include <QString>

namespace fin::gui
{
    UnderlyingsModel::UnderlyingsModel(present::ScenarioDocument& document, QObject* parent)
        : QAbstractListModel(parent)
        , document(document)
    {}

    int UnderlyingsModel::rowCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : static_cast<int>(document.Underlyings().size());
    }

    int UnderlyingsModel::Count() const
    {
        return rowCount();
    }

    QVariant UnderlyingsModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= rowCount())
            return {};

        const auto& row = document.Underlyings()[static_cast<std::size_t>(index.row())];

        switch (role)
        {
            case Qt::DisplayRole:
            case ticker:
                return QString::fromStdString(row.ticker);
            case spotCents:
                return row.spotCents;
            case driftBasisPoints:
                return row.driftBasisPoints;
            case volatilityBasisPoints:
                return row.volatilityBasisPoints;
            case impliedVolatilityBasisPoints:
                return row.impliedVolatilityBasisPoints;
            case dividendYieldBasisPoints:
                return row.dividendYieldBasisPoints;
            case dividendWithholdingBasisPoints:
                return row.dividendWithholdingBasisPoints;
            case strikeIncrementCents:
                return row.strikeIncrementCents;
            default:
                return {};
        }
    }

    bool UnderlyingsModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= rowCount())
            return false;

        auto row = document.Underlyings()[static_cast<std::size_t>(index.row())];
        bool numeric = true;

        switch (role)
        {
            case Qt::EditRole:
            case ticker:
                row.ticker = value.toString().trimmed().toStdString();
                break;
            case spotCents:
                row.spotCents = value.toInt(&numeric);
                break;
            case driftBasisPoints:
                row.driftBasisPoints = value.toInt(&numeric);
                break;
            case volatilityBasisPoints:
                row.volatilityBasisPoints = value.toInt(&numeric);
                break;
            case impliedVolatilityBasisPoints:
                row.impliedVolatilityBasisPoints = value.toInt(&numeric);
                break;
            case dividendYieldBasisPoints:
                row.dividendYieldBasisPoints = value.toInt(&numeric);
                break;
            case dividendWithholdingBasisPoints:
                row.dividendWithholdingBasisPoints = value.toInt(&numeric);
                break;
            case strikeIncrementCents:
                row.strikeIncrementCents = value.toInt(&numeric);
                break;
            default:
                return false;
        }

        if (!numeric)
            return false;

        document.Replace(static_cast<std::size_t>(index.row()), row);

        emit dataChanged(index, index, { role });
        emit edited();

        return true;
    }

    Qt::ItemFlags UnderlyingsModel::flags(const QModelIndex& index) const
    {
        return index.isValid() ? Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable : Qt::NoItemFlags;
    }

    QHash<int, QByteArray> UnderlyingsModel::roleNames() const
    {
        return { { ticker, "ticker" },
            { spotCents, "spotCents" },
            { driftBasisPoints, "driftBasisPoints" },
            { volatilityBasisPoints, "volatilityBasisPoints" },
            { impliedVolatilityBasisPoints, "impliedVolatilityBasisPoints" },
            { dividendYieldBasisPoints, "dividendYieldBasisPoints" },
            { dividendWithholdingBasisPoints, "dividendWithholdingBasisPoints" },
            { strikeIncrementCents, "strikeIncrementCents" } };
    }

    int UnderlyingsModel::RoleOf(int column)
    {
        return ticker + column;
    }

    QVariant UnderlyingsModel::cell(int row, int column) const
    {
        if (column < 0 || column >= columns)
            return {};

        return data(index(row), RoleOf(column));
    }

    bool UnderlyingsModel::setCell(int row, int column, const QVariant& value)
    {
        if (column < 0 || column >= columns)
            return false;

        return setData(index(row), value, RoleOf(column));
    }

    QString UnderlyingsModel::heading(int column) const
    {
        static const char* const headings[columns] = { "Ticker", "Spot", "Drift", "Realised", "Implied", "Yield", "Withheld", "Strike" };

        return column >= 0 && column < columns ? tr(headings[column]) : QString();
    }

    QString UnderlyingsModel::unit(int column) const
    {
        static const char* const units[columns] = { "the name as the market lists it", "spot price, cents", "drift, basis points a year", "realised volatility, basis points a year", "implied volatility, basis points a year", "dividend yield, basis points a year", "dividend withholding, basis points", "strike step, cents" };

        return column >= 0 && column < columns ? tr(units[column]) : QString();
    }

    void UnderlyingsModel::Reset()
    {
        beginResetModel();
        endResetModel();

        emit countChanged();
    }
}
