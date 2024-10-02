#include "render_window.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QTabWidget>
#include "components/uiint2.h"
#include "components/uiint.h"
#include "components/uifloat.h"
#include "components/uidropdownmenu.h"
#include "components/uiopenglimage.h"
#include "components/uispectralgraph.h"
#include "components/usd_tree_component.h"
#include "components/spd_file_list_component.h"


RenderWindow::RenderWindow(settings * settings_ptr, rd::usd::loader * loader, QWidget *parent)
    : QMainWindow(parent), m_width(settings_ptr->image_width), m_height(settings_ptr->image_height), m_exposure(1.0f), m_gamma(2.2f)
{
    setWindowTitle("Render Window");
    m_settings_ptr = settings_ptr;
    m_loader = loader;
    observer_ptr = new observer(observer::CIE1931_2Deg, spectrum::RESPONSE_SAMPLES, spectrum::START_WAVELENGTH, spectrum::END_WAVELENGTH);
    m_image_buffer = new ImageSPD(m_width, m_height, spectrum::RESPONSE_SAMPLES, observer_ptr);
    m_image = new QImage(m_width, m_height, QImage::Format_RGB888);
    m_image->fill(Qt::black);
    m_exposure = settings_ptr->exposure;
    m_gamma = settings_ptr->gamma;
    m_whitebalance = 5400.0f;

    setupUI();

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int width = screenGeometry.width();  // 90% of screen width
    int height = screenGeometry.height() * 0.9;  // 90% of screen height
    resize(width, height);

    // Run update_image on a timer every second
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &RenderWindow::update_image);
    timer->start(1000);
}

