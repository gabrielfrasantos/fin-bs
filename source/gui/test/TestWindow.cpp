#include "source/gui/Resources.hpp"
#include "source/gui/Workspace.hpp"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTest>
#include <QUrl>
#include <memory>

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

    struct Shown
    {
        explicit Shown(const QString& scenario)
            : workspace(Configurations())
        {
            static const bool styled = []
            {
                QQuickStyle::setStyle("Material");
                fin::gui::Initialise();
                return true;
            }();
            static_cast<void>(styled);

            workspace.Open(scenario);
            engine.rootContext()->setContextProperty("workspace", &workspace);
            engine.load(QUrl("qrc:/fin/qml/Main.qml"));
        }

        QQuickWindow* Window() const
        {
            return engine.rootObjects().isEmpty() ? nullptr : qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        }

        QObject* Named(const char* name) const
        {
            auto* window = Window();
            return window == nullptr ? nullptr : window->findChild<QObject*>(name);
        }

        bool Enabled(const char* name) const
        {
            auto* object = Named(name);
            return object != nullptr && object->property("enabled").toBool();
        }

        fin::gui::Workspace workspace;
        QQmlApplicationEngine engine;
    };
}

class TestWindow
    : public QObject
{
    Q_OBJECT

private slots:

    void builds_from_the_embedded_files_with_every_control_it_promises()
    {
        const Shown shown(Example());

        QVERIFY(shown.Window() != nullptr);

        for (const auto* name : { "open", "save", "export", "project", "simulate", "search", "cancel", "status", "progress", "resultsTabs", "chart", "cycles", "ledger", "summary", "disclaimer", "underlyings", "add", "remove", "up", "down", "estimate" })
            QVERIFY2(shown.Named(name) != nullptr, name);
    }

    void offers_to_run_a_scenario_that_is_well_formed_and_not_one_that_is_not()
    {
        Shown shown(Example());

        QVERIFY(shown.Named("status") != nullptr);
        QCOMPARE(shown.Enabled("project"), true);
        QCOMPARE(shown.Enabled("cancel"), false);
        QCOMPARE(shown.Enabled("export"), false);

        shown.workspace.Scenario()->SetTargetDeltaBasisPoints(15000);

        QCOMPARE(shown.Enabled("project"), false);
        QVERIFY(shown.Named("status")->property("text").toString().contains("target_delta_basis_points"));
    }

    void shows_the_fields_in_the_units_the_model_stores()
    {
        const Shown shown(Example());

        QVERIFY(shown.Named("capital_cents") != nullptr);
        QCOMPARE(shown.Named("capital_cents")->property("value").toInt(), 20000000);
        QCOMPARE(shown.Named("target_delta_basis_points")->property("value").toInt(), 3000);
        QCOMPARE(shown.Named("underlyings")->property("count").toInt(), 3);
    }

    void a_press_on_project_runs_and_fills_the_tables()
    {
        Shown shown(Example());

        QVERIFY(shown.Named("project") != nullptr);
        QVERIFY(QMetaObject::invokeMethod(shown.Named("project"), "clicked"));
        QCOMPARE(shown.workspace.Run()->Running(), true);
        QCOMPARE(shown.Enabled("cancel"), true);

        shown.workspace.Run()->Await();

        QCOMPARE(shown.Enabled("cancel"), false);
        QCOMPARE(shown.Enabled("export"), true);
        QCOMPARE(shown.workspace.Run()->Cycles()->Rows(), 18);
        QVERIFY(shown.Named("status")->property("text").toString().contains("Done"));
    }

    void flips_the_theme_without_rebuilding_the_window()
    {
        const Shown shown(Example());

        QVERIFY(shown.Window() != nullptr);
        QVERIFY(shown.Window()->setProperty("dark", true));
        QCOMPARE(shown.Window()->property("dark").toBool(), true);
        QCOMPARE(shown.Named("theme")->property("checked").toBool(), true);
    }
};

QTEST_MAIN(TestWindow)

#include "TestWindow.moc"
