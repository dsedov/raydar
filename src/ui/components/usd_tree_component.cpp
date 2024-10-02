#include "usd_tree_component.h"

USDTreeComponent::USDTreeComponent(pxr::UsdStageRefPtr stage, QWidget *parent)
    : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_usdTreeView = new UiUSDTreeView(this);
    m_usdTreeView->loadUSDStage(stage);
    m_layout->addWidget(m_usdTreeView);
    setLayout(m_layout);
}