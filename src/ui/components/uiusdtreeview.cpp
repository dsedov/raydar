#include "uiusdtreeview.h"
#include <QHeaderView>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdLux/rectLight.h>
#include <pxr/usd/usdGeom/metrics.h>

UiUSDTreeView::UiUSDTreeView(QWidget *parent)
    : QTreeView(parent)
{
    m_model = new QStandardItemModel(this);
    setModel(m_model);
    setHeaderHidden(true);
    setAnimated(false);
    setExpandsOnDoubleClick(true);
}

void UiUSDTreeView::loadUSDStage(const pxr::UsdStageRefPtr stage)
{
    m_model->clear();
    pxr::UsdPrim rootPrim = stage->GetPseudoRoot();
    populateTree(rootPrim, m_model->invisibleRootItem());

    expandToDepth(1); 
}

void UiUSDTreeView::populateTree(const pxr::UsdPrim &prim, QStandardItem *parentItem)
{
    if (!prim.IsValid()) {
        return;
    }

    QString primName = QString::fromStdString(prim.GetName());
    QString primType = QString::fromStdString(prim.GetTypeName().GetString());

    QStandardItem *item = new QStandardItem(primName);
    item->setToolTip(primType);

    parentItem->appendRow(item);

    for (const auto &child : prim.GetChildren()) {
        populateTree(child, item);
    }
}