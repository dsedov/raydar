#ifndef UISPECTRALGRAPH_H
#define UISPECTRALGRAPH_H

#include <QWidget>
#include <QVector>
#include "../../data/spectrum.h"

class UISpectralGraph : public QWidget
{
    Q_OBJECT

public:
    explicit UISpectralGraph(QWidget *parent = nullptr);

    QSize sizeHint() const override;

public slots:
    void setSpectralData(const QVector<float>& data);
    void update_graph(const spectrum& color_spectrum);
protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<float> m_spectralData;
    float m_minValue;
    float m_maxValue;

    void updateMinMax();
};

#endif // UISPECTRALGRAPH_H