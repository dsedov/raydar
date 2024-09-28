#ifndef UIOPENGLIMAGE_H
#define UIOPENGLIMAGE_H

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QtOpenGL/QOpenGLTexture>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QPoint>
#include <QVector2D>
#include <QPainter>
#include <QFontMetrics>

class UIOpenGLImage : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit UIOpenGLImage(QWidget *parent = nullptr);
    ~UIOpenGLImage();

    void setImage(const QImage &image);
    void fitInView();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QOpenGLTexture *m_texture;
    QImage *m_image;
    QOpenGLShaderProgram *m_program;
    QOpenGLShaderProgram *m_textProgram;  // Add this line
    QMatrix4x4 m_projection;
    QMatrix4x4 m_view;
    QPoint m_lastPos;
    QPoint m_deltaPos;
    float m_zoom;
    QVector2D m_translation;
    QOpenGLTexture* m_textTexture;
    QPointF m_mousePos;
    
    void updateProjection();
    void updateView();
    void updateTextTexture(const QString& text);
    void renderText(const QString& text, int x, int y);
    QPointF screenToImageCoordinates(const QPointF& screenPos);

    int image_width;
    int image_height;
};

#endif // UIOPENGLIMAGE_H