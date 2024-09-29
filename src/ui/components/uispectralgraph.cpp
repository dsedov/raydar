#include "uispectralgraph.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>

UISpectralGraph::UISpectralGraph(QWidget *parent)
    : QWidget(parent), m_minValue(0), m_maxValue(1)
{
    m_spectralData.resize(31);
    setMinimumSize(300, 150);
    observer_ptr = new observer(observer::CIE1931_2Deg, spectrum::RESPONSE_SAMPLES, spectrum::START_WAVELENGTH, spectrum::END_WAVELENGTH);
    
    // Set the background color and make the widget transparent
    setAutoFillBackground(true);
    setAttribute(Qt::WA_TranslucentBackground);
}

QSize UISpectralGraph::sizeHint() const
{
    return QSize(400, 150);
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

    // Draw the background with rounded corners and border
    painter.setPen(QPen(QColor("#1e1e1e"), 1));  // 1px border with color #1e1e1e
    painter.setBrush(QColor("#252525"));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 5, 5);  // Adjust rect to account for the border

    // Set up the drawing area
    int margin = 40;
    int graphWidth = width() - 1 * margin - 15;
    int graphHeight = height() - 2 * margin;

    // Draw axes
    painter.setPen(QPen(QColor("#515151"), 1));
    // horizontal
    painter.drawLine(margin, height() - 20, width() - 10, height() - 20);

    // Vertical
    painter.drawLine(margin, margin, margin, height() - 20);

    // Draw spectral curve
    if (!m_spectralData.isEmpty()) {
        QPainterPath path;
        for (int i = 0; i < m_spectralData.size(); ++i) {
            float x = margin + (i / 30.0) * graphWidth;
            float y = height() - 20 - ((m_spectralData[i] - m_minValue) / (m_maxValue - m_minValue)) * graphHeight;
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
        painter.drawText(QPointF(x - 15, height() - 20 + 15), QString::number(wavelength) + "nm");
    }

    // Draw min and max values
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(QString::number(m_maxValue, 'f', 2));
    painter.drawText(QRectF(5, margin - 10, textWidth, 20), Qt::AlignRight, QString::number(m_maxValue, 'f', 2));
    painter.drawText(QRectF(5, height() - 30, textWidth, 20), Qt::AlignRight, QString::number(m_minValue, 'f', 2));
}