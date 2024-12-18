#pragma once
#include <QPainter>
#include <QtWidgets/QMainWindow>
#include <QTimer>

inline  const QColor rgbColors[3] = { QColor(255,0,0),QColor(0,255,0),QColor(0,0,255) };


struct CommonFanSettings
{
	uint8_t fanBladesCount = 8;

	int simulationFps = 24;

	/* How many rotates every second make fan blade.
	For example 24.
	Common big fan on floor make 1000 rpm or 16.7 rps,
	if (simulationFps == fanRps) sync is OK
	*/
	double fanRps = 22.0;

	/* How many degrees moved blade every 1 second.
	   For example 24 * 360 = 8640�. */
	double fanAngleSpeedDegreesEverySec() const { return fanRps * 360; }

	/* How many degrees moved blade every 1 simulation frame.
	 * Used to calculate when begin new frame.
	 * For example 24 * 360 = 8640�.
	 */
	double fanAngleSpeedDegreesEveryFrame() const { return fanAngleSpeedDegreesEverySec() / simulationFps; };
};

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