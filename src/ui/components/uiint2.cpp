#include "uiint2.h"
#include <QHBoxLayout>

UiInt2::UiInt2(const QString& label, QWidget *parent, int min, int max, int step)
    : QWidget(parent)
{
    m_label = new QLabel(label, this);
    m_spinBox1 = new QSpinBox(this);
    m_spinBox2 = new QSpinBox(this);
    m_spinBox1->setRange(min, max);
    m_spinBox2->setRange(min, max);
    m_spinBox1->setSingleStep(step);
    m_spinBox2->setSingleStep(step);

    setupUI();

    connect(m_spinBox1, &QSpinBox::editingFinished, this, &UiInt2::emitValuesChanged);
    connect(m_spinBox2, &QSpinBox::editingFinished, this, &UiInt2::emitValuesChanged);

    
}

void UiInt2::emitValuesChanged()
{
    emit values_changed(m_spinBox1->value(), m_spinBox2->value());
}
void UiInt2::setupUI()
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(m_label);
    layout->addWidget(m_spinBox1);
    layout->addWidget(m_spinBox2);
    m_label->setMinimumWidth(0);
    m_label->setMaximumWidth(QWIDGETSIZE_MAX);
    layout->setStretchFactor(m_label, 3);
    layout->setStretchFactor(m_spinBox1, 5);
    layout->setStretchFactor(m_spinBox2, 5);
    setLayout(layout);
}

void UiInt2::setValues(int value1, int value2)
{
    m_spinBox1->setValue(value1);
    m_spinBox2->setValue(value2);
}

std::pair<int, int> UiInt2::getValues() const
{
    return {m_spinBox1->value(), m_spinBox2->value()};
}

void UiInt2::update_values(int value1, int value2)
{
    setValues(value1, value2);
}
