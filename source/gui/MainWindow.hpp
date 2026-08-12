#pragma once

#include "source/present/RunController.hpp"
#include <QMainWindow>
#include <QString>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

class QLabel;
class QProgressBar;
class QPushButton;
class QTimer;

namespace fin::gui
{
    class ResultsPanel;
    class ScenarioPanel;

    class MainWindow
        : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit MainWindow(QString configurations, QWidget* parent = nullptr);
        ~MainWindow() override;

        ScenarioPanel& Scenario();

        bool Open(const QString& path);
        bool Save(const QString& path) const;
        bool Export(const QString& path) const;

        void Start(present::RunKind kind);
        void Stop();
        void Await();

        const present::RunController& Controller() const;
        QString Complaint() const;

    private:
        void BuildActions();
        void Judge();
        void Poll();
        void Finish();
        std::optional<present::RunInputs> Gathered();
        std::vector<std::string> Read(const QString& path) const;

        QString configurations;
        present::RunController controller;
        std::unique_ptr<std::thread> worker;
        ScenarioPanel* scenario{ nullptr };
        ResultsPanel* results{ nullptr };
        QProgressBar* progress{ nullptr };
        QLabel* status{ nullptr };
        QPushButton* project{ nullptr };
        QPushButton* simulate{ nullptr };
        QPushButton* search{ nullptr };
        QPushButton* cancel{ nullptr };
        QTimer* poll{ nullptr };
        QString complaint;
    };
}