void RenderWindow::setupUI()
{
    
    this->setStyleSheet(style_sheet());

    // Create the USD Tree Component
    m_usdTreeComponent = new USDTreeComponent(m_loader->get_stage(), this);

    // Create the SPD File List Component
    m_spdFileListComponent = new SPDFileListComponent(this);
    connect(m_spdFileListComponent, &SPDFileListComponent::fileSelected, this, &RenderWindow::onSPDFileSelected);
    connect(m_spdFileListComponent, &SPDFileListComponent::saveRequested, this, &RenderWindow::onSaveClicked);

    // Create the tab widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(m_usdTreeComponent, "USD");
    m_tabWidget->addTab(m_spdFileListComponent, "History");

    m_tabWidget->setTabPosition(QTabWidget::North); // Ensure tabs are at the top
    m_tabWidget->setUsesScrollButtons(false); // Disable scroll buttons

    m_openGLImage = new UIOpenGLImage(this);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedHeight(5); 
    m_progressBar->setTextVisible(false); 
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->hide();

    QStringList renderModeOptions = {"Full", "Fast"};
    m_render_mode = new UiDropdownMenu("Render Mode:", renderModeOptions, this);
    m_render_mode->setCurrentIndex(0);
    connect(m_render_mode, &UiDropdownMenu::index_changed, this, &RenderWindow::render_mode_changed);

    QStringList lightsourceOptions = {"default", "CIE D65", "CIE D50", "Studio LED", "D65 Color Booth", "D50 Color Booth"};
    m_lightsource = new UiDropdownMenu("Lightsource:", lightsourceOptions, this);
    m_lightsource->setCurrentIndex(0);
    connect(m_lightsource, &UiDropdownMenu::index_changed, this, &RenderWindow::lightsource_changed);

    QStringList observerOptions = {"CIE 1931 2 Deg", "CIE 1964 10 Deg", "CIE 2006 2 Deg", "CIE 2006 10 Deg"};
    m_observer = new UiDropdownMenu("Observer:", observerOptions, this);
    m_observer->setCurrentIndex(0);
    connect(m_observer, &UiDropdownMenu::index_changed, this, &RenderWindow::update_observer);

    QStringList primariesOptions = {"Display P3", "sRGB"};
    m_primaries = new UiDropdownMenu("Primaries:", primariesOptions, this);
    m_primaries->setCurrentIndex(0);
    connect(m_primaries, &UiDropdownMenu::index_changed, this, &RenderWindow::updatePrimaries);

    // Create UiFloat for exposure and gamma
    m_exposureInput = new UiFloat("Exposure:", this, 0.1, 1000, 0.1);
    m_exposureInput->setValue(m_exposure);
    connect(m_exposureInput, &UiFloat::value_changed, this, &RenderWindow::updateExposure);

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

    // Create a QHBoxLayout for the spectral graph and its label
    QHBoxLayout *spectralLayout = new QHBoxLayout();
    spectralLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *spectralLabel = new QLabel("Spectral Graph:", this);
    spectralLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
    m_spectralGraph = new UISpectralGraph(this);
    spectralLayout->addWidget(spectralLabel, 3);
    spectralLayout->addWidget(m_spectralGraph, 10);

    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    
    QWidget *imageWidget = new QWidget(this);
    QVBoxLayout *imageLayout = new QVBoxLayout(imageWidget);
    imageLayout->setContentsMargins(0, 0, 0, 0);
    imageLayout->addWidget(m_openGLImage, 1); 
    imageLayout->addWidget(m_progressBar);


    QWidget *controlWidget = new QWidget(this);
    QVBoxLayout *controlLayout = new QVBoxLayout(controlWidget);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->addWidget(m_render_mode);
    controlLayout->addWidget(m_lightsource);
    controlLayout->addWidget(m_observer);
    controlLayout->addWidget(m_primaries);
    controlLayout->addWidget(m_exposureInput);
    controlLayout->addWidget(m_gammaInput);
    controlLayout->addWidget(m_spectrumSamplingMenu); 
    controlLayout->addWidget(m_samplesInput);
    controlLayout->addWidget(m_depthInput);
    controlLayout->addWidget(m_resolutionInput);
    controlLayout->addLayout(spectralLayout);  // Add the new spectral layout
    controlLayout->addStretch(1);

    // Add render button
    m_renderButton = new QPushButton("Render", this);
    connect(m_renderButton, &QPushButton::clicked, this, &RenderWindow::render_requested);
    controlLayout->addWidget(m_renderButton);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_tabWidget);  // Add the tab widget instead of m_usdTreeView
    m_splitter->addWidget(imageWidget);
    m_splitter->addWidget(controlWidget);
    m_splitter->setStretchFactor(0, 0); 
    m_splitter->setStretchFactor(1, 50);
    m_splitter->setStretchFactor(2, 0);

    mainLayout->addWidget(m_splitter);
    
    setCentralWidget(centralWidget);
    
    m_openGLImage->setMouseTracking(true);
    centralWidget->setMouseTracking(true);
    setMouseTracking(true);

    connect(m_openGLImage, &UIOpenGLImage::image_position_changed, this, &RenderWindow::update_spectral_graph);
}

void RenderWindow::updateExposure(float value)
{
    m_exposure = value;
    need_to_update_image = true;
    update_image();
}

void RenderWindow::updateGamma(float value)
{
    m_gamma = value;
    need_to_update_image = true;
    update_image();
}
void RenderWindow::updateSamples(int samples) {
    m_samplesInput->blockSignals(true);
    m_samplesInput->setValue(samples);
    m_samples = samples;
    m_samplesInput->blockSignals(false);
}
void RenderWindow::update_observer(int index)
{   
    switch (index) {
        case 0:
            observer_ptr = new observer(observer::CIE1931_2Deg, spectrum::RESPONSE_SAMPLES, spectrum::START_WAVELENGTH, spectrum::END_WAVELENGTH);
            break;
        case 1:
            observer_ptr = new observer(observer::CIE1964_10Deg, spectrum::RESPONSE_SAMPLES, spectrum::START_WAVELENGTH, spectrum::END_WAVELENGTH);
            break;
        case 2:
            observer_ptr = new observer(observer::CIE2006_2Deg, spectrum::RESPONSE_SAMPLES, spectrum::START_WAVELENGTH, spectrum::END_WAVELENGTH);
            break;
        case 3:
            observer_ptr = new observer(observer::CIE2006_10Deg, spectrum::RESPONSE_SAMPLES, spectrum::START_WAVELENGTH, spectrum::END_WAVELENGTH);
            break;
    }
    need_to_update_image = true;
    update_image();
}

