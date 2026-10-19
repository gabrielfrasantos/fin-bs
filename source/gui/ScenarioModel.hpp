#pragma once

#include "source/gui/UnderlyingsModel.hpp"
#include "source/present/ScenarioDocument.hpp"
#include <QDate>
#include <QObject>
#include <QString>
#include <cstddef>
#include <string>
#include <vector>

namespace fin::gui
{
    // The scenario being edited, as properties the window binds to. Every write goes through the
    // Qt-free document, which is also where a field is judged: the window never clamps a value, it
    // shows why the document refuses it.
    class ScenarioModel
        : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString name READ Name WRITE SetName NOTIFY changed)
        Q_PROPERTY(QString currency READ Currency NOTIFY changed)
        Q_PROPERTY(int capitalCents READ CapitalCents WRITE SetCapitalCents NOTIFY changed)
        Q_PROPERTY(QDate start READ Start WRITE SetStart NOTIFY changed)
        Q_PROPERTY(int cycles READ Cycles WRITE SetCycles NOTIFY changed)
        Q_PROPERTY(int daysPerCycle READ DaysPerCycle WRITE SetDaysPerCycle NOTIFY changed)
        Q_PROPERTY(int observationDaysBeforeExpiry READ ObservationDaysBeforeExpiry WRITE SetObservationDaysBeforeExpiry NOTIFY changed)
        Q_PROPERTY(int targetDeltaBasisPoints READ TargetDeltaBasisPoints WRITE SetTargetDeltaBasisPoints NOTIFY changed)
        Q_PROPERTY(int cashBufferBasisPoints READ CashBufferBasisPoints WRITE SetCashBufferBasisPoints NOTIFY changed)
        Q_PROPERTY(int rollDeltaLimitBasisPoints READ RollDeltaLimitBasisPoints WRITE SetRollDeltaLimitBasisPoints NOTIFY changed)
        Q_PROPERTY(int rollDaysToExpiry READ RollDaysToExpiry WRITE SetRollDaysToExpiry NOTIFY changed)
        Q_PROPERTY(int rollProfitCaptureBasisPoints READ RollProfitCaptureBasisPoints WRITE SetRollProfitCaptureBasisPoints NOTIFY changed)
        Q_PROPERTY(int riskFreeRateBasisPoints READ RiskFreeRateBasisPoints WRITE SetRiskFreeRateBasisPoints NOTIFY changed)
        Q_PROPERTY(bool assessed READ Assessed WRITE SetAssessed NOTIFY changed)
        Q_PROPERTY(int assessmentMonth READ AssessmentMonth WRITE SetAssessmentMonth NOTIFY changed)
        Q_PROPERTY(int assessmentDay READ AssessmentDay WRITE SetAssessmentDay NOTIFY changed)
        Q_PROPERTY(int paths READ Paths WRITE SetPaths NOTIFY changed)
        Q_PROPERTY(int seed READ Seed WRITE SetSeed NOTIFY changed)
        Q_PROPERTY(QString taxConfiguration READ TaxConfiguration WRITE SetTaxConfiguration NOTIFY changed)
        Q_PROPERTY(QString brokerConfiguration READ BrokerConfiguration WRITE SetBrokerConfiguration NOTIFY changed)
        Q_PROPERTY(bool wellFormed READ WellFormed NOTIFY changed)
        Q_PROPERTY(QString offendingField READ OffendingField NOTIFY changed)
        Q_PROPERTY(QString complaint READ Complaint NOTIFY changed)
        Q_PROPERTY(QString priceComplaint READ PriceComplaint NOTIFY changed)
        Q_PROPERTY(fin::gui::UnderlyingsModel* underlyings READ Underlyings CONSTANT)

    public:
        explicit ScenarioModel(QObject* parent = nullptr);

        const present::ScenarioDocument& Document() const;
        void Present(const present::ScenarioDocument& document);

        QString Name() const;
        void SetName(const QString& name);
        QString Currency() const;
        int CapitalCents() const;
        void SetCapitalCents(int cents);
        QDate Start() const;
        void SetStart(const QDate& date);
        int Cycles() const;
        void SetCycles(int count);
        int DaysPerCycle() const;
        void SetDaysPerCycle(int days);
        int ObservationDaysBeforeExpiry() const;
        void SetObservationDaysBeforeExpiry(int days);
        int TargetDeltaBasisPoints() const;
        void SetTargetDeltaBasisPoints(int basisPoints);
        int CashBufferBasisPoints() const;
        void SetCashBufferBasisPoints(int basisPoints);
        int RollDeltaLimitBasisPoints() const;
        void SetRollDeltaLimitBasisPoints(int basisPoints);
        int RollDaysToExpiry() const;
        void SetRollDaysToExpiry(int days);
        int RollProfitCaptureBasisPoints() const;
        void SetRollProfitCaptureBasisPoints(int basisPoints);
        int RiskFreeRateBasisPoints() const;
        void SetRiskFreeRateBasisPoints(int basisPoints);
        bool Assessed() const;
        void SetAssessed(bool assessed);
        int AssessmentMonth() const;
        void SetAssessmentMonth(int month);
        int AssessmentDay() const;
        void SetAssessmentDay(int day);
        int Paths() const;
        void SetPaths(int count);
        int Seed() const;
        void SetSeed(int seed);
        QString TaxConfiguration() const;
        void SetTaxConfiguration(const QString& path);
        QString BrokerConfiguration() const;
        void SetBrokerConfiguration(const QString& path);

        bool WellFormed() const;
        QString OffendingField() const;
        QString Complaint() const;
        QString PriceComplaint() const;
        UnderlyingsModel* Underlyings();

        Q_INVOKABLE void add();
        Q_INVOKABLE void remove(int index);
        Q_INVOKABLE void move(int index, int step);
        Q_INVOKABLE bool estimate(int index, const QString& path);

        bool Adopt(std::size_t index, const std::vector<std::string>& closes);

    signals:
        void changed();

    private:
        template<class Edit>
        void Amend(Edit edit);

        present::ScenarioDocument document;
        UnderlyingsModel* underlyings;
        QString priceComplaint;
    };
}
