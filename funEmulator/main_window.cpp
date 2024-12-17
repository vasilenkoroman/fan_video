#include "main_window.h"

#include <QPainter>

const int kFps = 24;


QColor rgbColors[3] = { QColor(255,0,0),QColor(0,255,0),QColor(0,0,255) };






uint8_t fanBladesCount = 1;

double simulationFps = 24;


/* How many rotates every second make fan blade.
For example 24.
Common big fan on floor make 1000 rpm or 16.7 rps,
if (simulationFps == fanRps) sync is OK
*/
double fanRps = 22;

/* How many degrees moved blade every 1 second.
For example 24 * 360 = 8640°.*/
double fanAngleSpeedDegreesEverySec = fanRps * 360;

/* How many degrees moved blade every 1 simulation frame.
* Used to calculate when begin new frame.
For example 24 * 360 = 8640°.*/
double fanAngleSpeedDegreesEveryFrame = fanAngleSpeedDegreesEverySec / simulationFps;



uint16_t fanCenterX = 202;
uint16_t fanCenterY = 202;
uint16_t fanRadiusPx = 200;
double ledStepPxBetweenRGB = 1;
double globalCurrentAngleDegrees = 0;


namespace DrawMcuPwm
{
  enum PWM_ALIGNS { PWM_LEFT_ALIGN, PWM_CENTER_ALIGN };

  PWM_ALIGNS pwmAlign = PWM_LEFT_ALIGN;

  const uint32_t COLOR_BIT_DEPTH = 8;

  /* MCU PWM base clock in hz. MCU_PWM_TICK_HZ = CPU_HZ / (2, 3, ...).
For example CPU_HZ = 24000000.
MCU_PWM_TICK_HZ = 24000000 / 2 = 12000000
Better be const in MCU and set on compilation firmware, cut can be set dynamic.
*/
  uint32_t MCU_PWM_TICK_HZ = 12 * 1000 * 1000;

  /*One MCU PWM cycle length.
  For example: if MCU_PWM_CYCLE_LENGTH_TICKS == 8:
  PWM can work left align:
  10000000 = LED 12.5%,
  11000000 = LED 25%,
  11110000 = LED 50%,
  11111100 = LED 75%,
  PWM can work center align:
  00011000 = LED 25%,
  00111100 = LED 50%,
  01111110 = LED 75%,
  For 8 bit color channel center align, MCU_PWM_CYCLE_LENGTH_TICKS = 512.
  Better be const in MCU and set on compilation firmware, cut can be set dynamic.
  */

  uint32_t MCU_PWM_CYCLE_LENGTH_TICKS = 1 << COLOR_BIT_DEPTH;

  /*MCU PWM one cycle frequency in hz.
  For example if MCU_PWM_TICK_HZ == 12 MHz, MCU_PWM_CYCLE_LENGTH_TICKS = 512
  MCU_PWM_CYCLE_HZ = 12000000 / 512 = 23437,5hz*/
  double MCU_PWM_CYCLE_HZ = (double)MCU_PWM_TICK_HZ / MCU_PWM_CYCLE_LENGTH_TICKS;

  /* How many PWM cycles will be every cinema frame.
  For example if MCU_PWM_CYCLE_HZ == 23437,5hz and kFps == 24
  MCU_PWM_CYCLE_EVERY_FRAME = 23437,5 / 24 = 976,5625*/
  double MCU_PWM_CYCLE_EVERY_REPAINT = MCU_PWM_CYCLE_HZ / kFps;

  /* How many degrees moved blade every 1 cycle of PWM.
For example 8640° / 23437,5hz = 0,36864°/pwmCycle.*/
  double pwmCycleDegrees = fanAngleSpeedDegreesEverySec / MCU_PWM_CYCLE_HZ;

  static uint8_t MCU_PWM_OUTPUTS = 16;
  /*
  MCU_PWM_STROBE_CHANNELS = 0, brightness = 100%, 16 LEDs, 5 RGB LEDs
  MCU_PWM_STROBE_CHANNELS = 4, brightness = 25%, 48 LEDs, 16 RGB LEDs
  MCU_PWM_STROBE_CHANNELS = 8, brightness = 12.5%, 64 LEDs, 21 RGB LEDs
  */
  static uint8_t MCU_PWM_STROBE_CHANNELS = 8;
  static uint8_t MCU_PWM_LED_OUTPUTS = MCU_PWM_OUTPUTS - MCU_PWM_STROBE_CHANNELS;//12
  static uint32_t RGB_LED_COUNT = MCU_PWM_LED_OUTPUTS * MCU_PWM_STROBE_CHANNELS / 3;//16
  static double ledStepPx = fanRadiusPx / (RGB_LED_COUNT);
  static double ledStartRadius = ledStepPx / 2;
  void draw(QPainter& painter, uint64_t flash)
  {
    double maxSpanAngle = pwmCycleDegrees;

    for (int i = 0; i < RGB_LED_COUNT; i++)
    {
      if (flash % 4 != i % 4)
        continue;
      double radius2 = ledStartRadius + ledStepPx * i;
      for (int color = 0; color < 3; color++)
      {
        double radius3 = radius2 + color * ledStepPxBetweenRGB;
        painter.setPen(rgbColors[color]);
        uint8_t brightness = rand() % 256;
        double spanAngle = brightness / 256.0 * maxSpanAngle;
        QRectF r(
          fanCenterX - radius3,
          fanCenterY - radius3,
          radius3 * 2,
          radius3 * 2);
        painter.drawArc(r, (globalCurrentAngleDegrees - spanAngle / 2) * 16, spanAngle * 16);
      }
    }
    globalCurrentAngleDegrees += maxSpanAngle;

  }
};

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

void QtFunEmulatorApplication::paintSomethingToBuffer()
{
  // Draw to buffer allocated in QImage
  QImage* buffer = ui.bufferDataWidget->buffer();
  QPainter painter(buffer);
  //int startAngle = 0; //< Value in 1/16th of a degree.
  //int spanAngle = -16 * 180; //< Value in 1/16th of a degree.
  //painter.drawArc(buffer->rect(), startAngle, spanAngle);


  QRectF rectAll(0, 0, width(), height());
  painter.setBrush(QBrush(QColor(0, 0, 0)));
  painter.drawRect(rectAll);//Fill all black
  painter.setBrush(QBrush(QColor(0, 0, 0, 0)));//remove brush

  painter.setPen(Qt::white);
  painter.drawEllipse(QPointF(fanCenterX, fanCenterY), fanRadiusPx, fanRadiusPx);//FAN canvas

  double maxGlobalAngleThisRepaint = counterFrames * fanAngleSpeedDegreesEveryFrame;
  //globalCurrentAngleDegrees = 0;

  for (uint64_t flash = 0; flash < 999999 && globalCurrentAngleDegrees < maxGlobalAngleThisRepaint; flash++)
  {
    DrawMcuPwm::draw(painter, flash);

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