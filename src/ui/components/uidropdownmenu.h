#ifndef UIDROPDOWNMENU_H
#define UIDROPDOWNMENU_H

#include <QWidget>
#include <QLabel>
#include <QComboBox>

class UiDropdownMenu : public QWidget
{
    Q_OBJECT

public:
    explicit UiDropdownMenu(const QString& label, const QStringList& options, QWidget *parent = nullptr);

    void setCurrentIndex(int index);
    int getCurrentIndex() const;
    void addItem(const QString& item);

signals:
    void index_changed(int index);

public slots:
    void update_index(int index);

private:
    QLabel *m_label;
    QComboBox *m_comboBox;

    void setupUI();
};

#endif // UIDROPDOWNMENU_H