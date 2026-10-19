#include "source/gui/Workspace.hpp"
#include "source/io/ReportWriter.hpp"
#include <QFile>
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

class TestRunModel
    : public QObject
{
    Q_OBJECT

private slots:

    void opens_a_scenario_and_offers_to_run_it()
    {
        fin::gui::Workspace workspace(Configurations());

        QVERIFY(workspace.Open(Example()));
        QCOMPARE(workspace.Scenario()->Underlyings()->Count(), 3);
        QCOMPARE(workspace.Run()->Runnable(), true);
        QCOMPARE(workspace.Run()->HasResult(), false);
    }

    void refuses_to_run_a_field_it_cannot_read_and_says_which()
    {
        fin::gui::Workspace workspace(Configurations());
        workspace.Open(Example());

        workspace.Scenario()->SetTargetDeltaBasisPoints(15000);

        QCOMPARE(workspace.Run()->Runnable(), false);
        QVERIFY(workspace.Scenario()->Complaint().contains("target_delta_basis_points"));
    }

    void is_running_only_while_a_run_is_under_way()
    {
        fin::gui::Workspace workspace(Configurations());
        workspace.Open(Example());

        QCOMPARE(workspace.Run()->Running(), false);

        workspace.Run()->Start(fin::present::RunKind::projection);

        QCOMPARE(workspace.Run()->Running(), true);
        QCOMPARE(workspace.Run()->Runnable(), false);

        workspace.Run()->Await();

        QCOMPARE(workspace.Run()->Running(), false);
        QCOMPARE(workspace.Run()->Runnable(), true);
        QCOMPARE(workspace.Run()->HasResult(), true);
    }

    void writes_a_scenario_that_reads_back_as_the_same_document()
    {
        QTemporaryDir room;
        fin::gui::Workspace workspace(Configurations());
        workspace.Open(Example());

        const auto before = workspace.Scenario()->Document();

        QVERIFY(workspace.Save(room.filePath("saved.json")));
        QVERIFY(workspace.Open(room.filePath("saved.json")));
        QVERIFY(workspace.Scenario()->Document() == before);
    }

    void exports_nothing_until_something_has_been_run()
    {
        QTemporaryDir room;
        fin::gui::Workspace workspace(Configurations());
        workspace.Open(Example());

        QVERIFY(!workspace.Export(room.filePath("results.csv")));
        QVERIFY(!workspace.Message().isEmpty());
    }

    void runs_the_published_scenario_to_the_table_it_has_always_run_to()
    {
        fin::gui::Workspace workspace(Configurations());
        workspace.Open(Example());
        workspace.Run()->Start(fin::present::RunKind::projection);
        workspace.Run()->Await();

        QCOMPARE(workspace.Run()->Controller().State(), fin::present::RunState::completed);
        QCOMPARE(workspace.Run()->Cycles()->Rows(), 18);
        QVERIFY(!workspace.Run()->Highlights().isEmpty());

        const auto written = QString::fromStdString(fin::io::CycleReport(workspace.Run()->Controller().Result()->rows));

        QCOMPARE(written, Contents(Golden()));
    }

    void exports_the_run_it_has_just_made()
    {
        QTemporaryDir room;
        fin::gui::Workspace workspace(Configurations());
        workspace.Open(Example());
        workspace.Run()->Start(fin::present::RunKind::projection);
        workspace.Run()->Await();

        QVERIFY(workspace.Export(room.filePath("results.csv")));
        QVERIFY(Contents(room.filePath("results.csv")).startsWith(Contents(Golden())));
    }
};

QTEST_MAIN(TestRunModel)

#include "TestRunModel.moc"
