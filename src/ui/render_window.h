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
class RenderWindow : public QMainWindow
{
    Q_OBJECT
public:
    RenderWindow(settings * settings_ptr, QWidget *parent = nullptr);
    void updatePixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, const QString& metadata);

public slots:
    void updateProgress(int progress, int total);
    void updateBucket(int x, int y, ImagePNG* image);
    void updateGain(float value);
    void updateGamma(float value);
    void spectrumSamplingChanged(int index);  // New slot
    void update_resolution(int value1, int value2);
    void updateSamples(int value);
    void updateDepth(int value);

signals:
    void render_requested();
    void spectrum_sampling_changed(int index);  // New signal

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateImageLabelSize();
    void setupUI();
    settings * m_settings_ptr;
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
    UiFloat *m_gainInput;
    UiFloat *m_gammaInput;
    float m_gain;
    float m_gamma;
    QSplitter *m_splitter;
    QPushButton *m_renderButton;  // New render button
    QComboBox *m_spectrumComboBox;  // New dropdown menu
    UiInt *m_samplesInput;  // New samples input
    UiInt *m_depthInput;  // New depth input
    UiInt2 *m_resolutionInput;  // New resolution input
    void update_image();
};

#endif // RENDER_WINDOW_H
