#include "main_window.h"

#include <QPainter>
#include <QImage>

#include <QtOpenGL/QOpenGLFramebufferObject>
#include <QtOpenGL/QOpenGLPaintDevice>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFunctions>

#include "DrawMcuPwm.h"
#include "DrawWs2812.h"

const int kThreads = 8;

//#define USE_OPENGL_PAINTER

QtFunEmulatorApplication::QtFunEmulatorApplication(QWidget* parent)
  : QMainWindow(parent)
{
  ui.setupUi(this);

  connect(&m_timer, &QTimer::timeout, [this]() { paintFan(); });

  connect(ui.spinBoxFps, &QSpinBox::valueChanged, this, [this]() { restartTimer(); });
  restartTimer();
}

enum UI_TABS { UI_TAB_MCU_PWM, UI_TAB_WS2118 };
UI_TABS uiTab = UI_TAB_MCU_PWM;

struct DrawContext
{
    QSize size;
    int fps = 0;
    uint64_t frameNumber = 0;
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
        DrawMcuPwm::draw(painter, /*context.fps,*/ context.frameNumber, fanCenter, fanRadiusPx);
    else if (uiTab == UI_TAB_WS2118)
        DrawWs2812::draw();

    painter.end();

#ifdef USE_OPENGL_PAINTER
    return fbo.toImage().mirrored();
#else
    return buffer;
#endif

    //return buffer;
}

void QtFunEmulatorApplication::paintFan()
{
    DrawContext context {
        ui.bufferDataWidget->size(),
        ui.spinBoxFps->value(),
        m_frameCounter++};

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