// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
    serialPort(new QSerialPort(this)), portGroup(new QActionGroup(this)),
    connectionLabel(new QLabel(this)), chart1(new QChart()),
    series1(new QLineSeries(this)), axisX1(new QValueAxis()),
    axisY1(new QValueAxis()), sampleCount1(0), chart2(new QChart()),
    series2(new QLineSeries(this)), axisX2(new QValueAxis()),
    axisY2(new QValueAxis()), sampleCount2(0),isPanning(false) {
    ui->setupUi(this);

    // Initialize status labels
    connectionLabel->setText(tr("Disconnected"));
    liveDataLabel = new QLabel(this);
    liveDataLabel->setTextFormat(Qt::PlainText);
    liveDataLabel->setWordWrap(false);
    liveDataLabel->setText(tr("LIVE: OFF"));
    statusBar()->addWidget(liveDataLabel);

    // Create and configure live polling timer
    liveTimer = new QTimer(this);
    liveTimer->setInterval(100);
    liveTimer->stop();
    connect(liveTimer, &QTimer::timeout, this, [this](){
        sendSerial("live");
    });
    ui->actionLIVE_ON->setEnabled(false);
    ui->actionLIVE_OFF->setEnabled(false);

    autoScroll = false;

    // Serial menu
    refreshAction = ui->actionREFRESH;
    connectAction = ui->actionCONNECT;
    disconnectAction = ui->actionDISCONNECT;
    portMenu = new QMenu(tr("Serial Port"), this);
    ui->actionSERIAL_PORT->setMenu(portMenu);
    portGroup->setExclusive(true);
    connectAction->setEnabled(false);
    disconnectAction->setEnabled(false);
    connectActions();
    statusBar()->addPermanentWidget(connectionLabel);

    // Chart 1
    chartView1 = ui->chartViewLoop1;
    chart1->addSeries(series1);
    chart1->legend()->hide();
    axisX1->setTitleText("Sample Count");
    axisX1->setRange(0, windowSize);
    axisX1->setLabelFormat("%d");
    chart1->addAxis(axisX1, Qt::AlignBottom);
    series1->attachAxis(axisX1);
    axisY1->setTitleText("Frequency (Hz)");
    axisY1->setRange(-1, 1);
    chart1->addAxis(axisY1, Qt::AlignLeft);
    series1->attachAxis(axisY1);
    chartView1->setChart(chart1);
    chartView1->setRenderHint(QPainter::Antialiasing);
    series1->setPen(QPen(Qt::red, 2));
    chartView1->setRenderHint(QPainter::Antialiasing);
    chartView1->setRubberBand(QChartView::NoRubberBand);

    // Chart 2
    chartView2 = ui->chartViewLoop2;
    chart2->addSeries(series2);
    chart2->legend()->hide();
    axisX2->setTitleText("Sample Count");
    axisX2->setRange(0, windowSize);
    axisX2->setLabelFormat("%d");
    chart2->addAxis(axisX2, Qt::AlignBottom);
    series2->attachAxis(axisX2);
    axisY2->setTitleText("Frequency (Hz)");
    axisY2->setRange(-1, 1);
    chart2->addAxis(axisY2, Qt::AlignLeft);
    series2->attachAxis(axisY2);
    chartView2->setChart(chart2);
    chartView2->setRenderHint(QPainter::Antialiasing);
    series2->setPen(QPen(Qt::blue, 2));
    chartView2->setRenderHint(QPainter::Antialiasing);
    chartView2->setRubberBand(QChartView::NoRubberBand);

    chartView1->installEventFilter(this);
    chartView2->installEventFilter(this);

    scrollBar1 = new QScrollBar(Qt::Horizontal, this);
    scrollBar2 = new QScrollBar(Qt::Horizontal, this);
    ui->verticalLayoutCharts->insertWidget(1, scrollBar1);
    ui->verticalLayoutCharts->insertWidget(3, scrollBar2);
    connect(scrollBar1, &QScrollBar::valueChanged, this, [this](int v){
        axisX1->setRange(v, v + windowSize);
        autoscaleYVisible(series1, axisX1, axisY1);
    });
    connect(scrollBar2, &QScrollBar::valueChanged, this, [this](int v){
        axisX2->setRange(v, v + windowSize);
        autoscaleYVisible(series2, axisX2, axisY2);
    });

    ui->actionRESET_MCU->setEnabled(false);
    ui->actionLED_TEST->setEnabled(false);
    ui->actionFORMAT_EEPROM->setEnabled(false);

    ui->btnCLA1->setEnabled(false);
    ui->btnCAL2->setEnabled(false);

    ui->actionSAVE_LOOP_1->setEnabled(false);
    ui->actionSAVE_LOOP_2->setEnabled(false);

    ui->actionEEPROM->setEnabled(false);
    ui->actionOPEN_PARAMETERS->setEnabled(false);

    resetLoop1();
    resetLoop2();

    on_actionREFRESH_triggered();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::connectActions() {
    connect(portGroup, &QActionGroup::triggered, this,
            &MainWindow::onPortSelected);
    connect(serialPort, &QSerialPort::readyRead,
            this, &MainWindow::onSerialReadyRead);
}

void MainWindow::on_actionREFRESH_triggered() {
    for (auto *act : portGroup->actions()) {
        portMenu->removeAction(act);
        portGroup->removeAction(act);
        delete act;
    }
    for (auto &info : QSerialPortInfo::availablePorts()) {
        QAction *act = new QAction(
            QString("%1 (%2)").arg(info.portName(), info.description().isEmpty()
                                                        ? info.systemLocation()
                                                                                 : info.description()),
            this);
        act->setCheckable(true);
        act->setData(info.portName());
        portGroup->addAction(act);
        portMenu->addAction(act);
    }
    connectAction->setEnabled(false);
}
void MainWindow::onPortSelected(QAction *action) {
    action->setChecked(true);
    connectAction->setEnabled(true);
}
void MainWindow::on_actionCONNECT_triggered()
{
    QAction *sel = portGroup->checkedAction();
    if (!sel) { statusBar()->showMessage(tr("No port selected!"),0); return; }
    serialPort->setPort(QSerialPortInfo(sel->data().toString()));
    serialPort->setBaudRate(921600);
    if (serialPort->open(QIODevice::ReadWrite)) {
        connectionLabel->setText(tr("Connected: %1").arg(sel->data().toString()));
        refreshAction->setEnabled(false);
        connectAction->setEnabled(false);
        disconnectAction->setEnabled(true);
        // Disable load actions while connected
        ui->actionLOAD_LOOP1->setEnabled(false);
        ui->actionLOAD_LOOP2->setEnabled(false);

        ui->actionLIVE_ON->setEnabled(true);
        ui->actionLIVE_OFF->setEnabled(false);

        ui->actionRESET_MCU->setEnabled(true);
        ui->actionLED_TEST->setEnabled(true);
        ui->actionFORMAT_EEPROM->setEnabled(true);

        ui->btnCLA1->setEnabled(true);
        ui->btnCAL2->setEnabled(true);

        ui->actionEEPROM->setEnabled(true);
        ui->actionOPEN_PARAMETERS->setEnabled(true);
    }
    else {
        // Restore portName if we overwrote it above:
        connectionLabel->setText(tr(""));
        statusBar()->showMessage(
            tr("Failed to connect to %1").arg(sel->data().toString()),
            5000  // stay visible for 5s
            );
    }
}
void MainWindow::on_actionDISCONNECT_triggered()
{
    if (serialPort->isOpen()) serialPort->close();
    connectionLabel->clear();
    refreshAction->setEnabled(true);
    disconnectAction->setEnabled(false);
    connectAction->setEnabled(portGroup->checkedAction()!=nullptr);
    // Re-enable load actions when disconnected
    ui->actionLOAD_LOOP1->setEnabled(true);
    ui->actionLOAD_LOOP2->setEnabled(true);

    if (liveTimer->isActive()) {
        liveTimer->stop();
        autoScroll = false;   // also turn off auto‐scroll
    }
    liveDataLabel->setText(tr("Live: OFF"));
    ui->actionLIVE_ON->setEnabled(false);
    ui->actionLIVE_OFF->setEnabled(false);

    ui->actionRESET_MCU->setEnabled(false);
    ui->actionLED_TEST->setEnabled(false);
    ui->actionFORMAT_EEPROM->setEnabled(false);

    ui->actionEEPROM->setEnabled(false);
    ui->actionOPEN_PARAMETERS->setEnabled(false);

    ui->btnCLA1->setEnabled(false);
    ui->btnCAL2->setEnabled(false);
}

void MainWindow::addLoop1Data(double frequency)
{
    // 1) Append the new point
    series1->append(sampleCount1, frequency);
    ++sampleCount1;

    // 2) Recompute how far you can scroll: total_samples – windowSize
    int maxScroll = qMax(0, sampleCount1 - windowSize);

    // 3) Update scroll‐bar1’s range & page step
    scrollBar1->setRange(0, maxScroll);
    scrollBar1->setPageStep(windowSize);

    // 4) If we’re auto‐scrolling, jump thumb to the end
    if (autoScroll) {
        scrollBar1->setValue(maxScroll);
    }

    autoscaleYVisible(series1, axisX1, axisY1);

}
void MainWindow::addLoop2Data(double frequency)
{
    // 1) Append your new point
    series2->append(sampleCount2, frequency);
    ++sampleCount2;

    // 2) Compute how far you can scroll: total_samples – windowSize
    int maxScroll = qMax(0, sampleCount2 - windowSize);

    // 3) Update scroll‐bar range & page step
    scrollBar2->setRange(0, maxScroll);
    scrollBar2->setPageStep(windowSize);

    // 4) Only if you’re in “live” (autoScroll), push the thumb to the end:
    if (autoScroll) {
        scrollBar2->setValue(maxScroll);
    }

    autoscaleYVisible(series2, axisX2, axisY2);
}
void MainWindow::resetLoop1()
{
    // Clear series and reset sample counter
    series1->clear();
    sampleCount1 = 0;

    // Reset axes: X from 0 to windowSize; Y from 0 to 1
    axisX1->setRange(0, windowSize);
    axisY1->setRange(0, 1);

    // Fully reset scrollbar
    scrollBar1->setRange(0, 0);
    scrollBar1->setPageStep(windowSize);
    scrollBar1->setValue(0);
    scrollBar1->setEnabled(false);
}
void MainWindow::resetLoop2()
{
    // Clear series and reset sample counter
    series2->clear();
    sampleCount2 = 0;

    // Reset axes: X from 0 to windowSize; Y from 0 to 1
    axisX2->setRange(0, windowSize);
    axisY2->setRange(0, 1);

    // Fully reset scrollbar
    scrollBar2->setRange(0, 0);
    scrollBar2->setPageStep(windowSize);
    scrollBar2->setValue(0);
    scrollBar2->setEnabled(false);
}
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    auto handleChart = [&](QChart *chart,
                           QChartView *view,
                           QLineSeries *series,
                           QValueAxis *axisX,
                           QValueAxis *axisY,
                           int &sampleCount) -> bool
    {
        // Helper to clamp axis min ≥ 0 and max ≤ data bounds
        auto clampAxes = [&](){
            // X-axis clamp
            double minX = axisX->min();
            double maxX = axisX->max();
            // Ensure lower bound ≥ 0
            if (minX < 0) {
                double span = maxX - minX;
                minX = 0;
                maxX = span;
            }
            // Ensure upper bound ≤ total samples
            if (maxX > sampleCount) {
                maxX = sampleCount;
                minX = qMax(0.0, maxX - windowSize);
            }
            axisX->setRange(minX, maxX);

            autoscaleYVisible(series, axisX, axisY);
        };

        // Mouse wheel: zoom
        if (event->type() == QEvent::Wheel) {
            auto *we = static_cast<QWheelEvent*>(event);
            chart->zoom(we->angleDelta().y() > 0 ? 0.9 : 1.1);
            clampAxes();
            return true;
        }

        // Mouse drag: pan
        if (event->type() == QEvent::MouseMove && isPanning) {
            auto *me = static_cast<QMouseEvent*>(event);
            QPointF pixelDelta = me->pos() - lastMousePos;
            lastMousePos = me->pos();

            // Convert pixel delta to data delta
            QRectF plotArea = chart->plotArea();
            double dx = pixelDelta.x() * (axisX->max() - axisX->min())
                        / plotArea.width();
            double dy = -pixelDelta.y() * (axisY->max() - axisY->min())
                        / plotArea.height();

            // Shift axis ranges
            double newMinX = axisX->min() - dx;
            double newMaxX = axisX->max() - dx;
            axisX->setRange(newMinX, newMaxX);

            double newMinY = axisY->min() - dy;
            double newMaxY = axisY->max() - dy;
            axisY->setRange(newMinY, newMaxY);

            clampAxes();
            return true;
        }

        // Mouse press: start panning or reset
        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::MiddleButton) {
                chart->zoomReset();
                clampAxes();
                return true;
            }
            if (me->button() == Qt::LeftButton) {
                isPanning = true;
                lastMousePos = me->pos();
                return true;
            }
        }

        // Mouse release: end panning
        if (event->type() == QEvent::MouseButtonRelease) {
            auto *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                isPanning = false;
                return true;
            }
        }

        return false;
    };

    if (obj == chartView1) {
        if (handleChart(chart1, chartView1, series1, axisX1, axisY1, sampleCount1))
            return true;
    }
    if (obj == chartView2) {
        if (handleChart(chart2, chartView2, series2, axisX2, axisY2, sampleCount2))
            return true;
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::on_btnRESET1_clicked() { resetLoop1(); }
void MainWindow::on_btnRESET2_clicked() { resetLoop2(); }

void MainWindow::on_actionSAVE_LOOP_1_triggered() {
    QString fn = QFileDialog::getSaveFileName(this, tr("Save Loop 1"), QString(),
                                              tr("CSV Files (*.csv)"));
    if (fn.isEmpty())
        return;
    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream o(&f);
    o << "#Loop 1\n";
    for (auto &p : series1->pointsVector())
        o << p.x() << "," << p.y() << "\n";
}
void MainWindow::on_actionSAVE_LOOP_2_triggered() {
    QString fn = QFileDialog::getSaveFileName(this, tr("Save Loop 2"), QString(),
                                              tr("CSV Files (*.csv)"));
    if (fn.isEmpty())
        return;
    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream o(&f);
    o << "#Loop 2\n";
    for (auto &p : series2->pointsVector())
        o << p.x() << "," << p.y() << "\n";
}

void MainWindow::on_actionLOAD_LOOP1_triggered()
{
    if (serialPort->isOpen()) {
        statusBar()->showMessage(tr("Cannot load while connected."), 5000);
        return;
    }
    QString fn = QFileDialog::getOpenFileName(this, tr("Load Loop 1"), QString(), tr("CSV Files (*.csv)"));
    if (fn.isEmpty()) {
        statusBar()->showMessage(tr("Load canceled."), 2000);
        return;
    }
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        statusBar()->showMessage(tr("Failed to open file: %1").arg(fn), 5000);
        return;
    }
    QTextStream in(&f);
    QString header = in.readLine().trimmed();
    if (!header.contains("1")) {
        statusBar()->showMessage(tr("Invalid file format for Loop 1."), 5000);
        return;
    }
    series1->clear();
    sampleCount1 = 0;
    QVector<QPointF> pts;
    while (!in.atEnd()) {
        QStringList parts = in.readLine().split(',');
        if (parts.size() == 2) {
            bool okX, okY;
            double x = parts[0].toDouble(&okX);
            double y = parts[1].toDouble(&okY);
            if (okX && okY) {
                pts.append({ x, y });
                sampleCount1++;
            }
        }
    }
    series1->replace(pts);
    axisX1->setRange(0, windowSize);
    if (!pts.isEmpty()) {
        double minY = pts.first().y(), maxY = minY;
        for (const QPointF &p : pts) {
            minY = qMin(minY, p.y());
            maxY = qMax(maxY, p.y());
        }
        axisY1->setRange(minY, maxY);
    }
    scrollBar1->setRange(0, sampleCount1 - windowSize);
    scrollBar1->setValue(0);
    statusBar()->showMessage(tr("Loop 1 data loaded successfully."), 5000);
}
void MainWindow::on_actionLOAD_LOOP2_triggered()
{
    if (serialPort->isOpen()) {
        statusBar()->showMessage(tr("Cannot load while connected."), 5000);
        return;
    }
    QString fn = QFileDialog::getOpenFileName(this, tr("Load Loop 2"), QString(), tr("CSV Files (*.csv)"));
    if (fn.isEmpty()) {
        statusBar()->showMessage(tr("Load canceled."), 2000);
        return;
    }
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        statusBar()->showMessage(tr("Failed to open file: %1").arg(fn), 5000);
        return;
    }
    QTextStream in(&f);
    QString header = in.readLine().trimmed();
    if (!header.contains("2")) {
        statusBar()->showMessage(tr("Invalid file format for Loop 2."), 5000);
        return;
    }
    series2->clear();
    sampleCount2 = 0;
    QVector<QPointF> pts;
    while (!in.atEnd()) {
        QStringList parts = in.readLine().split(',');
        if (parts.size() == 2) {
            bool okX, okY;
            double x = parts[0].toDouble(&okX);
            double y = parts[1].toDouble(&okY);
            if (okX && okY) {
                pts.append({ x, y });
                sampleCount2++;
            }
        }
    }
    series2->replace(pts);
    axisX2->setRange(0, windowSize);
    if (!pts.isEmpty()) {
        double minY = pts.first().y(), maxY = minY;
        for (const QPointF &p : pts) {
            minY = qMin(minY, p.y());
            maxY = qMax(maxY, p.y());
        }
        axisY2->setRange(minY, maxY);
    }
    scrollBar2->setRange(0, sampleCount2 - windowSize);
    scrollBar2->setValue(0);
    statusBar()->showMessage(tr("Loop 2 data loaded successfully."), 5000);
}

