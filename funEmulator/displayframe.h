#pragma once

#include <QWidget>
#include <QPainter>
#include <QResizeEvent>

class DisplayFrame: public QWidget
{
    using base_type = QWidget;

private:
    QImage m_drawBuffer;

public:
    DisplayFrame(QWidget* parent = nullptr): QWidget(parent)
    {
        setMinimumSize(300, 200);
    }

    void setBuffer(QImage image)
    {
        m_drawBuffer = std::move(image);
        repaint();
    }

protected:

    virtual void resizeEvent(QResizeEvent* event)
    {
        int size = qMin(event->size().width(), event->size().height()); // Determine the smaller dimension
        resize(size, size); // Force the widget to be square
        m_drawBuffer = QImage(size, size, QImage::Format_RGBA64_Premultiplied);
    }

    virtual void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform); //< Enable smooth scaling

        painter.drawImage(0, 0, m_drawBuffer);
    }
};
