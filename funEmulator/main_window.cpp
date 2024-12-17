#include "main_window.h"

#include <QPainter>

const int kFps = 24;

QtFunEmulatorApplication::QtFunEmulatorApplication(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    connect(&m_timer, &QTimer::timeout,
        [this]()
        {
            paintSomethingToBuffer();
            repaint(); //< Repaint main windows to display new buffer.
        });

    connect(ui.spinBoxFps, &QSpinBox::valueChanged, this, [this](){ restartTimer();});
    restartTimer();
}

void QtFunEmulatorApplication::paintSomethingToBuffer()
{
    // Draw to buffer allocated in QImage
    QImage* buffer = ui.bufferDataWidget->buffer();
    QPainter painter(buffer);
    int startAngle = 0; //< Value in 1/16th of a degree.
    int spanAngle = -16 * 180; //< Value in 1/16th of a degree.
    painter.drawArc(buffer->rect(), startAngle, spanAngle);
}

void QtFunEmulatorApplication::restartTimer()
{
    m_timer.stop();
    m_timer.start(1000 / ui.spinBoxFps->value());
}