void RenderWindow::update_spectral_graph(int x, int y) {
    spectrum color_spectrum = m_image_buffer->get_pixel(x, y) / ( m_exposure * m_exposure);
    m_spectralGraph->update_graph(color_spectrum);
}
void RenderWindow::updatePrimaries(int index) {
    need_to_update_image = true;
    update_image();
}
void RenderWindow::update_image() {
    if(!need_to_update_image) return;
    need_to_update_image = false;
    static const interval intensity(0.000, 0.999);

    QImage updatedImage(m_width, m_height, QImage::Format_RGB888);
    if(m_primaries->getCurrentIndex() == 0) {
        for (int i = 0; i < m_image_buffer->width(); i++) {
            for (int j = 0; j < m_image_buffer->height(); j++) {
                spectrum color_spectrum = m_image_buffer->get_pixel(i, j)  * std::pow(2.0, m_exposure);

                color color_rgb_displayP3 = color_spectrum.to_XYZ(observer_ptr).to_rgbDisplayP3();
                float r = int(255.999 * intensity.clamp(linear_to_gamma2(color_rgb_displayP3.x(), m_gamma))); 
                float g = int(255.999 * intensity.clamp(linear_to_gamma2(color_rgb_displayP3.y(), m_gamma)));
                float b = int(255.999 * intensity.clamp(linear_to_gamma2(color_rgb_displayP3.z(), m_gamma)));
                updatedImage.setPixelColor(i, j, QColor::fromRgb(r, g, b));
            }
        }
    } else {
        for (int i = 0; i < m_image_buffer->width(); i++) {
            for (int j = 0; j < m_image_buffer->height(); j++) {
                spectrum color_spectrum = m_image_buffer->get_pixel(i, j)  * std::pow(2.0, m_exposure);

                color color_rgb = color_spectrum.to_rgb(observer_ptr);
                float r = int(255.999 * intensity.clamp(linear_to_gamma2(color_rgb.x(), m_gamma))); 
                float g = int(255.999 * intensity.clamp(linear_to_gamma2(color_rgb.y(), m_gamma)));
                float b = int(255.999 * intensity.clamp(linear_to_gamma2(color_rgb.z(), m_gamma)));
                updatedImage.setPixelColor(i, j, QColor::fromRgb(r, g, b));
            }
        }
    }
    
    m_openGLImage->setImage(updatedImage);
}

void RenderWindow::updateProgress(int progress, int total)
{
    int percentage = static_cast<int>((static_cast<double>(progress) / total) * 100);
    if (percentage <= 0 || percentage >= 99) {
        m_progressBar->hide();
    } else {
        m_progressBar->show();
    }
    m_progressBar->setValue(percentage);
}

