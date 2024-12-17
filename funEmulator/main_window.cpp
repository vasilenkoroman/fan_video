#include "main_window.h"

#include <QPainter>
#include <QImage>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QStandardPaths>
#include <QDir>

#include <QtOpenGL/QOpenGLFramebufferObject>
#include <QtOpenGL/QOpenGLPaintDevice>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFunctions>

#include "DrawWs2812.h"

const int kThreads = 8;

//#define USE_OPENGL_PAINTER

QtFunEmulatorApplication::QtFunEmulatorApplication(QWidget* parent)
  : QMainWindow(parent)
{
  QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  m_settingsFileName = QDir(homePath).filePath("pwm_settings.json");

  ui.setupUi(this);

  loadSettingsFromFile();
  loadSettingsToUi();

  connect(&m_timer, &QTimer::timeout, [this]() { paintFan(); });

  connect(ui.spinBoxFps, &QSpinBox::valueChanged, this, [this]() { restartTimer(); });

  connect(ui.spinBoxBladeCount, &QSpinBox::valueChanged, this, [this]() { readSettingsFromUi(); });
  connect(ui.spinBoxColorBitDepth, &QSpinBox::valueChanged, this, [this]() { readSettingsFromUi(); });
  connect(ui.spinBoxCpuFrequencyHz, &QSpinBox::valueChanged, this, [this]() { readSettingsFromUi(); });
  connect(ui.spinBoxCpuFrequencyDivider, &QSpinBox::valueChanged, this, [this]() { readSettingsFromUi(); });
  connect(ui.spinBoxPwmHwOutputs, &QSpinBox::valueChanged, this, [this]() { readSettingsFromUi(); });
  connect(ui.spinBoxChannelsStrobe, &QSpinBox::valueChanged, this, [this]() { readSettingsFromUi(); });
  connect(ui.spinBoxColorCount, &QSpinBox::valueChanged, this, [this]() { readSettingsFromUi(); });
  connect(ui.doubleSpinBoxLedStepPx, &QDoubleSpinBox::valueChanged, this, [this]() { readSettingsFromUi(); });
  connect(ui.comboBoxPwmAlign, &QComboBox::currentIndexChanged, this, [this]() { readSettingsFromUi(); });

  connect(ui.resetToDefaultButton, &QPushButton::clicked,
      this, [this]()
      {
          m_settings = {};
          m_updating = true;
          loadSettingsToUi();
          saveSettingsToFile();
          m_updating = false;
      });

  restartTimer();
}

enum UI_TABS { UI_TAB_MCU_PWM, UI_TAB_WS2118 };
UI_TABS uiTab = UI_TAB_MCU_PWM;

struct DrawContext
{
    QSize size;
    int fps = 0;
    uint64_t frameNumber = 0;
    DrawMcuPwm::McuPwmSettings settingsPwm;

};

/* Every frame, every 1 fps, every repaint counterFrames++.
must be set 0 every settings change*/
QImage drawAsync(const DrawContext& context)
{

#ifdef USE_OPENGL_PAINTER
    QOpenGLContext glContext;
    glContext.create();

    QOffscreenSurface surface;
    surface.setFormat(glContext.format());
    surface.create();

    glContext.makeCurrent(&surface);

    // Step 2: Create an OpenGL framebuffer object (FBO)
    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    QOpenGLFramebufferObject fbo(context.size.width(), context.size.height(), fboFormat);
    fbo.bind();

    // Step 3: Clear the framebuffer with OpenGL (background color)
    QOpenGLFunctions* gl = glContext.functions();
    gl->glViewport(0, 0, context.size.width(), context.size.height());
    gl->glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QOpenGLPaintDevice device(context.size);
    QPainter painter(&device);

#else
    // Draw to buffer allocated in QImage
    QImage buffer(context.size.width(), context.size.height(), QImage::Format_RGBA64_Premultiplied);
    QPainter painter(&buffer);
#endif

    QRectF rectAll(0, 0, context.size.width(), context.size.height());
    painter.setBrush(QBrush(QColor(0, 0, 0)));
    painter.drawRect(rectAll);//Fill all black
    painter.setBrush(QBrush(QColor(0, 0, 0, 0)));//remove brush

    painter.setPen(Qt::white);

    const int fanCenter = context.size.width() / 2;
    const double fanRadiusPx = fanCenter * 0.99;
    painter.drawEllipse(QPointF(fanCenter, fanCenter), fanRadiusPx, fanRadiusPx);//FAN canvas

    painter.setCompositionMode(QPainter::CompositionMode_Plus);

    if (uiTab == UI_TAB_MCU_PWM)
        DrawMcuPwm::draw(painter, context.settingsPwm,/*context.fps,*/ context.frameNumber, fanCenter, fanRadiusPx);
    else if (uiTab == UI_TAB_WS2118)
        DrawWs2812::draw();

    painter.end();

#ifdef USE_OPENGL_PAINTER
    return fbo.toImage().mirrored();
#else
    return buffer;
#endif
}

