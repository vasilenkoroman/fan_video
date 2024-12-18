#pragma once
#include <QPainter>
#include <QtWidgets/QMainWindow>
#include <QTimer>

inline  const QColor rgbColors[3] = { QColor(255,0,0),QColor(0,255,0),QColor(0,0,255) };


inline uint8_t fanBladesCount = 8;//TODO fanBladesCount not work in UI



inline void drawLineFromArc(QPainter& painter, QRectF r, double aStart, double aLength)
{
	double radius = r.width() / 2.0;
	double center = r.x() + radius;

	double startAngleRad = aStart * M_PI / 180.0;
	double endAngleRad = (aStart + aLength) * M_PI / 180.0;

	double xStart = center + radius * cos(-startAngleRad);
	double yStart = center + radius * sin(-startAngleRad);

	double xEnd = center + radius * cos(-endAngleRad);
	double yEnd = center + radius * sin(-endAngleRad);

	painter.drawLine(QPointF(xStart, yStart), QPointF(xEnd, yEnd));
}