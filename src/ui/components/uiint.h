#ifndef UIINT_H
#define UIINT_H

#include <QWidget>
#include <QLabel>
#include <QSpinBox>

class UiInt : public QWidget
{
    Q_OBJECT

public:
    explicit UiInt(const QString& label, QWidget *parent = nullptr, int min = 1, int max = 100, int step = 1);

    void setValue(int value);
    int getValue() const;

signals:
    void value_changed(int value);

public slots:
    void update_value(int value);

private:
    QLabel *m_label;
    QSpinBox *m_spinBox;

    void setupUI();
};

#endif // INT_H
