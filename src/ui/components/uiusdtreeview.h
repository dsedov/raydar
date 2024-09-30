#ifndef UIUSDTREEVIEW_H
#define UIUSDTREEVIEW_H

#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>
#include <QSplitter>
#include <pxr/usd/usd/stage.h>

class UiUSDTreeView : public QWidget
{
    Q_OBJECT

public:
    explicit UiUSDTreeView(QWidget *parent = nullptr);
    void loadUSDStage(const pxr::UsdStageRefPtr stage);

private:
    QSplitter *m_splitter;
    QTreeView *m_topTreeView;
    QTreeView *m_bottomTreeView;
    QStandardItemModel *m_topModel;
    QStandardItemModel *m_bottomModel;

    void setupTopTreeView();
    void setupBottomTreeView();
    void populateTopTree(const pxr::UsdPrim &prim, QStandardItem *parentItem);
    void populateBottomTree();
    void collectUSDFiles(const QStandardItem *item, QSet<QString> &usdFiles);
};

#endif // UIUSDTREEVIEW_H