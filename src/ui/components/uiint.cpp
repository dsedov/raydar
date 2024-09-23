#include "uiint.h"
#include <QHBoxLayout>

UiInt::UiInt(const QString& label, QWidget *parent, int min, int max, int step)
    : QWidget(parent)
{
    m_label = new QLabel(label, this);
    m_spinBox = new QSpinBox(this);
    m_spinBox->setRange(min, max);
    m_spinBox->setSingleStep(step);

    setupUI();

    connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &UiInt::value_changed);
}

void UiInt::setupUI()
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

void UiInt::setValue(int value)
{
    m_spinBox->setValue(value);
}

int UiInt::getValue() const
{
    return m_spinBox->value();
}

void UiInt::update_value(int value)
{
    setValue(value);
}