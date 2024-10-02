#ifndef USD_TREE_COMPONENT_H
#define USD_TREE_COMPONENT_H

#include <QWidget>
#include <QVBoxLayout>
#include "uiusdtreeview.h"

class USDTreeComponent : public QWidget
{
    Q_OBJECT

public:
    explicit USDTreeComponent(pxr::UsdStageRefPtr stage, QWidget *parent = nullptr);

private:
    UiUSDTreeView *m_usdTreeView;
    QVBoxLayout *m_layout;
};

#endif // USD_TREE_COMPONENT_H