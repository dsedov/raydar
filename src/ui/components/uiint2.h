#ifndef UIINT2_H
#define UIINT2_H

#include <QWidget>
#include <QLabel>
#include <QSpinBox>

class UiInt2 : public QWidget
{
    Q_OBJECT

public:
    explicit UiInt2(const QString& label, QWidget *parent = nullptr, int min = 1, int max = 16384, int step = 1);

    void setValues(int value1, int value2);
    std::pair<int, int> getValues() const;

signals:
    void values_changed(int value1, int value2);
    

public slots:
    void update_values(int value1, int value2);
    void emitValuesChanged();

private:
    QLabel *m_label;
    QSpinBox *m_spinBox1;
    QSpinBox *m_spinBox2;

    void setupUI();
};

#endif // INT2_H