void MainWindow::onSerialReadyRead()
{
    // Read and process each complete line
    while (serialPort->canReadLine()) {
        QByteArray rawLine = serialPort->readLine();       // includes the '\n'
        rawLine = rawLine.trimmed();                       // strips '\r' and '\n'
        QString line = QString::fromUtf8(rawLine);

        emit serialLineReceived(line);

        // Check for LIVE command
        if (line.startsWith("LIVE:")) {
            QString data = line.mid(QString("LIVE:").length());
            QStringList parts = data.split(',', Qt::KeepEmptyParts);
            if (parts.size() == 22) {
                bool ok;
                double freq0 = parts[0].toDouble(&ok);
                if (ok)
                {
                    addLoop1Data(freq0);
                }
                double freq1 = parts[1].toDouble(&ok);
                if (ok) addLoop2Data(freq1);
                // Parse other values
                int state0 = parts[2].toInt(&ok);
                int state1 = parts[3].toInt(&ok);
                double base0 = parts[4].toDouble(&ok);
                double base1 = parts[5].toDouble(&ok);
                double std0 = parts[6].toDouble(&ok);
                double std1 = parts[7].toDouble(&ok);
                double jump0 = parts[8].toDouble(&ok);
                double jump1 = parts[9].toDouble(&ok);
                double open0 = parts[10].toDouble(&ok);
                double open1 = parts[11].toDouble(&ok);
                double short0 = parts[12].toDouble(&ok);
                double short1 = parts[13].toDouble(&ok);
                int cal0 = parts[14].toInt(&ok);
                int cal1 = parts[15].toInt(&ok);
                int sens1 = parts[16].toInt(&ok);
                int sens2 = parts[17].toInt(&ok);
                int boost = parts[18].toInt(&ok);
                int freqChange = parts[19].toInt(&ok);
                int loop2Event = parts[20].toInt(&ok);
                int detectMode = parts[21].toInt(&ok);
                // Display on status bar
                // Build first half (up through calibration flags):
                QString line1 = tr("S0:%1  S1:%2  B0:%3  B1:%4 STD0:%5 STD1:%6 J0:%7  J1:%8  O0:%9  O1:%10  SH0:%11  SH1:%12  C0:%13  C1:%14")
                                    .arg(state0).arg(state1)
                                    .arg(base0,0,'f',1).arg(base1,0,'f',1)
                                    .arg(std0,0,'f',1).arg(std1,0,'f',1)
                                    .arg(jump0,0,'f',1).arg(jump1,0,'f',1)
                                    .arg(open0,0,'f',1).arg(open1,0,'f',1)
                                    .arg(short0,0,'f',1).arg(short1,0,'f',1)
                                    .arg(cal0).arg(cal1);

                // Build second half (sensitivities onward):
                QString line2 = tr("S1:%1  S2:%2  B:%3  FC:%4  L2:%5  M:%6")
                                    .arg(sens1).arg(sens2)
                                    .arg(boost).arg(freqChange)
                                    .arg(loop2Event).arg(detectMode);

                // Set both with a newline in between
                liveDataLabel->setText(line1 + "\n" + line2);
            } else {
                qDebug() << "LIVE format error:" << line;
            }
        } else {
        }
    }
}
void MainWindow::sendSerial(const QString &text)
{
    QString buffer = text.toLower();
    if (serialPort->isOpen()) {
        QByteArray data = buffer.toUtf8() + "\r\n";
        serialPort->write(data);
    } else {
        qDebug() << "Serial send failed: port not open";
    }
}

