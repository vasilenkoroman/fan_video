#pragma once
#include <QPainter>
#include <QtWidgets/QMainWindow>
#include <QTimer>

inline  const QColor rgbColors[3] = { QColor(255,0,0),QColor(0,255,0),QColor(0,0,255) };


inline uint8_t fanBladesCount = 1;

inline double simulationFps = 24;


/* How many rotates every second make fan blade.
For example 24.
Common big fan on floor make 1000 rpm or 16.7 rps,
if (simulationFps == fanRps) sync is OK
*/
inline double fanRps = 22;

/* How many degrees moved blade every 1 second.
For example 24 * 360 = 8640°.*/
inline double fanAngleSpeedDegreesEverySec = fanRps * 360;

/* How many degrees moved blade every 1 simulation frame.
* Used to calculate when begin new frame.
For example 24 * 360 = 8640°.*/
inline double fanAngleSpeedDegreesEveryFrame = fanAngleSpeedDegreesEverySec / simulationFps;