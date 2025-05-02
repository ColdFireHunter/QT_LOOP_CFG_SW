#ifndef EEPROMDIALOG_H
#define EEPROMDIALOG_H

#include <QDialog>
#include <QSerialPort>
#include <QTableWidget>
#include <QPushButton>

class EEPROMDialog : public QDialog {
    Q_OBJECT

public:
    EEPROMDialog(QSerialPort* port, QWidget* parent = nullptr);

public slots:
    void requestData();            // Send the “eeprom” command
    void appendLine(const QString &line);

private slots:
    void onRefreshClicked();

private:
    QSerialPort*   serialPort;
    QTableWidget*  table;
    QPushButton*   refreshBtn;
};

#endif // EEPROMDIALOG_H
