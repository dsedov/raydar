#ifndef UIOPENGLIMAGE_H
#define UIOPENGLIMAGE_H

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QtOpenGL/QOpenGLTexture>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QPoint>

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
    QOpenGLShaderProgram *m_program;
    QMatrix4x4 m_projection;
    QMatrix4x4 m_view;
    QPoint m_lastPos;
    float m_zoom;

    void updateProjection();
};

#endif // UIOPENGLIMAGE_H