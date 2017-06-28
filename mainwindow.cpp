/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the QtSerialBus module.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
/*THIS PROJECT WAS BASED ON QT MODBUS EXAMPLES
for stepic C/C++ course*/

/*Some comments were added for reviewers
in this step we will analyze the architecture and implement some patterns

This project implements MVC (Model-View-Controller) architecture pattern.

This pattern usually consicts of three components :
> Model (modbusDevice entity of QModbusRtuSerialSlave class or QModbusTcpServer class, depending on options)
> View (ui,QMainwindow class, QDialog class)
> Controller (The controller implemented in the same Mainwindow class, with some functions(methods))
*/

#include "mainwindow.h"
#include "settingsdialog.h"
#include "ui_mainwindow.h"

#include <QModbusRtuSerialSlave>
#include <QModbusTcpServer>
#include <QRegularExpression>
#include <QStatusBar>
#include <QUrl>

enum ModbusConnection {
    Serial,
    Tcp
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , modbusDevice(nullptr)
{
    ui->setupUi(this);


    ui->connectType->setCurrentIndex(0);
    on_connectType_currentIndexChanged(0);

    m_settingsDialog = new SettingsDialog(this);
    init();
}

MainWindow::~MainWindow()
{
    if (modbusDevice)
        modbusDevice->disconnectDevice();
    delete modbusDevice;

    delete ui;
}

void MainWindow::init()
{
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionExit->setEnabled(true);
    ui->actionOptions->setEnabled(true);

    connect(ui->actionConnect, &QAction::triggered,
            this, &MainWindow::on_connectButton_clicked);
    connect(ui->actionDisconnect, &QAction::triggered,
            this, &MainWindow::on_connectButton_clicked);

    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);
    connect(ui->actionOptions, &QAction::triggered, m_settingsDialog, &QDialog::show);
}

void MainWindow::on_connectType_currentIndexChanged(int index) /*This function creates the "model" of 2 different types, depending on the settings given in the ui*/
{
    if (modbusDevice) {
        modbusDevice->disconnect();
        delete modbusDevice;
        modbusDevice = nullptr;
    }

    ModbusConnection type = static_cast<ModbusConnection> (index);
    if (type == Serial) {
        modbusDevice = new QModbusRtuSerialSlave(this);
    } else if (type == Tcp) {
        modbusDevice = new QModbusTcpServer(this);
        if (ui->portEdit->text().isEmpty())
            ui->portEdit->setText(QLatin1Literal("127.0.0.1:502"));
    }
    ui->listenOnlyBox->setEnabled(type == Serial);

    if (!modbusDevice) {
        ui->connectButton->setDisabled(true);
        if (type == Serial)
            statusBar()->showMessage(tr("Could not create Modbus slave."), 0);
        else
            statusBar()->showMessage(tr("Could not create Modbus server."), 0);
    } else {

        connect(modbusDevice, &QModbusServer::stateChanged,
                this, &MainWindow::onStateChanged);
        connect(modbusDevice, &QModbusServer::errorOccurred,
                this, &MainWindow::handleDeviceError);

    }
}

void MainWindow::handleDeviceError(QModbusDevice::Error newError) /*This function handles errors (: */
{
    if (newError == QModbusDevice::NoError || !modbusDevice)
        return;

    statusBar()->showMessage(modbusDevice->errorString(), 0);
}

