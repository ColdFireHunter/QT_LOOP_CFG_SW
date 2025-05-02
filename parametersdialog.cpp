// parametersdialog.cpp
#include "parametersdialog.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>

ParametersDialog::ParametersDialog(QSerialPort *port, QWidget *parent)
    : QDialog(parent), serialPort(port), initializing(true) {
    setWindowTitle(tr("Parameters"));

    // Define commands and their high limits
    commands = {"sens1_low",
                "sens1_medium",
                "sens1_high",
                "sens2_low",
                "sens2_medium",
                "sens2_high",
                "open_loop1",
                "open_loop2",
                "short_loop1",
                "short_loop2",
                "boost_loop1",
                "boost_loop2",
                "output_polarity_auf1",
                "output_polarity_auf2",
                "output_polarity_zu",
                "blanking_time",
                "pulse_time",
                "out_error_polarity",
                "rnd_recal_enable",
                "seq_reset_enable",
                "seq_timeout_ms",
                "mode3"};
    // corresponding high limits (booleans are 0–1, timeout up to e.g. 60000)
    highLimits = QVector<int>({
        65535, 65535, 65535,    // sens1
        65535, 65535, 65535,    // sens2
        65535, 65535,           // open loops
        65535, 65535,           // short loops
        65535, 65535,           // boosts
        1,     1,               // auf polarities
        1,     65535, 65535, 1, // zu / blank / pulse / error pol
        1,     1,               // rnd_recal / seq_reset
        65535,                  // seq_timeout_ms
        1                       // mode3
    });

    int rows = commands.size();
    table = new QTableWidget(rows, 2, this);
    table->setHorizontalHeaderLabels({tr("Parameter"), tr("Value")});
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::DoubleClicked |
                           QAbstractItemView::SelectedClicked |
                           QAbstractItemView::EditKeyPressed);

    // Populate names and default zeros
    for (int i = 0; i < rows; ++i) {
        table->setItem(i, 0, new QTableWidgetItem(commands[i]));
        auto *item = new QTableWidgetItem("0");
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        table->setItem(i, 1, item);
    }
    connect(table, &QTableWidget::cellChanged, this,
            &ParametersDialog::onCellChanged);

    btnRefresh = new QPushButton(tr("Refresh"), this);
    btnLoad = new QPushButton(tr("Load"), this);
    btnSave = new QPushButton(tr("Save"), this);
    btnOk = new QPushButton(tr("OK"), this);
    connect(btnRefresh, &QPushButton::clicked, this,
            &ParametersDialog::onRefreshClicked);
    connect(btnLoad, &QPushButton::clicked, this,
            &ParametersDialog::onLoadClicked);
    connect(btnSave, &QPushButton::clicked, this,
            &ParametersDialog::onSaveClicked);
    connect(btnOk, &QPushButton::clicked, this, &ParametersDialog::onOkClicked);

    auto *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(btnRefresh);
    btnLayout->addWidget(btnLoad);
    btnLayout->addWidget(btnSave);
    btnLayout->addStretch();
    btnLayout->addWidget(btnOk);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(table);
    mainLayout->addLayout(btnLayout);

    // Subscribe to parameter lines
    connect(parent, SIGNAL(serialLineReceived(QString)), this,
            SLOT(onSerialLineReceived(QString)));

    for (int i = 0; i < rows; ++i) {
        auto *nameItem = new QTableWidgetItem(commands[i]);
        // Make parameter names read-only:
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(i, 0, nameItem);

        auto *valItem = new QTableWidgetItem("0");
        valItem->setFlags(valItem->flags() | Qt::ItemIsEditable);
        table->setItem(i, 1, valItem);
    }

    // Set the size of the dialog
    setMinimumSize(800, 600);
    setMaximumSize(1280, 720);

    // Initial fetch
    onRefreshClicked();
    initializing = false;

    originalValues.resize(commands.size());
}

void ParametersDialog::onRefreshClicked() {
    if (serialPort->isOpen())
        serialPort->write("param\r\n");
}

void ParametersDialog::onLoadClicked()
{
    if (serialPort->isOpen()) {
        serialPort->write("load\r\n");
        // After loading from EEPROM, re-fetch the PARAMETERS: line
        QTimer::singleShot(50, this, &ParametersDialog::onRefreshClicked);
    }
}

void ParametersDialog::onSaveClicked() {
    if (serialPort->isOpen())
        serialPort->write("save\r\n");
}

void ParametersDialog::onOkClicked() { close(); }

void ParametersDialog::onCellChanged(int row, int column)
{
    if (initializing || column != 1)
        return;

    bool ok;
    int val = table->item(row,1)->text().toInt(&ok);
    if (!ok || val < 0 || val > highLimits[row]) {
        QMessageBox::warning(
            this,
            tr("Invalid value"),
            tr("Value must be between 0 and %1").arg(highLimits[row])
            );
        table->item(row,1)->setText(originalValues[row]);
        return;
    }

    // Send the updated value
    QString cmd = QString("%1=%2").arg(commands[row]).arg(val);
    sendCommand(cmd);

    // Remember it
    originalValues[row] = table->item(row,1)->text();

    // Re-fetch after a 50 ms delay
    QTimer::singleShot(50, this, &ParametersDialog::onRefreshClicked);
}

void ParametersDialog::onSerialLineReceived(const QString &line) {
    if (!line.startsWith("PARAMETERS:")) return;
    auto parts = line
                     .mid(QString("PARAMETERS:").length())
                     .split(',', Qt::SkipEmptyParts);
    if (parts.size() != commands.size()) return;

    initializing = true;
    for (int i = 0; i < parts.size(); ++i) {
        table->item(i,1)->setText(parts[i]);
        originalValues[i] = parts[i];    // <-- remember it
    }
    initializing = false;
}

void ParametersDialog::sendCommand(const QString &cmd) {
    if (serialPort->isOpen()) {
        serialPort->write(cmd.toUtf8() + "\r\n");
    }
}
