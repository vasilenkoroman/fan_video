#include "main_window.h"

#include <QPainter>

double globalCurrentAngleDegrees = 0;

#include "DrawMcuPwm.h"
#include "DrawWs2812.h"

/* Every frame, every 1 fps, every repaint counterFrames++. */
uint64_t counterFrames = 0;

QtFunEmulatorApplication::QtFunEmulatorApplication(QWidget* parent)
  : QMainWindow(parent)
{
  ui.setupUi(this);

  connect(&m_timer, &QTimer::timeout,
    [this]()
    {
      paintSomethingToBuffer();
      repaint(); //< Repaint main windows to display new buffer.
    });

  connect(ui.spinBoxFps, &QSpinBox::valueChanged, this, [this]() { restartTimer(); });
  restartTimer();
}

enum UI_TABS { UI_TAB_MCU_PWM, UI_TAB_WS2118 };
UI_TABS uiTab = UI_TAB_MCU_PWM;
void QtFunEmulatorApplication::paintSomethingToBuffer()
{
  // Draw to buffer allocated in QImage
  QImage* buffer = ui.bufferDataWidget->buffer();
  QPainter painter(buffer);

  QRectF rectAll(0, 0, width(), height());
  painter.setBrush(QBrush(QColor(0, 0, 0)));
  painter.drawRect(rectAll);//Fill all black
  painter.setBrush(QBrush(QColor(0, 0, 0, 0)));//remove brush

  painter.setPen(Qt::white);
  painter.drawEllipse(QPointF(fanCenterX, fanCenterY), fanRadiusPx, fanRadiusPx);//FAN canvas

  double maxGlobalAngleThisRepaint = counterFrames * fanAngleSpeedDegreesEveryFrame;

  for (uint64_t flash = 0; flash < 999999 && globalCurrentAngleDegrees < maxGlobalAngleThisRepaint; flash++)
  {
    if(uiTab == UI_TAB_MCU_PWM)
      DrawMcuPwm::draw(painter, flash, &globalCurrentAngleDegrees);
    else if(uiTab == UI_TAB_WS2118)
      DrawWs2812::draw();
  }

  // Repaint main windows to display new buffer.
  repaint();
  counterFrames++;
}

void QtFunEmulatorApplication::restartTimer()
{
  m_timer.stop();
  m_timer.start(1000 / ui.spinBoxFps->value());
}