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
            // Draw to buffer allocated in QImage
            QPainter painter(&m_drawBuffer);
            QRectF rectangle(
                0,
                40,
                width(),
                height());
            int startAngle = 30 * 16;
            int spanAngle = 120 * 16;
            painter.drawArc(rectangle, startAngle, spanAngle);

            // Repaint main windows to display new buffer.
            repaint();
        });

    // QImage format is 16-bit per component, 64 bit per pixel.
    m_drawBuffer = QImage(width(), height(), QImage::Format_RGBA64_Premultiplied);

    m_timer.start(1000 / kFps);
}

void QtFunEmulatorApplication::paintEvent(QPaintEvent* event)
{
    base_type::paintEvent(event);
    QPainter painter(this);
    // Draw buffer to the mainWindow
    painter.drawImage(QRect(0, 0, width(), height()), m_drawBuffer);
}

QtFunEmulatorApplication::~QtFunEmulatorApplication()
{}
