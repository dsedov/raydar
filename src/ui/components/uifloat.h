#ifndef UIFLOAT_H
#define UIFLOAT_H

#include <QWidget>
#include <QLabel>
#include <QDoubleSpinBox>

class UiFloat : public QWidget
{
    Q_OBJECT

public:
    explicit UiFloat(const QString& label, QWidget *parent = nullptr, float min = 1, float max = 100, float step = 0.1);

    void setValue(float value);
    float getValue() const;

signals:
    void value_changed(float value);

public slots:
    void update_value(float value);

private:
    QLabel *m_label;
    QDoubleSpinBox *m_spinBox;

    void setupUI();
};

#endif // UIFLOAT_H