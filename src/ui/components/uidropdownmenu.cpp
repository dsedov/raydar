#include "uidropdownmenu.h"
#include <QHBoxLayout>

UiDropdownMenu::UiDropdownMenu(const QString& label, const QStringList& options, QWidget *parent)
    : QWidget(parent)
{
    m_label = new QLabel(label, this);
    m_comboBox = new QComboBox(this);
    m_comboBox->addItems(options);

    setupUI();

    connect(m_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &UiDropdownMenu::index_changed);
}

void UiDropdownMenu::setupUI()
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(m_label);
    layout->addWidget(m_comboBox);
    m_label->setMinimumWidth(0);
    m_label->setMaximumWidth(QWIDGETSIZE_MAX);
    layout->setStretchFactor(m_label, 3);
    layout->setStretchFactor(m_comboBox, 10);
    setLayout(layout);
}

void UiDropdownMenu::setCurrentIndex(int index)
{
    m_comboBox->setCurrentIndex(index);
}

int UiDropdownMenu::getCurrentIndex() const
{
    return m_comboBox->currentIndex();
}

void UiDropdownMenu::addItem(const QString& item)
{
    m_comboBox->addItem(item);
}

void UiDropdownMenu::update_index(int index)
{
    setCurrentIndex(index);
}