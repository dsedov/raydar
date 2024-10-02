#ifndef SPD_FILE_LIST_COMPONENT_H
#define SPD_FILE_LIST_COMPONENT_H

#include <QWidget>
#include <QListWidget>
#include <QFileSystemWatcher>
#include <QPushButton>

class SPDFileListComponent : public QWidget
{
    Q_OBJECT

public:
    explicit SPDFileListComponent(QWidget *parent = nullptr);

signals:
    void fileSelected(const QString &filePath);
    void saveRequested();

public slots:
    void refresh();
    
private slots:
    void updateFileList();
    void onFileClicked(QListWidgetItem *item);
    void onSaveClicked(); // Add new slot

private:
    QListWidget *m_fileListWidget;
    QFileSystemWatcher *m_fileWatcher;
    QString m_currentDir;
    QPushButton *m_saveButton; // Add new member variable

    void setupUI();
    void populateFileList();
};

#endif // SPD_FILE_LIST_COMPONENT_H