#include "uifloat.h"
#include <QHBoxLayout>

UiFloat::UiFloat(const QString& label, QWidget *parent, float min, float max, float step)
    : QWidget(parent)
{
    m_label = new QLabel(label, this);
    m_spinBox = new QDoubleSpinBox(this);
    m_spinBox->setRange(min, max);
    m_spinBox->setSingleStep(step);

    setupUI();

    connect(m_spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        emit value_changed(static_cast<float>(value));
    });
}

void UiFloat::setupUI()
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(m_label);
    layout->addWidget(m_spinBox);
    m_label->setMinimumWidth(0);
    m_label->setMaximumWidth(QWIDGETSIZE_MAX);
    layout->setStretchFactor(m_label, 3);
    layout->setStretchFactor(m_spinBox, 10);
    setLayout(layout);
}

void UiFloat::setValue(float value)
{
    m_spinBox->setValue(value);
}

float UiFloat::getValue() const
{
    return static_cast<float>(m_spinBox->value());
}

void UiFloat::update_value(float value)
{
    setValue(value);
}