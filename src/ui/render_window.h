#ifndef RENDER_WINDOW_H
#define RENDER_WINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QImage>
#include <QScrollArea>
#include <QMap>
#include <QProgressBar> // Assuming this is needed for the progress bar
#include <QSlider>
#include <QSplitter>
#include <QPushButton>
#include "../image/image_png.h"

class RenderWindow : public QMainWindow
{
    Q_OBJECT
public:
    RenderWindow(int width, int height, QWidget *parent = nullptr);
    void updatePixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, const QString& metadata);

public slots:
    void updateProgress(int progress, int total);
    void updateBucket(int x, int y, ImagePNG* image);
    void updateGain(int value);
    void updateGamma(int value);

signals:
    void render_requested();
protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateImageLabelSize();
    void setupUI();

    int m_width;
    int m_height;
    QImage *m_image;
    QLabel *m_imageLabel;
    QScrollArea *m_scrollArea;
    QLabel *m_metadataLabel;
    QMap<QString, QString> m_metadata;
    QProgressBar *m_progressBar;
    observer * observer_ptr;
    ImagePNG * m_image_buffer;
    QSlider *m_gainSlider;
    QSlider *m_gammaSlider;
    QLabel *m_gainLabel;
    QLabel *m_gammaLabel;
    float m_gain;
    float m_gamma;
    QSplitter *m_splitter;
    QPushButton *m_renderButton;  // New render button
    void update_image();
};

#endif // RENDER_WINDOW_H
