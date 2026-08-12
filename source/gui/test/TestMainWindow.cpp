#include "source/gui/MainWindow.hpp"
#include "source/gui/ScenarioPanel.hpp"
#include "source/io/ReportWriter.hpp"
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>

namespace
{
    QString Configurations()
    {
        return QString(FIN_BS_CONFIG_DIRECTORY);
    }

    QString Example()
    {
        return Configurations() + "/scenarios/example.json";
    }

    QString Golden()
    {
        return QString(FIN_BS_DATA_DIRECTORY) + "/golden/example-projection.csv";
    }

    QString Contents(const QString& path)
    {
        QFile file(path);

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return QString();

        return QTextStream(&file).readAll();
    }
}

class TestMainWindow
    : public QObject
{
    Q_OBJECT

private slots:

    void opens_a_scenario_and_shows_the_underlyings_it_names()
    {
        fin::gui::MainWindow window(Configurations());

        QVERIFY(window.Open(Example()));
        QCOMPARE(window.Scenario().Document().Underlyings().size(), std::size_t(3));
        QCOMPARE(window.findChild<QPushButton*>("project")->isEnabled(), true);
    }

    void refuses_to_run_a_field_it_cannot_read_and_says_which()
    {
        fin::gui::MainWindow window(Configurations());
        window.Open(Example());

        window.Scenario().findChild<QSpinBox*>("target_delta_basis_points")->setValue(15000);

        QCOMPARE(window.findChild<QPushButton*>("project")->isEnabled(), false);
        QVERIFY(window.findChild<QLabel*>("status")->text().contains("target_delta_basis_points"));
    }

    void offers_cancel_only_while_a_run_is_under_way()
    {
        fin::gui::MainWindow window(Configurations());
        window.Open(Example());

        QCOMPARE(window.findChild<QPushButton*>("cancel")->isEnabled(), false);

        window.Start(fin::present::RunKind::projection);

        QCOMPARE(window.findChild<QPushButton*>("cancel")->isEnabled(), true);
        QCOMPARE(window.findChild<QPushButton*>("project")->isEnabled(), false);

        window.Await();

        QCOMPARE(window.findChild<QPushButton*>("cancel")->isEnabled(), false);
        QCOMPARE(window.findChild<QPushButton*>("project")->isEnabled(), true);
    }

    void writes_a_scenario_that_reads_back_as_the_same_document()
    {
        QTemporaryDir room;
        fin::gui::MainWindow window(Configurations());
        window.Open(Example());

        const auto before = window.Scenario().Document();

        QVERIFY(window.Save(room.filePath("saved.json")));
        QVERIFY(window.Open(room.filePath("saved.json")));
        QVERIFY(window.Scenario().Document() == before);
    }

    void exports_nothing_until_something_has_been_run()
    {
        QTemporaryDir room;
        fin::gui::MainWindow window(Configurations());
        window.Open(Example());

        QVERIFY(!window.Export(room.filePath("results.csv")));
    }

    void runs_the_published_scenario_to_the_table_it_has_always_run_to()
    {
        fin::gui::MainWindow window(Configurations());
        window.Open(Example());
        window.Start(fin::present::RunKind::projection);
        window.Await();

        QCOMPARE(window.Controller().State(), fin::present::RunState::completed);

        const auto written = QString::fromStdString(fin::io::CycleReport(window.Controller().Result()->rows));

        QCOMPARE(written, Contents(Golden()));
    }

    void exports_the_run_it_has_just_made()
    {
        QTemporaryDir room;
        fin::gui::MainWindow window(Configurations());
        window.Open(Example());
        window.Start(fin::present::RunKind::projection);
        window.Await();

        QVERIFY(window.Export(room.filePath("results.csv")));
        QVERIFY(Contents(room.filePath("results.csv")).startsWith(Contents(Golden())));
    }
};

QTEST_MAIN(TestMainWindow)

#include "TestMainWindow.moc"
