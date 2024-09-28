#include "uispectralgraph.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>

UISpectralGraph::UISpectralGraph(QWidget *parent)
    : QWidget(parent), m_minValue(0), m_maxValue(1)
{
    m_spectralData.resize(31);
    setMinimumSize(300, 200);
    observer_ptr = new observer(observer::CIE1931_2Deg, spectrum::RESPONSE_SAMPLES, spectrum::START_WAVELENGTH, spectrum::END_WAVELENGTH);
}

QSize UISpectralGraph::sizeHint() const
{
    return QSize(400, 250);
}

void UISpectralGraph::update_graph(const spectrum& color_spectrum)
{
    for (int i = 0; i < 31; ++i) {
        m_spectralData[i] = color_spectrum[i];
    }
    color_rgb = color_spectrum.to_rgb(observer_ptr);
    updateMinMax();
    update();
}

void UISpectralGraph::setSpectralData(const QVector<float>& data)
{
    if (data.size() != 31) {
        qWarning() << "Spectral data must contain exactly 31 points";
        return;
    }
    m_spectralData = data;
    
    updateMinMax();
    update();
}

void UISpectralGraph::updateMinMax()
{
    m_minValue = m_spectralData[0];
    m_maxValue = m_spectralData[0];
    for (float value : m_spectralData) {
        m_minValue = qMin(m_minValue, value);
        m_maxValue = qMax(m_maxValue, value);
    }
}

void UISpectralGraph::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Set up the drawing area
    int margin = 80;
    int graphWidth = width() - 1 * margin;
    int graphHeight = height() - 2 * margin;

    // Draw axes
    painter.drawLine(margin, height() - margin, width(), height() - margin);
    painter.drawLine(margin, margin, margin, height() - margin);

    // Draw spectral curve
    if (!m_spectralData.isEmpty()) {
        QPainterPath path;
        for (int i = 0; i < m_spectralData.size(); ++i) {
            float x = margin + (i / 30.0) * graphWidth;
            float y = height() - margin - ((m_spectralData[i] - m_minValue) / (m_maxValue - m_minValue)) * graphHeight;
            if (i == 0) {
                path.moveTo(x, y);
            } else {
                path.lineTo(x, y);
            }
        }
        painter.setPen(QPen(Qt::white, 2));
        painter.drawPath(path);
    }

    // Draw wavelength labels
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);
    for (int i = 0; i <= 3; ++i) {
        int wavelength = 400 + i * 100;
        float x = margin + (i / 3.0) * graphWidth;
        painter.drawText(QPointF(x - 15, height() - margin + 15), QString::number(wavelength));
    }

    // Draw min and max values
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(QString::number(m_maxValue, 'f', 2));
    painter.drawText(QRectF(margin, margin - 10, textWidth, 20), Qt::AlignRight, QString::number(m_maxValue, 'f', 2));
    painter.drawText(QRectF(margin, height() - margin - 10, textWidth, 20), Qt::AlignRight, QString::number(m_minValue, 'f', 2));

    painter.drawText(QRectF(margin, margin + 10, textWidth, 20), Qt::AlignRight, QString::number(color_rgb.x(), 'f', 2));
    painter.drawText(QRectF(margin, margin + 30, textWidth, 20), Qt::AlignRight, QString::number(color_rgb.y(), 'f', 2));
    painter.drawText(QRectF(margin, margin + 50, textWidth, 20), Qt::AlignRight, QString::number(color_rgb.z(), 'f', 2));
}