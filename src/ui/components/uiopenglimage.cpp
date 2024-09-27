#include "uiopenglimage.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

UIOpenGLImage::UIOpenGLImage(QWidget *parent)
    : QOpenGLWidget(parent), m_texture(nullptr), m_program(nullptr), m_zoom(1.0f), m_translation(0, 0)
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
    image_width = image.width();
    image_height = image.height();

    makeCurrent();
    if (m_texture) {
        delete m_texture;
    }
    m_texture = new QOpenGLTexture(image);
    m_texture->setMinificationFilter(QOpenGLTexture::Nearest);
    m_texture->setMagnificationFilter(QOpenGLTexture::Nearest);
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
        m_translation = QVector2D(0, 0);
        updateView();
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
        "uniform vec2 textureSize;\n"
        "void main() {\n"
        "    vec2 texelCoord = texCoord0;\n"
        "    gl_FragColor = texture2D(texture, texelCoord);\n"
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
    m_program->setUniformValue("textureSize", QVector2D(m_texture->width(), m_texture->height()));

    // Calculate aspect ratio
    float imageAspect = float(image_width) /float(image_height);
    // Adjust vertex coordinates to maintain aspect ratio
    float x, y;
    if (imageAspect <= 1.0f) {
        // Image is wider than the widget
        x = 1.0f;
        y = imageAspect;
    } else {
        // Image is taller than the widget
        x = imageAspect;
        y = 1.0f;
    }

    GLfloat vertices[] = {
        -x, -y,
         x, -y,
         x,  y,
        -x,  y
    };

    // Correct UV coordinates to flip the image vertically
    GLfloat texCoords[] = {
        0.0f, 1.0f,  // Bottom-left
        1.0f, 1.0f,  // Bottom-right
        1.0f, 0.0f,  // Top-right
        0.0f, 0.0f   // Top-left
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
    if (event->button() == Qt::LeftButton) {
        m_lastPos = event->pos();
    }
}

void UIOpenGLImage::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPointF delta = QPointF(event->pos() - m_lastPos) / (m_zoom * width());
        m_translation += QVector2D(delta.x() * 2, -delta.y() * 2);
        m_lastPos = event->pos();
        updateView();
        update();
    }
}

void UIOpenGLImage::wheelEvent(QWheelEvent *event)
{
    float zoomFactor = 1.0f + event->angleDelta().y() / 1200.0f;

    // Convert mouse position to normalized device coordinates
    QPointF mousePos = event->position();

    QPointF normalizedPos(
        1.0f - (mousePos.x() / width()) * 2.0f,
        (mousePos.y() / height()) * 2.0f - 1.0f
    );

    // Calculate zoom center in world space
    QVector2D zoomCenter(
        normalizedPos.x() / m_zoom + m_translation.x(),
        normalizedPos.y() / m_zoom + m_translation.y()
    );

    // Update zoom
    m_zoom *= zoomFactor;

    // Update translation to keep the zoom center fixed
    m_translation = zoomCenter - QVector2D(normalizedPos.x() / m_zoom, normalizedPos.y() / m_zoom);

    updateView();
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

void UIOpenGLImage::updateView()
{
    m_view.setToIdentity();
    m_view.translate(m_translation.x(), m_translation.y());
}