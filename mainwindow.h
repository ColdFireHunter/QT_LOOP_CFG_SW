#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QActionGroup>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QScrollBar>
#include <QLabel>
#include <cmath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QMessageBox>  // at the top with the other Qt includes
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>>

#include <eepromdialog.h>
#include <parametersdialog.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void addLoop1Data(double frequency);
    void addLoop2Data(double frequency);
    void sendSerial(const QString &text);

signals:
    void serialLineReceived(const QString &line);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void on_actionREFRESH_triggered();
    void onPortSelected(QAction *action);
    void on_actionCONNECT_triggered();
    void on_actionDISCONNECT_triggered();
    void on_btnRESET1_clicked();
    void on_btnRESET2_clicked();
    void on_actionSAVE_LOOP_1_triggered();
    void on_actionSAVE_LOOP_2_triggered();
    void on_actionLOAD_LOOP1_triggered();
    void on_actionLOAD_LOOP2_triggered();
    void onSerialReadyRead();
    void on_actionLIVE_ON_triggered();
    void on_actionLIVE_OFF_triggered();
    void on_actionRESET_MCU_triggered();
    void on_actionLED_TEST_triggered();
    void on_actionFORMAT_EEPROM_triggered();
    void on_btnCLA1_clicked();
    void on_btnCAL2_clicked();
    void on_actionEEPROM_triggered();
    void on_actionOPEN_PARAMETERS_triggered();

private:
    void connectActions();
    void resetLoop1();
    void resetLoop2();

    Ui::MainWindow *ui;
    QSerialPort *serialPort;
    QByteArray serialBuffer;
    QMenu *portMenu;
    QActionGroup *portGroup;
    QAction *refreshAction;
    QAction *connectAction;
    QAction *disconnectAction;
    QLabel *connectionLabel;
    QLabel *liveDataLabel;  // shows parsed live data
    QTimer  *liveTimer;      // polls device every 100 ms
    QScrollBar *scrollBar1;
    QScrollBar *scrollBar2;

    QChartView *chartView1;
    QChart *chart1;
    QLineSeries *series1;
    QValueAxis *axisX1;
    QValueAxis *axisY1;
    int sampleCount1;

    QChartView *chartView2;
    QChart *chart2;
    QLineSeries *series2;
    QValueAxis *axisX2;
    QValueAxis *axisY2;
    int sampleCount2;

    bool isPanning;
    QPoint lastMousePos;
    const int windowSize = 100;
    bool autoScroll;

    EEPROMDialog* eepromDialog = nullptr;
    ParametersDialog *parametersDialog = nullptr;

    void autoscaleYVisible(QLineSeries* series, QValueAxis* axisX, QValueAxis* axisY);
};


#endif // MAINWINDOW_H

