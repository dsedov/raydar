#ifndef UIUSDTREEVIEW_H
#define UIUSDTREEVIEW_H

#include <QTreeView>
#include <QStandardItemModel>
#include <pxr/usd/usd/stage.h>

class UiUSDTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit UiUSDTreeView(QWidget *parent = nullptr);
    void loadUSDStage(const pxr::UsdStageRefPtr stage);

private:
    QStandardItemModel *m_model;
    void populateTree(const pxr::UsdPrim &prim, QStandardItem *parentItem);
};

#endif // UIUSDTREEVIEW_H