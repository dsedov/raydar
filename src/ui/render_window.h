#ifndef RENDER_WINDOW_H
#define RENDER_WINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QImage>
#include <QScrollArea>
#include <QMap>
#include <QProgressBar> // Assuming this is needed for the progress bar

class RenderWindow : public QMainWindow
{
    Q_OBJECT
public:
    RenderWindow(int width, int height, QWidget *parent = nullptr);
    void updatePixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, const QString& metadata);

public slots:
    void updateProgress(int progress, int total);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;


private:
    void updateImageLabelSize();

    int m_width;
    int m_height;
    QImage *m_image;
    QLabel *m_imageLabel;
    QScrollArea *m_scrollArea;
    QLabel *m_metadataLabel;
    QMap<QString, QString> m_metadata;
    QProgressBar *m_progressBar;

    void setupConnections() {

    }
};

#endif // RENDER_WINDOW_H