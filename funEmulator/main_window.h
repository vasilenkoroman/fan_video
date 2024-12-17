#pragma once

#include <QtWidgets/QMainWindow>
#include <QTimer>

#include "ui_main_window.h"

class QtFunEmulatorApplication : public QMainWindow
{
    Q_OBJECT
    using base_type = QMainWindow;
public:
    QtFunEmulatorApplication(QWidget *parent = nullptr);

private:
    void restartTimer();
    void paintSomethingToBuffer();

private:
    Ui::QtFunEmulatorApplication ui;
    QTimer m_timer;
};