void MainWindow::on_actionLIVE_ON_triggered()
{
    if (!serialPort->isOpen()) {
        statusBar()->showMessage(tr("Cannot start Live: not connected"), 5000);
        return;
    }
    if (!liveTimer->isActive()) {
        liveTimer->start();
        autoScroll = true;      // enable auto‐scroll
        liveDataLabel->setText(tr("Live: ON"));
        ui->actionLIVE_ON->setEnabled(false);
        ui->actionLIVE_OFF->setEnabled(true);

        scrollBar1->setEnabled(false);
        scrollBar2->setEnabled(false);

        ui->actionRESET_MCU->setEnabled(false);
        ui->actionLED_TEST->setEnabled(false);
        ui->actionFORMAT_EEPROM->setEnabled(false);

        ui->actionSAVE_LOOP_1->setEnabled(false);
        ui->actionSAVE_LOOP_2->setEnabled(false);
    }
}
void MainWindow::on_actionLIVE_OFF_triggered()
{
    if (liveTimer->isActive()) {
        liveTimer->stop();
        autoScroll = false;     // disable auto‐scroll
        liveDataLabel->setText(tr("Live: OFF"));
        ui->actionLIVE_ON->setEnabled(true);
        ui->actionLIVE_OFF->setEnabled(false);

        if (sampleCount1 > windowSize) scrollBar1->setEnabled(true);
        if (sampleCount2 > windowSize) scrollBar2->setEnabled(true);

        ui->actionRESET_MCU->setEnabled(true);
        ui->actionLED_TEST->setEnabled(true);
        ui->actionFORMAT_EEPROM->setEnabled(true);

        ui->actionSAVE_LOOP_1->setEnabled(true);
        ui->actionSAVE_LOOP_2->setEnabled(true);
    }
}

