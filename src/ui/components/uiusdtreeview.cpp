#include "uiusdtreeview.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFileInfo>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/tokens.h>
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
#include <pxr/usd/sdf/layerTree.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/sdf/reference.h>
#include <QFileIconProvider>

UiUSDTreeView::UiUSDTreeView(QWidget *parent)
    : QWidget(parent)
{
    m_splitter = new QSplitter(Qt::Vertical, this);
    m_topTreeView = new QTreeView(m_splitter);
    m_bottomTreeView = new QTreeView(m_splitter);

    setupTopTreeView();
    setupBottomTreeView();

    m_splitter->addWidget(m_topTreeView);
    m_splitter->addWidget(m_bottomTreeView);

    // Set the splitter sizes to 3/4 for top and 1/4 for bottom
    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(1, 1);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_splitter);
    setLayout(layout);
}

void UiUSDTreeView::setupTopTreeView()
{
    m_topModel = new QStandardItemModel(this);
    m_topTreeView->setModel(m_topModel);
    m_topTreeView->setHeaderHidden(true);
    m_topTreeView->setAnimated(false);
    m_topTreeView->setExpandsOnDoubleClick(true);
}

void UiUSDTreeView::setupBottomTreeView()
{
    m_bottomModel = new QStandardItemModel(this);
    m_bottomTreeView->setModel(m_bottomModel);
    m_bottomTreeView->setHeaderHidden(true);
    m_bottomTreeView->setAnimated(false);
    m_bottomTreeView->setExpandsOnDoubleClick(false);
    m_bottomTreeView->setRootIsDecorated(false); // Hide the root node
}

void UiUSDTreeView::loadUSDStage(const pxr::UsdStageRefPtr stage)
{
    m_topModel->clear();
    pxr::UsdPrim rootPrim = stage->GetPseudoRoot();
    populateTopTree(rootPrim, m_topModel->invisibleRootItem());

    m_topTreeView->expandToDepth(1);

    populateBottomTree(stage);
}

void UiUSDTreeView::populateTopTree(const pxr::UsdPrim &prim, QStandardItem *parentItem)
{
    if (!prim.IsValid()) {
        return;
    }

    QString primName = QString::fromStdString(prim.GetName());
    QString primType = QString::fromStdString(prim.GetTypeName().GetString());

    QStandardItem *item = new QStandardItem(primName);
    item->setToolTip(primType);

    // Store the full path of the prim as user data
    item->setData(QString::fromStdString(prim.GetPath().GetString()), Qt::UserRole);

    parentItem->appendRow(item);

    for (const auto &child : prim.GetChildren()) {
        populateTopTree(child, item);
    }
}

void UiUSDTreeView::populateBottomTree(const pxr::UsdStageRefPtr &stage)
{
    m_bottomModel->clear();
    QStandardItem *rootItem = m_bottomModel->invisibleRootItem();

    pxr::SdfLayerRefPtr rootLayer = stage->GetRootLayer();
    addLayerToTree(rootLayer, rootItem);

    m_bottomTreeView->expandAll();
}

void UiUSDTreeView::addLayerToTree(const pxr::SdfLayerRefPtr &layer, QStandardItem *parentItem)
{
    if (!layer) {
        return;
    }

    QString layerPath = QString::fromStdString(layer->GetIdentifier());
    QFileInfo fileInfo(layerPath);
    QStandardItem *layerItem = new QStandardItem(fileInfo.fileName());
    layerItem->setToolTip(layerPath);

    // Add file icon
    QFileIconProvider iconProvider;
    QIcon icon = iconProvider.icon(fileInfo);
    layerItem->setIcon(icon);

    parentItem->appendRow(layerItem);

    // Add sublayers
    for (const auto &sublayer : layer->GetSubLayerPaths()) {
        pxr::SdfLayerRefPtr sublayerRef = pxr::SdfLayer::FindOrOpen(sublayer);
        if (sublayerRef) {
            addLayerToTree(sublayerRef, layerItem);
        }
    }

    // Add references
    pxr::SdfPrimSpecHandle pseudoRoot = layer->GetPseudoRoot();
    for (const auto &child : pseudoRoot->GetNameChildren()) {
        if (pxr::SdfPrimSpecHandle primSpec = pxr::SdfPrimSpecHandle(child)) {
            for (const auto &ref : primSpec->GetReferenceList().GetAddedOrExplicitItems()) {
                if (!ref.GetAssetPath().empty()) {
                    pxr::SdfLayerRefPtr refLayer = pxr::SdfLayer::FindOrOpen(ref.GetAssetPath());
                    if (refLayer) {
                        addLayerToTree(refLayer, layerItem);
                    }
                }
            }
        }
    }
}