void RenderWindow::updateBucket(int x, int y, ImageSPD* image)
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
    m_image_buffer = new ImageSPD(m_width, m_height, spectrum::RESPONSE_SAMPLES, observer_ptr);
    m_image = new QImage(m_width, m_height, QImage::Format_RGB888);
    m_image->fill(Qt::black);
    need_to_update_image = true;
    update_image();
}
QString RenderWindow::style_sheet()
{
    return "* { background-color: #323232; font-size: 12px; color: #bababa;} \
    QLabel { color: #bababa; } \
    QPushButton { background-color: #515151; color: #bababa; border-radius: 5px; border: 1px solid #1e1e1e; padding: 4px; } \
    QPushButton:hover { background-color: #616161; } \
    QPushButton:pressed { background-color: #414141; } \
    QDoubleSpinBox, QSpinBox { background-color: #252525; color: #bababa; border-radius: 5px; border: 1px solid #1e1e1e; padding: 4px; } \
    QDoubleSpinBox::up-button, QSpinBox::up-button { background-color: #3a3a3a; color: #bababa; border-top-right-radius: 5px; margin-right: 1px; } \
    QDoubleSpinBox::down-button, QSpinBox::down-button { background-color: #3a3a3a; color: #bababa; border-bottom-right-radius: 5px; margin-right: 1px; } \
    QComboBox { background-color: #3a3a3a; color: #bababa; border-radius: 5px; border: 1px solid #1e1e1e; padding: 4px; } \
    QComboBox::drop-down { background-color: #3a3a3a; color: #bababa;  border: 1px solid #3a3a3a; border-radius: 5px; margin-right: 1px;  } \
    QProgressBar { background-color: #252525; color: #bababa; border-radius: 5px; border: 1px solid #1e1e1e; padding: 4px; text-align: center; padding:1px; } \
    QProgressBar::chunk { background-color: #515151; } \
    QListWidget, QTreeView { background-color: #252525; color: #bababa; border-radius: 5px; border: 1px solid #1e1e1e; padding: 4px; } \
    QTabWidget::pane { background-color: #252525; color: #bababa; border-radius: 5px; border: none; padding: 0px; margin: 0px; } \
    QTabWidget::tab-bar { alignment: left; left: 5px; bottom: -1px;} \
    QTabBar::tab { background-color: #3a3a3a; color: #bababa; border-radius: 5px; border: 1px solid #1e1e1e; padding: 1px; padding-left: 15px; padding-right: 15px; border-bottom: 1px solid #252525; border-bottom-left-radius: 0px; border-bottom-right-radius: 0px; } \
    QTabBar::tab:selected { background-color: #252525; } \
    QTabWidget { background-color: #252525; color: #bababa; border-radius: 5px; border: 1px solid #1e1e1e; padding: 0px; } \
    ";
}
void RenderWindow::onSaveClicked() {
    // Handle the save button click here
    auto file_name = m_settings_ptr->get_file_name(m_image_buffer->width(),m_image_buffer->height(), m_samples, 0, false);
    file_name += ".spd";
    m_image_buffer->exposure_ = m_exposure;
    m_image_buffer->gamma_ = m_gamma;
    m_image_buffer->observer_type_ = m_observer->getCurrentIndex();
    m_image_buffer->light_source_ = m_lightsource->getCurrentIndex();
    m_image_buffer->render_mode_ = m_render_mode->getCurrentIndex();
    m_image_buffer->spectrum_type_ = m_spectrumSamplingMenu->getCurrentIndex();
    m_image_buffer->samples_ = m_samplesInput->getValue();
    m_image_buffer->depth_ = m_depthInput->getValue();
    m_image_buffer->save_spectrum(file_name.c_str());
    // You can add your logic to save the current SPD file here
}
void RenderWindow::onSPDFileSelected(const QString &filePath) {
    m_image_buffer->load_spectrum(filePath.toStdString().c_str());
    m_exposure = m_image_buffer->exposure_;
    m_gamma = m_image_buffer->gamma_;
    m_gammaInput->setValue(m_gamma);
    m_exposureInput->setValue(m_exposure);
    m_observer->setCurrentIndex(m_image_buffer->observer_type_);
    m_lightsource->setCurrentIndex(m_image_buffer->light_source_);
    m_render_mode->setCurrentIndex(m_image_buffer->render_mode_);
    m_spectrumSamplingMenu->setCurrentIndex(m_image_buffer->spectrum_type_);
    m_samplesInput->setValue(m_image_buffer->samples_);
    m_depthInput->setValue(m_image_buffer->depth_);

    need_to_update_image = true;
    update_image();
}