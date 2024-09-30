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
}

void UiUSDTreeView::loadUSDStage(const pxr::UsdStageRefPtr stage)
{
    m_topModel->clear();
    pxr::UsdPrim rootPrim = stage->GetPseudoRoot();
    populateTopTree(rootPrim, m_topModel->invisibleRootItem());

    m_topTreeView->expandToDepth(1);

    populateBottomTree();
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

void UiUSDTreeView::populateBottomTree()
{
    m_bottomModel->clear();
    QSet<QString> usdFiles;

    collectUSDFiles(m_topModel->invisibleRootItem(), usdFiles);

    QStandardItem *rootItem = new QStandardItem("USD Files");
    m_bottomModel->appendRow(rootItem);

    for (const QString &file : usdFiles) {
        QFileInfo fileInfo(file);
        QStandardItem *fileItem = new QStandardItem(fileInfo.fileName());
        fileItem->setToolTip(file);
        rootItem->appendRow(fileItem);
    }

    m_bottomTreeView->expandAll();
}

void UiUSDTreeView::collectUSDFiles(const QStandardItem *item, QSet<QString> &usdFiles)
{
    if (!item) {
        return;
    }

    QString path = item->data(Qt::UserRole).toString();
    if (!path.isEmpty() && path.endsWith(".usd", Qt::CaseInsensitive)) {
        usdFiles.insert(path);
    }

    for (int i = 0; i < item->rowCount(); ++i) {
        collectUSDFiles(item->child(i), usdFiles);
    }
}