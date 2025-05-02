#include "eepromdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QRegularExpression>

EEPROMDialog::EEPROMDialog(QSerialPort* port, QWidget* parent)
    : QDialog(parent), serialPort(port)
{
    setWindowTitle(tr("EEPROM Contents"));

    // Table: 128 rows × 17 cols (addr + 16 bytes)
    table = new QTableWidget(128, 17, this);
    QStringList headers;
    headers << tr("Addr");
    for (int i = 0; i < 16; ++i)
        headers << QString::asprintf("%02X", i);
    table->setHorizontalHeaderLabels(headers);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Only vertical scrolling
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked,
            this, &EEPROMDialog::onRefreshClicked);

    auto *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(refreshBtn);
    btnLayout->addStretch();

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5,5,5,5);
    mainLayout->setSpacing(5);
    mainLayout->addWidget(table);
    mainLayout->addLayout(btnLayout);

    // Limit height to 720 pixels (or less) so vertical scroll appears if needed
    setMaximumHeight(720);
    setMaximumWidth(1280);

    setMinimumWidth(800);
    setMinimumHeight(600);

    // Listen for broadcasted lines
    // connected externally in MainWindow

    // Initial load
    requestData();
}

void EEPROMDialog::requestData()
{
    table->clearContents();
    // send the command
    if (serialPort->isOpen()) {
        serialPort->write("eeprom\r\n");
    }
}

void EEPROMDialog::onRefreshClicked()
{
    requestData();
}

void EEPROMDialog::appendLine(const QString &line)
{
    // Only handle lines starting with the hex address "0x"
    if (!line.startsWith("0x"))
        return;

    // Split on any whitespace
    QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (parts.size() < 17)
        return;  // must have addr + 16 bytes

    bool ok;
    int addr = parts.at(0).mid(2).toInt(&ok, 16);
    if (!ok) return;
    int row = addr / 16;

    // Fill the address column
    table->setItem(row, 0, new QTableWidgetItem(parts.at(0)));
    // Fill the 16 byte columns, center-aligned
    for (int i = 1; i <= 16; ++i) {
        QString byteStr = parts.value(i, "--");
        auto *item = new QTableWidgetItem(byteStr);
        item->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, i, item);
    }
}
