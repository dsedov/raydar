#include "uiopenglimage.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

UIOpenGLImage::UIOpenGLImage(QWidget *parent)
    : QOpenGLWidget(parent), m_texture(nullptr), m_program(nullptr), m_zoom(1.0f)
{
    setFocusPolicy(Qt::StrongFocus);
}

UIOpenGLImage::~UIOpenGLImage()
{
    makeCurrent();
    delete m_texture;
    delete m_program;
    doneCurrent();
}

void UIOpenGLImage::setImage(const QImage &image)
{
    makeCurrent();
    if (m_texture) {
        delete m_texture;
    }
    m_texture = new QOpenGLTexture(image.mirrored());
    doneCurrent();
    update();
}

void UIOpenGLImage::fitInView()
{
    if (m_texture) {
        float widgetAspect = float(width()) / height();
        float imageAspect = float(m_texture->width()) / m_texture->height();
        if (widgetAspect > imageAspect) {
            m_zoom = float(height()) / m_texture->height();
        } else {
            m_zoom = float(width()) / m_texture->width();
        }
        m_view.setToIdentity();
        updateProjection();
        update();
    }
}

void UIOpenGLImage::initializeGL()
{
    initializeOpenGLFunctions();

    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
        "attribute vec2 vertex;\n"
        "attribute vec2 texCoord;\n"
        "varying vec2 texCoord0;\n"
        "uniform mat4 mvp;\n"
        "void main() {\n"
        "    gl_Position = mvp * vec4(vertex, 0.0, 1.0);\n"
        "    texCoord0 = texCoord;\n"
        "}\n");
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
        "varying vec2 texCoord0;\n"
        "uniform sampler2D texture;\n"
        "void main() {\n"
        "    gl_FragColor = texture2D(texture, texCoord0);\n"
        "}\n");
    m_program->bindAttributeLocation("vertex", 0);
    m_program->bindAttributeLocation("texCoord", 1);
    m_program->link();

    m_view.setToIdentity();
}

void UIOpenGLImage::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    if (!m_texture) return;

    m_program->bind();
    m_texture->bind();

    QMatrix4x4 mvp = m_projection * m_view;
    m_program->setUniformValue("mvp", mvp);
    m_program->setUniformValue("texture", 0);

    GLfloat vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    GLfloat texCoords[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f
    };

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texCoords);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    m_texture->release();
    m_program->release();
}

void UIOpenGLImage::resizeGL(int w, int h)
{
    updateProjection();
}

void UIOpenGLImage::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        m_lastPos = event->pos();
    }
}

void UIOpenGLImage::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPointF delta = QPointF(event->pos() - m_lastPos) / (m_zoom * width());
        m_view.translate(delta.x() * 2, -delta.y() * 2);
        m_lastPos = event->pos();
        update();
    }
}

void UIOpenGLImage::wheelEvent(QWheelEvent *event)
{
    float zoomFactor = 1.0f + event->angleDelta().y() / 1200.0f;
    m_zoom *= zoomFactor;

    QPointF mousePos = event->position() / width() * 2.0f - QPointF(1.0f, 1.0f);
    mousePos.setY(-mousePos.y());
    
    m_view.translate(mousePos.x(), mousePos.y());
    m_view.scale(zoomFactor, zoomFactor);
    m_view.translate(-mousePos.x(), -mousePos.y());

    updateProjection();
    update();
}

void UIOpenGLImage::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F) {
        fitInView();
    }
}

void UIOpenGLImage::updateProjection()
{
    float aspect = float(width()) / height();
    m_projection.setToIdentity();
    m_projection.ortho(-aspect / m_zoom, aspect / m_zoom, -1.0f / m_zoom, 1.0f / m_zoom, -1.0f, 1.0f);
}