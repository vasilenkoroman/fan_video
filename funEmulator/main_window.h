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
    ~QtFunEmulatorApplication();

    void paintEvent(QPaintEvent* event) override;

private:
    Ui::QtFunEmulatorApplication ui;
    QImage m_drawBuffer;
    QTimer m_timer;
};
