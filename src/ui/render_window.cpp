#include "render_window.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QProgressBar>
#include <QTimer>

RenderWindow::RenderWindow(int width, int height, QWidget *parent)
    : QMainWindow(parent), m_width(width), m_height(height), m_gain(1.0f), m_gamma(2.2f)
{
    setWindowTitle("Render Window");
    observer_ptr = new observer(observer::CIE1931_2Deg, spectrum::RESPONSE_SAMPLES, spectrum::START_WAVELENGTH, spectrum::END_WAVELENGTH);
    m_image_buffer = new ImagePNG(m_width, m_height, spectrum::RESPONSE_SAMPLES, observer_ptr);
    m_image = new QImage(m_width, m_height, QImage::Format_RGB888);
    m_image->fill(Qt::black);
    
    setupUI();

    resize(1000, 600);
    updateImageLabelSize();

    // Run update_image on a timer every second
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &RenderWindow::update_image);
    timer->start(1000);
}

void RenderWindow::setupUI()
{
    m_imageLabel = new QLabel(this);
    m_imageLabel->setPixmap(QPixmap::fromImage(*m_image));
    m_imageLabel->setScaledContents(true);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidget(m_imageLabel);
    m_scrollArea->setWidgetResizable(true);
    
    m_metadataLabel = new QLabel(this);
    m_metadataLabel->setAlignment(Qt::AlignCenter);
    m_metadataLabel->setText("Hover over the image to see metadata");
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);

    // Create sliders
    m_gainSlider = new QSlider(Qt::Horizontal, this);
    m_gainSlider->setRange(100, 100000);  // 1.0 to 100.0
    m_gainSlider->setValue(30000); 
    m_gain = 300.0f;
    connect(m_gainSlider, &QSlider::valueChanged, this, &RenderWindow::updateGain);

    

    m_gammaSlider = new QSlider(Qt::Horizontal, this);
    m_gammaSlider->setRange(0, 1000);  // 0.0 to 10.0
    m_gammaSlider->setValue(220); 
    m_gamma = 2.2f;
    connect(m_gammaSlider, &QSlider::valueChanged, this, &RenderWindow::updateGamma);


    m_gainLabel = new QLabel("Gain: 300.0", this);
    m_gammaLabel = new QLabel("Gamma: 2.2", this);

    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    
    QVBoxLayout *imageLayout = new QVBoxLayout();
    imageLayout->addWidget(m_scrollArea);
    imageLayout->addWidget(m_progressBar);
    imageLayout->addWidget(m_metadataLabel);

    QVBoxLayout *sliderLayout = new QVBoxLayout();
    sliderLayout->addWidget(m_gainLabel);
    sliderLayout->addWidget(m_gainSlider);
    sliderLayout->addWidget(m_gammaLabel);
    sliderLayout->addWidget(m_gammaSlider);

    // create spacer
    QSpacerItem *spacer = new QSpacerItem(10, 100, QSizePolicy::Expanding, QSizePolicy::Expanding);
    sliderLayout->addItem(spacer);

    mainLayout->addLayout(imageLayout);
    mainLayout->addLayout(sliderLayout);
    
    setCentralWidget(centralWidget);
    
    m_imageLabel->setMouseTracking(true);
    m_scrollArea->setMouseTracking(true);
    centralWidget->setMouseTracking(true);
    setMouseTracking(true);
}

void RenderWindow::updateGain(int value)
{
    m_gain = value / 100.0f;
    m_gainLabel->setText(QString("Gain: %1").arg(m_gain, 0, 'f', 2));
    update_image();
}

void RenderWindow::updateGamma(int value)
{
    m_gamma = value / 100.0f;
    m_gammaLabel->setText(QString("Gamma: %1").arg(m_gamma, 0, 'f', 2));
    update_image();
}

void RenderWindow::update_image()
{
    static const interval intensity(0.000, 0.999);
    for (int i = 0; i < m_image_buffer->width(); i++) {
        for (int j = 0; j < m_image_buffer->height(); j++) {
            spectrum color_spectrum = m_image_buffer->get_pixel(i, j) / ( m_gain * m_gain);
            color color_rgb = color_spectrum.to_rgb(observer_ptr);
            float r = intensity.clamp(linear_to_gamma2(color_rgb.x(), m_gamma));
            float g = intensity.clamp(linear_to_gamma2(color_rgb.y(), m_gamma));
            float b = intensity.clamp(linear_to_gamma2(color_rgb.z(), m_gamma));
            m_image->setPixelColor(i, j, QColor::fromRgbF(r, g, b));
        }
    }
    updateImageLabelSize();
}

void RenderWindow::updateProgress(int progress, int total)
{
    int percentage = static_cast<int>((static_cast<double>(progress) / total) * 100);
    m_progressBar->setValue(percentage);
}

void RenderWindow::updateBucket(int x, int y, ImagePNG* image)
{
    
    for (int i = 0; i < image->width(); i++) {
        for (int j = 0; j < image->height(); j++) {
            m_image_buffer->set_pixel(x + i, y + j, image->get_pixel(i, j));
        }
    }
}

void RenderWindow::updatePixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, const QString& metadata)
{
    if (x < 0 || x >= m_width || y < 0 || y >= m_height)
        return;
    
    m_image->setPixelColor(x, y, QColor(r, g, b));
    m_metadata[QString("%1,%2").arg(x).arg(y)] = metadata;
    
    m_imageLabel->setPixmap(QPixmap::fromImage(*m_image));
}

void RenderWindow::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = m_scrollArea->viewport()->mapFrom(this, event->pos());
    pos = m_imageLabel->mapFrom(m_scrollArea->viewport(), pos);
    QRect scaledRect = m_imageLabel->rect();

    if (scaledRect.contains(pos))
    {
        double scaleX = static_cast<double>(m_width) / scaledRect.width();
        double scaleY = static_cast<double>(m_height) / scaledRect.height();

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

void RenderWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateImageLabelSize();
}

void RenderWindow::updateImageLabelSize()
{
    QSize scrollAreaSize = m_scrollArea->viewport()->size();
    QSize imageSize = m_image->size();
    QSize newSize = imageSize.scaled(scrollAreaSize, Qt::KeepAspectRatio);
    m_imageLabel->setFixedSize(newSize);
    m_imageLabel->setPixmap(QPixmap::fromImage(*m_image).scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
