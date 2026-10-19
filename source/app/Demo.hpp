#pragma once

#include "source/gui/Workspace.hpp"
#include <QObject>
#include <QQmlApplicationEngine>
#include <QString>
#include <cstddef>
#include <functional>
#include <vector>

class QQuickWindow;

namespace fin::app
{
    class Demo
        : public QObject
    {
        Q_OBJECT

    public:
        Demo(QQmlApplicationEngine& engine, gui::Workspace& workspace, QString folder);

    private:
        struct Step
        {
            QString name;
            std::function<void()> act;
            bool awaitsRun;
        };

        void Next();
        void Capture(const QString& name);
        void ShowTab(int index);
        void Scroll(double fraction);
        QQuickWindow* Window() const;

        QQmlApplicationEngine& engine;
        gui::Workspace& workspace;
        QString folder;
        std::vector<Step> steps;
        std::size_t at{ 0 };
        int taken{ 0 };
    };
}
