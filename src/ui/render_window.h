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
#include <QComboBox>  // Add this include
#include <QSpinBox>  // Add this include
#include "../image/image_png.h"
#include "components/uiint2.h"
#include "components/uiint.h"
#include "components/uifloat.h"
#include "../helpers/settings.h"
#include "components/uidropdownmenu.h"
#include "components/uiopenglimage.h"
#include "components/uispectralgraph.h"
#include "components/uiusdtreeview.h"
#include "../usd/loader.h"
#include <QTabWidget>
#include "components/usd_tree_component.h"
#include "components/spd_file_list_component.h"

class RenderWindow : public QMainWindow
{
    Q_OBJECT
public:
    RenderWindow(settings * settings_ptr, rd::usd::loader * loader, QWidget *parent = nullptr);

public slots:
    void updateProgress(int progress, int total);
    void updateBucket(int x, int y, ImagePNG* image);
    void updateGain(float value);
    void updateGamma(float value);
    void updateSamples(int samples);

private slots:
    void update_spectral_graph(int x, int y);
    void update_resolution(int width, int height);
    void update_observer(int index);
    void onSPDFileSelected(const QString &filePath);
    void onSaveClicked();
signals:
    void render_requested();
    void spectrum_sampling_changed(int index); 
    void lightsource_changed(int index);
    void observer_changed(int index);
    void samples_changed(int samples);
    void depth_changed(int depth);
    void resolution_changed(int width, int height);
    void render_mode_changed(int index);
protected:

    void resizeEvent(QResizeEvent *event) override;

private:
    void updateImageLabelSize();
    void setupUI();
    settings * m_settings_ptr;
    int m_width;
    int m_height;
    int m_samples;
    int m_depth;
    QImage *m_image;
    rd::usd::loader * m_loader;
    QScrollArea *m_scrollArea;
    QMap<QString, QString> m_metadata;
    QProgressBar *m_progressBar;
    observer * observer_ptr;
    ImagePNG * m_image_buffer;
    UiFloat *m_gainInput;
    UiFloat *m_gammaInput;
    float m_gain;
    float m_gamma;
    float m_whitebalance;
    QSplitter *m_splitter;
    QPushButton *m_renderButton;  // New render button
    UiDropdownMenu *m_render_mode;
    UiDropdownMenu *m_lightsource;
    UiDropdownMenu *m_observer;
    UiDropdownMenu *m_spectrumSamplingMenu;  // Replace QComboBox with UiDropdownMenu
    UiInt *m_samplesInput;  // New samples input
    UiInt *m_depthInput;  // New depth input
    UiInt2 *m_resolutionInput;  // New resolution input
    void update_image();
    bool need_to_update_image;
    UIOpenGLImage *m_openGLImage;
    QString style_sheet();
    UISpectralGraph *m_spectralGraph;
    QTabWidget *m_tabWidget;
    USDTreeComponent *m_usdTreeComponent;
    SPDFileListComponent *m_spdFileListComponent;
};

#endif // RENDER_WINDOW_H
