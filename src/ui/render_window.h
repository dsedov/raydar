#ifndef RENDER_WINDOW_H
#define RENDER_WINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QMap>
#include <QScrollArea>
#include <QResizeEvent>

class RenderWindow : public QMainWindow
{
public:
    RenderWindow(int width, int height, QWidget *parent = nullptr)
        : QMainWindow(parent), m_width(width), m_height(height)
    {
        setWindowTitle("Render Window");
        
        m_image = new QImage(m_width, m_height, QImage::Format_RGB888);
        m_image->fill(Qt::black);
        
        m_imageLabel = new QLabel(this);
        m_imageLabel->setPixmap(QPixmap::fromImage(*m_image));
        m_imageLabel->setScaledContents(true);

        m_scrollArea = new QScrollArea(this);
        m_scrollArea->setWidget(m_imageLabel);
        m_scrollArea->setWidgetResizable(true);
        
        m_metadataLabel = new QLabel(this);
        m_metadataLabel->setAlignment(Qt::AlignCenter);
        m_metadataLabel->setText("Hover over the image to see metadata");
        
        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *layout = new QVBoxLayout(centralWidget);
        layout->addWidget(m_scrollArea);
        layout->addWidget(m_metadataLabel);
        
        setCentralWidget(centralWidget);
        
        m_imageLabel->setMouseTracking(true);
        m_scrollArea->setMouseTracking(true);
        centralWidget->setMouseTracking(true);
        setMouseTracking(true);

        resize(800, 600); // Set an initial size for the window
        updateImageLabelSize(); // Ensure the image is sized correctly on startup
    }
    
    void updatePixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, const QString& metadata)
    {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height)
            return;
        
        m_image->setPixelColor(x, y, QColor(r, g, b));
        m_metadata[QString("%1,%2").arg(x).arg(y)] = metadata;
        
        m_imageLabel->setPixmap(QPixmap::fromImage(*m_image));
    }
    
protected:
    void mouseMoveEvent(QMouseEvent *event) override
    {
        QPoint pos = m_scrollArea->viewport()->mapFrom(this, event->pos());
        pos = m_imageLabel->mapFrom(m_scrollArea->viewport(), pos);
        QRect scaledRect = m_imageLabel->rect();

        if (scaledRect.contains(pos))
        {
            // Calculate the scaling factors
            double scaleX = static_cast<double>(m_width) / scaledRect.width();
            double scaleY = static_cast<double>(m_height) / scaledRect.height();

            // Map the position to the original image coordinates
            int originalX = static_cast<int>(pos.x() * scaleX);
            int originalY = static_cast<int>(pos.y() * scaleY);

            QString key = QString("%1,%2").arg(originalX).arg(originalY);
            if (m_metadata.contains(key))
            {
                m_metadataLabel->setText(m_metadata[key]);
            }
            else
            {
                m_metadataLabel->setText(QString("Coordinates: (%1, %2)").arg(originalX).arg(originalY));
            }
        }
        else
        {
            m_metadataLabel->setText("Hover over the image to see metadata");
        }
    }

    void resizeEvent(QResizeEvent *event) override
    {
        QMainWindow::resizeEvent(event);
        updateImageLabelSize();
    }

private:
    void updateImageLabelSize()
    {
        QSize scrollAreaSize = m_scrollArea->viewport()->size();
        QSize imageSize = m_image->size();
        QSize newSize = imageSize.scaled(scrollAreaSize, Qt::KeepAspectRatio);
        m_imageLabel->setFixedSize(newSize);
        m_imageLabel->setPixmap(QPixmap::fromImage(*m_image).scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    int m_width;
    int m_height;
    QImage *m_image;
    QLabel *m_imageLabel;
    QScrollArea *m_scrollArea;
    QLabel *m_metadataLabel;
    QMap<QString, QString> m_metadata;
};

#endif // RENDER_WINDOW_H