void MainWindow::on_connectButton_clicked() /*This function catches "connect" button activations and sets connection parameter for the model*/
{                                           /*and then fills the model contents with given data(from ui table) or simply with zeroes*/
    bool intendToConnect = (modbusDevice->state() == QModbusDevice::UnconnectedState);

    statusBar()->clearMessage();

    if (intendToConnect) {
        if (static_cast<ModbusConnection> (ui->connectType->currentIndex()) == Serial) {
            modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter,
                ui->portEdit->text());
            modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter,
                m_settingsDialog->settings().parity);
            modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,
                m_settingsDialog->settings().baud);
            modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,
                m_settingsDialog->settings().dataBits);
            modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,
                m_settingsDialog->settings().stopBits);
        } else {
            const QUrl url = QUrl::fromUserInput(ui->portEdit->text());
            modbusDevice->setConnectionParameter(QModbusDevice::NetworkPortParameter, url.port());
            modbusDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter, url.host());
        }
        modbusDevice->setServerAddress(ui->serverEdit->text().toInt());
        if (!modbusDevice->connectDevice()) {
            statusBar()->showMessage(tr("Connect failed: ") + modbusDevice->errorString(), 0);
        } else {
            ui->actionConnect->setEnabled(false);
            ui->actionDisconnect->setEnabled(true);
            statusBar()->showMessage(tr("Connected") , 0);
        }
    } else {
        modbusDevice->disconnectDevice();
        ui->actionConnect->setEnabled(true);
        ui->actionDisconnect->setEnabled(false);
        statusBar()->showMessage(tr("Disconnected") , 0);
    }
    bool ok = true;
    int registerQuantity = ui->registerQuantity->value();
    for(int i = 0 ; i < registerQuantity ; i++){
    modbusDevice->setData(QModbusDataUnit::InputRegisters, i, 0);
    modbusDevice->setData(QModbusDataUnit::HoldingRegisters, i, 0);
    }
    for(int i = 0, m = ui->registerQuantity->value() / 10 + 1; i < m ; i++){
        for(int j = 0 ; j < 10 ; j++){
            if(ui->tableWidget->item(i,j))
            {
                ok = modbusDevice->setData(QModbusDataUnit::InputRegisters,
                                           i*10 + j, ui->tableWidget->item(i,j)
                                           ->text().toInt(&ok, 16));
                ok = modbusDevice->setData(QModbusDataUnit::HoldingRegisters,
                                           i*10 + j, ui->tableWidget->item(i,j)
                                           ->text().toInt(&ok, 16));

                if (!ok)
                    statusBar()->showMessage(tr("Could not set register: ") + modbusDevice->errorString(),
                                             0);
            }
        }
    }
}

void MainWindow::onStateChanged(int state) /*this function switches the string content located on the button depending on model status */
{
    bool connected = (state != QModbusDevice::UnconnectedState);
    ui->actionConnect->setEnabled(!connected);
    ui->actionDisconnect->setEnabled(connected);

    if (state == QModbusDevice::UnconnectedState){
        ui->connectButton->setText(tr("Connect"));

    }
    else if (state == QModbusDevice::ConnectedState)
        ui->connectButton->setText(tr("Disconnect"));
}


void MainWindow::on_registerQuantity_valueChanged() /*this function watches registers quantity for the model(capacity changes and modifies the ui table and the model(capacity) */
{
    ui->tableWidget->setRowCount(ui->registerQuantity->value() / 10 + 1);
    ui->tableWidget->setColumnCount(10);
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "+0" << "+1" << "+2"
                                               << "+3" << "+4" << "+5" << "+6" << "+7"
                                               << "+8" << "+9" );
    ui->tableWidget->setVerticalHeaderLabels(QStringList() << "+0" << "+10" << "+20"
                                             << "+30" << "+40" << "+50" << "+60" << "+70"
                                             << "+80" << "+90" );

    QModbusDataUnitMap reg;

    reg.insert(QModbusDataUnit::InputRegisters, { QModbusDataUnit::InputRegisters, 0, ui->registerQuantity->value() + 10 });
    reg.insert(QModbusDataUnit::HoldingRegisters, { QModbusDataUnit::HoldingRegisters, 0, ui->registerQuantity->value() + 10 });

    modbusDevice->setMap(reg);


}

void MainWindow::on_tableWidget_cellChanged(int row, int column)/*this function watches for changes in the table and if there is a change in the tables it will modify the model*/
{
    bool ok = true;
    if(modbusDevice->state() == QModbusDevice::ConnectedState){
        ok = modbusDevice->setData(QModbusDataUnit::InputRegisters,
                              row*10 + column, ui->tableWidget->item(row,column)->text().toInt(&ok, 10));
        ok = modbusDevice->setData(QModbusDataUnit::HoldingRegisters,
                              row*10 + column, ui->tableWidget->item(row,column)->text().toInt(&ok, 10));
    }
    if (!ok)
        statusBar()->showMessage(tr("Could not set register: ") + modbusDevice->errorString(),
                                 0);

}

/*thanks for reading*/
