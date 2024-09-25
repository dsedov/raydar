#include "render_window.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTimer>
#include "components/uiint2.h"
#include "components/uiint.h"
#include "components/uifloat.h"
#include "components/uidropdownmenu.h"

RenderWindow::RenderWindow(settings * settings_ptr, QWidget *parent)
    : QMainWindow(parent), m_width(settings_ptr->image_width), m_height(settings_ptr->image_height), m_gain(300.0f), m_gamma(2.2f)
{
    setWindowTitle("Render Window");
    m_settings_ptr = settings_ptr;
    observer_ptr = new observer(observer::CIE1931_2Deg, spectrum::RESPONSE_SAMPLES, spectrum::START_WAVELENGTH, spectrum::END_WAVELENGTH);
    m_image_buffer = new ImagePNG(m_width, m_height, spectrum::RESPONSE_SAMPLES, observer_ptr);
    m_image = new QImage(m_width, m_height, QImage::Format_RGB888);
    m_image->fill(Qt::black);
    m_gain = 5;
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

    // Create UiFloat for gain and gamma
    m_gainInput = new UiFloat("Gain:", this, 1, 1000, 0.1);
    m_gainInput->setValue(m_gain);
    connect(m_gainInput, &UiFloat::value_changed, this, &RenderWindow::updateGain);

    m_gammaInput = new UiFloat("Gamma:", this,0.1, 10.0, 0.1);
    m_gammaInput->setValue(m_gamma);
    connect(m_gammaInput, &UiFloat::value_changed, this, &RenderWindow::updateGamma);

    // Replace spectrum sampling dropdown with UiDropdownMenu
    QStringList spectrumOptions = {"full", "stochastic"};
    m_spectrumSamplingMenu = new UiDropdownMenu("Spectrum:", spectrumOptions, this);
    connect(m_spectrumSamplingMenu, &UiDropdownMenu::index_changed, this, &RenderWindow::spectrum_sampling_changed);

    // Add samples input
    m_samplesInput = new UiInt("Samples:", this, 1, 8912, 1);
    m_samplesInput->setValue(m_settings_ptr->samples);
    connect(m_samplesInput, &UiInt::value_changed, this, &RenderWindow::samples_changed);

    // Add depth input
    m_depthInput = new UiInt("Depth:", this, 1, 48, 1);
    m_depthInput->setValue(m_settings_ptr->max_depth);
    connect(m_depthInput, &UiInt::value_changed, this, &RenderWindow::updateDepth);

    // Add resolution input
    m_resolutionInput = new UiInt2("Resolution:", this, 16, 16384, 16);
    m_resolutionInput->setValues(m_width, m_height);
    connect(m_resolutionInput, &UiInt2::values_changed, this, &RenderWindow::resolution_changed);
    connect(m_resolutionInput, &UiInt2::values_changed, this, &RenderWindow::update_resolution);

    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    
    QWidget *imageWidget = new QWidget(this);
    QVBoxLayout *imageLayout = new QVBoxLayout(imageWidget);
    imageLayout->addWidget(m_scrollArea);
    imageLayout->addWidget(m_progressBar);
    imageLayout->addWidget(m_metadataLabel);

    QWidget *controlWidget = new QWidget(this);
    QVBoxLayout *controlLayout = new QVBoxLayout(controlWidget);
    controlLayout->addWidget(m_gainInput);
    controlLayout->addWidget(m_gammaInput);
    controlLayout->addWidget(m_spectrumSamplingMenu);  // Add the new UiDropdownMenu
    controlLayout->addWidget(m_samplesInput);
    controlLayout->addWidget(m_depthInput);
    controlLayout->addWidget(m_resolutionInput);
    controlLayout->addStretch(1);

    // Add render button
    m_renderButton = new QPushButton("Render", this);
    connect(m_renderButton, &QPushButton::clicked, this, &RenderWindow::render_requested);
    controlLayout->addWidget(m_renderButton);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(imageWidget);
    m_splitter->addWidget(controlWidget);
    m_splitter->setStretchFactor(0, 3);  // Give more stretch to the image side
    m_splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(m_splitter);
    
    setCentralWidget(centralWidget);
    
    m_imageLabel->setMouseTracking(true);
    m_scrollArea->setMouseTracking(true);
    centralWidget->setMouseTracking(true);
    setMouseTracking(true);
}

void RenderWindow::updateGain(float value)
{
    m_gain = value;
    update_image();
}

void RenderWindow::updateGamma(float value)
{
    m_gamma = value;
    update_image();
}

void RenderWindow::updateSamples(int value)
{
    // Implement the logic for updating samples
}

void RenderWindow::updateDepth(int value)
{
    // Implement the logic for updating depth
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
void RenderWindow::update_resolution(int width, int height)
{
    m_width = width;
    m_height = height;
    m_image_buffer = new ImagePNG(m_width, m_height, spectrum::RESPONSE_SAMPLES, observer_ptr);
    m_image = new QImage(m_width, m_height, QImage::Format_RGB888);
    m_image->fill(Qt::black);
    update_image();
}
void RenderWindow::updateImageLabelSize()
{
    QSize scrollAreaSize = m_scrollArea->viewport()->size();
    QSize imageSize = m_image->size();
    QSize newSize = imageSize.scaled(scrollAreaSize, Qt::KeepAspectRatio);
    m_imageLabel->setFixedSize(newSize);
    m_imageLabel->setPixmap(QPixmap::fromImage(*m_image).scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