void QtFunEmulatorApplication::loadSettingsToUi()
{
    ui.spinBoxColorBitDepth->setValue(m_settings.colorBitDepth);
    ui.spinBoxColorCount->setValue(m_settings.colorCount);
    ui.spinBoxCpuFrequencyDivider->setValue(m_settings.cpuFrequencyDivider);
    ui.spinBoxCpuFrequencyHz->setValue(m_settings.cpuFrequencyHz / 1000000);
    ui.doubleSpinBoxLedStepPx->setValue(m_settings.ledStepPxBetweenRGB);
    ui.spinBoxChannelsStrobe->setValue(m_settings.mcuPwmCountChannelsStrobe);
    ui.spinBoxPwmHwOutputs->setValue(m_settings.mcuPwmHwOutputs);
    ui.comboBoxPwmAlign->setCurrentIndex(m_settings.pwmAlign);

    updateReadOnlyUiControls();
}

void QtFunEmulatorApplication::updateReadOnlyUiControls()
{
    ui.lineEditCycleLengthTicks->setValue(m_settings.getMcuPwmCycleLengthTicks());
    ui.lineEditPwmTickHz->setValue(m_settings.getMcuPwmTickHz());
    ui.lineEditPwmTickUSec->setValue(m_settings.getMcuPwmTickUSec());
    ui.lineEditPwmCycleHz->setValue(m_settings.getMcuPwmCycleHz());
    ui.lineEditPwmCycleUSec->setValue(m_settings.getMcuPwmCycleUSec());
}

void QtFunEmulatorApplication::readSettingsFromUi()
{
    m_settings.colorBitDepth = ui.spinBoxColorBitDepth->value();
    m_settings.colorCount = ui.spinBoxColorCount->value();
    m_settings.cpuFrequencyDivider = ui.spinBoxCpuFrequencyDivider->value();
    m_settings.cpuFrequencyHz = ui.spinBoxCpuFrequencyHz->value() * 1000000;
    m_settings.ledStepPxBetweenRGB = ui.doubleSpinBoxLedStepPx->value();
    m_settings.mcuPwmCountChannelsStrobe = ui.spinBoxChannelsStrobe->value();
    m_settings.mcuPwmHwOutputs = ui.spinBoxPwmHwOutputs->value();
    m_settings.pwmAlign = (DrawMcuPwm::McuPwmSettings::PWM_ALIGNS) ui.comboBoxPwmAlign->currentIndex();

    updateReadOnlyUiControls();

    if (!m_updating)
        saveSettingsToFile();
}

void QtFunEmulatorApplication::saveSettingsToFile()
{
    QJsonObject jsonObj;
    jsonObj["colorBitDepth"] = (qint64) m_settings.colorBitDepth;
    jsonObj["colorCount"] = (qint64) m_settings.colorCount;
    jsonObj["cpuFrequencyDivider"] = (qint64) m_settings.cpuFrequencyDivider;
    jsonObj["cpuFrequencyHz"] = (qint64) m_settings.cpuFrequencyHz;
    jsonObj["ledStepPxBetweenRGB"] = m_settings.ledStepPxBetweenRGB;
    jsonObj["mcuPwmCountChannelsStrobe"] = m_settings.mcuPwmCountChannelsStrobe;
    jsonObj["mcuPwmHwOutputs"] = m_settings.mcuPwmHwOutputs;
    jsonObj["pwmAlign"] = m_settings.pwmAlign;

    QJsonDocument jsonDoc(jsonObj);
    QFile file(m_settingsFileName);
    if (file.open(QIODeviceBase::WriteOnly | QIODeviceBase::Truncate))
        file.write(jsonDoc.toJson(QJsonDocument::Indented));
}

void QtFunEmulatorApplication::loadSettingsFromFile()
{
    QFile file(m_settingsFileName);
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray jsonData = file.readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        if (!jsonDoc.isNull() && jsonDoc.isObject())
        {
            const auto jsonObj = jsonDoc.object();
            m_settings.colorBitDepth = jsonObj["colorBitDepth"].toInteger();
            m_settings.colorCount = jsonObj["colorCount"].toInteger();
            m_settings.cpuFrequencyDivider = jsonObj["cpuFrequencyDivider"].toInteger();
            m_settings.cpuFrequencyHz = jsonObj["cpuFrequencyHz"].toInteger();
            m_settings.ledStepPxBetweenRGB = jsonObj["ledStepPxBetweenRGB"].toDouble();
            m_settings.mcuPwmCountChannelsStrobe = jsonObj["mcuPwmCountChannelsStrobe"].toInteger();
            m_settings.mcuPwmHwOutputs = jsonObj["mcuPwmHwOutputs"].toInteger();
            m_settings.pwmAlign = (DrawMcuPwm::McuPwmSettings::PWM_ALIGNS) jsonObj["pwmAlign"].toInteger();
        }
    }
}

void QtFunEmulatorApplication::paintFan()
{
    DrawContext context {
        ui.bufferDataWidget->size(),
        ui.spinBoxFps->value(),
        m_frameCounter++,
        m_settings
    };

#ifdef USE_OPENGL_PAINTER
    // Multi-thread painting is not implemented for openGL version yet.
    ui.bufferDataWidget->setBuffer(drawAsync(context));
#else
    m_asyncDrawTask.push_back(std::async(
        std::launch::async, [context] { return drawAsync(context); }));

    while (m_asyncDrawTask.size() >= kThreads)
    {
        QImage image = m_asyncDrawTask.front().get();
        m_asyncDrawTask.pop_front();
        ui.bufferDataWidget->setBuffer(std::move(image));
    }
#endif
}

void QtFunEmulatorApplication::restartTimer()
{
  m_timer.stop();
  m_timer.start(1000 / ui.spinBoxFps->value());
}