void MainWindow::on_actionRESET_MCU_triggered()
{
    if (autoScroll) return;       // only when live is off
    sendSerial("reset");
}
void MainWindow::on_actionLED_TEST_triggered()
{
    if (autoScroll) return;
    sendSerial("led_test");
}
void MainWindow::on_actionFORMAT_EEPROM_triggered()
{
    if (autoScroll) return;
    auto reply = QMessageBox::question(this,
                                       tr("Confirm EEPROM Format"),
                                       tr("This will erase all EEPROM data. Are you sure?"),
                                       QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        sendSerial("format");
    }
}

void MainWindow::on_btnCLA1_clicked()
{
    if (serialPort->isOpen()) {
        sendSerial("cal1");
    }
}
void MainWindow::on_btnCAL2_clicked()
{
    if (serialPort->isOpen()) {
        sendSerial("cal2");
    }
}

void MainWindow::on_actionEEPROM_triggered()
{
        if (!eepromDialog) {
            eepromDialog = new EEPROMDialog(serialPort, this);
            // listen for lines
            connect(this,
                    &MainWindow::serialLineReceived,
                    eepromDialog,
                    &EEPROMDialog::appendLine);
        }
        eepromDialog->show();
        eepromDialog->raise();
        eepromDialog->requestData();
}
void MainWindow::on_actionOPEN_PARAMETERS_triggered()
{
    if (!parametersDialog) {
        parametersDialog = new ParametersDialog(serialPort, this);
        // forward all incoming lines into it
        connect(this,
                &MainWindow::serialLineReceived,
                parametersDialog,
                &ParametersDialog::onSerialLineReceived);
    }
    parametersDialog->show();
    parametersDialog->raise();
    parametersDialog->onRefreshClicked();  // fetch current params
}
void MainWindow::autoscaleYVisible(QLineSeries* series, QValueAxis* axisX, QValueAxis* axisY) {
    // 1) alle Punkte im sichtbaren X-Bereich sammeln
    const auto minX = axisX->min();
    const auto maxX = axisX->max();
    QVector<double> yVals;
    for (const auto &pt : series->pointsVector()) {
        if (pt.x() >= minX && pt.x() <= maxX)
            yVals.append(pt.y());
    }
    if (yVals.empty()) return;

    // 2) sortieren
    std::sort(yVals.begin(), yVals.end());

    // 3) Einmal-Spikes herausschneiden:
    //    Wenn das kleinste bzw. größte Element nur einmal vorkommt,
    //    überspringe es bei der Min/Max-Berechnung.
    int start = 0;
    int end   = int(yVals.size()) - 1;
    if (end > start) {
        // kleiner Spike?
        if (yVals[0] != yVals[1]) start++;
        // großer Spike?
        if (yVals[end] != yVals[end-1]) end--;
    }

    // 4) Range neu setzen
    axisY->setRange(yVals[start], yVals[end]);
}
