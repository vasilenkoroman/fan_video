#pragma once

#include <QtWidgets/QMainWindow>
#include <QTimer>
#include <QImage>

#include <deque>
#include <future>

#include "ui_main_window.h"

class QtFunEmulatorApplication : public QMainWindow
{
    Q_OBJECT
    using base_type = QMainWindow;
public:
    QtFunEmulatorApplication(QWidget *parent = nullptr);

private:
    void restartTimer();
    void paintFan();

private:
    Ui::QtFunEmulatorApplication ui;
    QTimer m_timer;
    std::deque<std::future<QImage>> m_asyncDrawTask;
    uint64_t m_frameCounter = 0;

};
