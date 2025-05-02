// parametersdialog.h
#ifndef PARAMETERSDIALOG_H
#define PARAMETERSDIALOG_H

#include <QDialog>
#include <QSerialPort>
#include <QTableWidget>
#include <QPushButton>
#include <QStringList>
#include <QTimer.h>

class ParametersDialog : public QDialog {
    Q_OBJECT
public:
    explicit ParametersDialog(QSerialPort* port, QWidget* parent = nullptr);

public slots:
    void onSerialLineReceived(const QString &line);
    void onRefreshClicked();

private slots:
    void onLoadClicked();
    void onSaveClicked();
    void onOkClicked();
    void onCellChanged(int row, int column);


private:
    void sendCommand(const QString &cmd);

    QSerialPort*   serialPort;
    QTableWidget*  table;
    QPushButton*   btnRefresh;
    QPushButton*   btnLoad;
    QPushButton*   btnSave;
    QPushButton*   btnOk;

    QStringList    commands;
    QVector<int>   highLimits;
    bool           initializing;

    QVector<QString> originalValues;
};

#endif // PARAMETERSDIALOG_H
