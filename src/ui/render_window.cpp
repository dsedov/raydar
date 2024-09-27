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
#include "components/uiopenglimage.h"

RenderWindow::RenderWindow(settings * settings_ptr, QWidget *parent)
    : QMainWindow(parent), m_width(settings_ptr->image_width), m_height(settings_ptr->image_height), m_gain(300.0f), m_gamma(2.2f)
{
    setWindowTitle("Render Window");
    m_settings_ptr = settings_ptr;
    observer_ptr = new observer(observer::CIE1931_2Deg, spectrum::RESPONSE_SAMPLES, spectrum::START_WAVELENGTH, spectrum::END_WAVELENGTH);
    m_image_buffer = new ImagePNG(m_width, m_height, spectrum::RESPONSE_SAMPLES, observer_ptr);
    m_image = new QImage(m_width, m_height, QImage::Format_RGB888);
    m_image->fill(Qt::black);
    m_gain = settings_ptr->gain;
    m_gamma = settings_ptr->gamma;
    m_whitebalance = 5400.0f;

    setupUI();

    resize(1000, 600);

    // Run update_image on a timer every second
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &RenderWindow::update_image);
    timer->start(1000);
}

void RenderWindow::setupUI()
{
    
    this->setStyleSheet(style_sheet());
        
    m_openGLImage = new UIOpenGLImage(this);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);

    // Create UiFloat for gain and gamma
    m_gainInput = new UiFloat("Gain:", this, 0.1, 1000, 0.1);
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
    connect(m_depthInput, &UiInt::value_changed, this, &RenderWindow::depth_changed);

    // Add resolution input
    m_resolutionInput = new UiInt2("Resolution:", this, 16, 16384, 16);
    m_resolutionInput->setValues(m_width, m_height);
    connect(m_resolutionInput, &UiInt2::values_changed, this, &RenderWindow::resolution_changed);
    connect(m_resolutionInput, &UiInt2::values_changed, this, &RenderWindow::update_resolution);

    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    
    QWidget *imageWidget = new QWidget(this);
    QVBoxLayout *imageLayout = new QVBoxLayout(imageWidget);
    imageLayout->addWidget(m_openGLImage, 1);  // Add stretch factor of 1
    imageLayout->addWidget(m_progressBar);

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
    
    m_openGLImage->setMouseTracking(true);
    centralWidget->setMouseTracking(true);
    setMouseTracking(true);
}

void RenderWindow::updateGain(float value)
{
    m_gain = value;
    need_to_update_image = true;
    update_image();
}

void RenderWindow::updateGamma(float value)
{
    m_gamma = value;
    need_to_update_image = true;
    update_image();
}

void RenderWindow::update_image()
{
    if(!need_to_update_image) return;
    need_to_update_image = false;
    static const interval intensity(0.000, 0.999);

    QImage updatedImage(m_width, m_height, QImage::Format_RGB888);

    for (int i = 0; i < m_image_buffer->width(); i++) {
        for (int j = 0; j < m_image_buffer->height(); j++) {
            spectrum color_spectrum = m_image_buffer->get_pixel(i, j) / ( m_gain * m_gain);
            color color_rgb = color_spectrum.to_rgb(observer_ptr);
            float r = int(255.999 * intensity.clamp(linear_to_gamma2(color_rgb.x(), m_gamma))); 
            float g = int(255.999 * intensity.clamp(linear_to_gamma2(color_rgb.y(), m_gamma)));
            float b = int(255.999 * intensity.clamp(linear_to_gamma2(color_rgb.z(), m_gamma)));
            updatedImage.setPixelColor(i, j, QColor::fromRgb(r, g, b));
        }
    }
    
    m_openGLImage->setImage(updatedImage);
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
    need_to_update_image = true;
}



void RenderWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}
void RenderWindow::update_resolution(int width, int height)
{
    m_width = width;
    m_height = height;
    m_image_buffer = new ImagePNG(m_width, m_height, spectrum::RESPONSE_SAMPLES, observer_ptr);
    m_image = new QImage(m_width, m_height, QImage::Format_RGB888);
    m_image->fill(Qt::black);
    need_to_update_image = true;
    update_image();
}
QString RenderWindow::style_sheet()
{
    return "* { background-color: #323232; } \
    QLabel { color: #bababa; } \
    QPushButton { background-color: #515151; color: #bababa; border-radius: 5px; border: 1px solid #1e1e1e; padding: 5px; } \
    QPushButton:hover { background-color: #616161; } \
    QPushButton:pressed { background-color: #414141; } \
    QDoubleSpinBox, QSpinBox { background-color: #252525; color: #bababa; border-radius: 5px; border: 1px solid #1e1e1e; padding: 5px; } \
    QDoubleSpinBox::up-button, QSpinBox::up-button { background-color: #3a3a3a; color: #bababa; border-top-right-radius: 5px; margin-right: 1px; } \
    QDoubleSpinBox::down-button, QSpinBox::down-button { background-color: #3a3a3a; color: #bababa; border-bottom-right-radius: 5px; margin-right: 1px; } \
    QComboBox { background-color: #3a3a3a; color: #bababa; border-radius: 5px; border: 1px solid #1e1e1e; padding: 5px; } \
    QComboBox::drop-down { background-color: #3a3a3a; color: #bababa;  border: 1px solid #3a3a3a; border-radius: 5px; margin-right: 1px;  } \
    QProgressBar { background-color: #252525; color: #bababa; border-radius: 5px; border: 1px solid #1e1e1e; padding: 5px; text-align: center; padding:1px; } \
    QProgressBar::chunk { background-color: #515151; } \
    ";
}
