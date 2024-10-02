#include "spd_file_list_component.h"
#include <QVBoxLayout>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <algorithm>

SPDFileListComponent::SPDFileListComponent(QWidget *parent) : QWidget(parent) {
    setupUI();
    m_currentDir = QDir::currentPath();
    m_fileWatcher = new QFileSystemWatcher(this);
    m_fileWatcher->addPath(m_currentDir);
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &SPDFileListComponent::updateFileList);
    updateFileList();
}

void SPDFileListComponent::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_fileListWidget = new QListWidget(this);
    layout->addWidget(m_fileListWidget);

    // Add Save button
    m_saveButton = new QPushButton("Save", this);
    layout->addWidget(m_saveButton);

    setLayout(layout);

    connect(m_fileListWidget, &QListWidget::itemClicked, this, &SPDFileListComponent::onFileClicked);
    // Connect Save button
    connect(m_saveButton, &QPushButton::clicked, this, &SPDFileListComponent::onSaveClicked);
}

void SPDFileListComponent::updateFileList() {
    m_fileListWidget->clear();
    populateFileList();
}
void SPDFileListComponent::refresh() {
    updateFileList();
}
void SPDFileListComponent::populateFileList() {
    QDir dir(m_currentDir);
    QStringList filters;
    filters << "*.spd";
    QFileInfoList fileInfoList = dir.entryInfoList(filters, QDir::Files, QDir::Time);

    for (const QFileInfo &fileInfo : fileInfoList) {
        QString displayName = fileInfo.fileName();
        QListWidgetItem *item = new QListWidgetItem(displayName);
        item->setData(Qt::UserRole, fileInfo.filePath());
        m_fileListWidget->addItem(item);
    }
}

void SPDFileListComponent::onFileClicked(QListWidgetItem *item) {
    if (item) {
        QString filePath = item->data(Qt::UserRole).toString();
        emit fileSelected(filePath);
    }
}

// Add new method for Save button click
void SPDFileListComponent::onSaveClicked() {
    emit saveRequested();
}