/***************************************************************************
 *   Copyright (C) 2017 by Jeremy Whiting <jeremypwhiting@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation version 2 of the License.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include "vpnservicemanager.h"

#include "authmanager.h"
#include "common.h"
#include "log.h"
#include "pathhelper.h"
#include "setting.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QMessageBox>
#include <QThread>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTemporaryFile>
#include <QProcess>
#include <QDir>

#define LOCAL_SOCKET_TIMEOUT 500

VPNServiceManager* VPNServiceManager::mInstance = 0;

VPNServiceManager::VPNServiceManager(QObject *parent)
    : QObject(parent),
      mConnected(false),
      mInPortLoop(false),
      mPortDialogShown(false),
      mUserRequestedDisconnect(false)
{
    m_socket.setServerName(kSocketName);
    QObject::connect(&m_socket, &QLocalSocket::readyRead, this, &VPNServiceManager::socket_readyRead);
    QObject::connect(&m_socket, &QLocalSocket::disconnected, this, &VPNServiceManager::socket_disconnected);
    QObject::connect(&m_socket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(socket_error(QLocalSocket::LocalSocketError)));
}

bool VPNServiceManager::ensureConnected()
{
    if (!mConnected) {
        qDebug() << "ensureConnected called, but not connected, connecting now.";
        mConnected = connectToCore();
        if (!mConnected) {
            // Sleep and try one more time in case it is starting up
            QThread::sleep(3);
            mConnected = connectToCore();
            if (!mConnected) {
                QMessageBox::critical(nullptr, tr("Connection error"), tr("Connection to Celo VPN service was closed.\nPlease ensure service is running"));
//                emit serviceUnavailable();
            }
        }
    }

    return mConnected;
}

void VPNServiceManager::sendCommand(const QJsonObject &object)
{
    QJsonDocument jDoc(object);
    QString strJson(jDoc.toJson(QJsonDocument::Compact));
    m_socket.write(strJson.toLatin1(), strJson.length());
    m_socket.write("\n");

    if (!m_socket.waitForBytesWritten(LOCAL_SOCKET_TIMEOUT))
        Log::logt(QString("Pending sending command ") + strJson);
    else
        Log::logt(QString("Sent command ") + strJson);
}

VPNServiceManager *VPNServiceManager::instance()
{
    if (mInstance == 0)
        mInstance = new VPNServiceManager(0);
    return mInstance;
}

bool VPNServiceManager::exists()
{
    return mInstance != 0;
}

void VPNServiceManager::cleanup()
{
    if (mInstance != NULL) {
        mInstance->sendDisconnectFromVPNRequest();
        mInstance->deleteLater();
        mInstance = 0;
    }
}

VPNServiceManager::~VPNServiceManager()
{
    Log::logt("destructing vpnservice manager, calling disconnect to disconnect");

    // disconnect connection
    disconnectFromCore();
}

vpnState VPNServiceManager::state()
{
    return mState;
}

bool VPNServiceManager::connectToCore()
{
    Log::logt("Trying to connect to service ");

    if (!m_socket.isValid()) {
        m_socket.setServerName(kSocketName);
        qDebug() << "Attempting to connect to server on socket " << m_socket.serverName();
        m_socket.connectToServer(kSocketName);
        if (m_socket.waitForConnected(500)) {
            Log::logt("Connect to celovpn service suceeded");
            mConnected = true;
        } else {
            Log::logt("Could not connect to celovpn service within half a second");
            qDebug() << "socket state is " << m_socket.state();
            qDebug() << "socket error string is " << m_socket.errorString();
            qDebug() << "socket name is " << m_socket.serverName();
        }
    }

    return m_socket.isValid();
}

void VPNServiceManager::disconnectFromCore()
{
    // No need to disconnect if we are disconnected
    if (m_socket.isOpen()) {
        Log::logt("Disconnecting from celovpn service called, socket is open, so closing");

        // close socket
        m_socket.close();
    }

    // set connection state flag. We either disconnected just now or were
    // already disconnected
    Log::logt("Setting connected to false since socket is disconnected");
    mConnected = false;
}

void VPNServiceManager::sendStatusRequest()
{
    if (ensureConnected()) {
        QJsonObject jObj;

        Log::logt("Sending command 'getStatus'");

        jObj["cmd"] = cmdGetStatus;

        sendCommand(jObj);
    } else {
        emit error(QString("VPNServiceConnection is not open for method %1").arg(__FUNCTION__));
    }
}

void VPNServiceManager::sendCredentials()
{
    Log::logt("sendCredentials called");
    if (ensureConnected()) {
        QJsonObject jObj;

        Log::logt("Sending command 'cmdSetCredentials'");
        jObj["cmd"] = cmdSetCredentials;
        jObj["vpnusername"] = AuthManager::instance()->VPNName();
        jObj["vpnpassword"] = AuthManager::instance()->VPNPassword();

        sendCommand(jObj);
    } else {
        emit error(QString("VPNServiceConnection is not open for method %1").arg(__FUNCTION__));
    }
}

void VPNServiceManager::sendConnectToVPNRequest()
{
    Log::logt(QString("sendConnectToVPNRequest called"));

    mUserRequestedDisconnect = false;

    if (ensureConnected()) {
        QJsonObject jObj;

        Log::logt("Sending command 'connectToVPN'");

        int serverId = Setting::instance()->serverID();
        AServer server = AuthManager::instance()->getServer(serverId);

        jObj["cmd"] = cmdStart;
        jObj["encryption"] = Setting::instance()->encryption();
        jObj["serverid"] = Setting::instance()->serverID();
        jObj["server"] = Setting::instance()->serverAddress();
        jObj["port"] = Setting::instance()->port();
        jObj["template"] = server.configUrls.value(Setting::instance()->port().toInt());
        jObj["localport"] = Setting::instance()->localPort();
        jObj["tcporudp"] = Setting::instance()->tcpOrUdp();
        jObj["fixDNS"] = Setting::instance()->fixDns();

        if (!Setting::instance()->dns1().isEmpty())
            jObj["dns1"] = Setting::instance()->dns1();
        else
            jObj["dns1"] = Setting::instance()->defaultDNS1();

        if (!Setting::instance()->dns2().isEmpty())
            jObj["dns2"] = Setting::instance()->dns2();
        else
            jObj["dns2"] = Setting::instance()->defaultDNS2();

        sendCommand(jObj);
    } else {
        Log::logt("socket to send connect command to is not open");
    }
}

void VPNServiceManager::sendDisconnectFromVPNRequest()
{
    Log::logt(QString("sendDisconnectFromVPNRequest called"));
    mUserRequestedDisconnect = true;
    if (ensureConnected()) {
        QJsonObject jObj;

        Log::logt("Sending command 'disconnectFromVPN'");

        jObj["cmd"] = cmdStop;

        sendCommand(jObj);
    } else {
        Log::logt("gui is not connected to celovpn service to send disconnect from vpn command");
    }
}

void VPNServiceManager::killRunningOpenvpn()
{
    Log::logt(QString("killRunningOpenvpn called"));
    if (ensureConnected()) {
        QJsonObject jObj;

        Log::logt("Sending command 'killRunningOpenvpn'");

        jObj["cmd"] = cmdKillRunningOpenvpn;

        sendCommand(jObj);
    } else {
        Log::logt("gui is not connected to celovpn service to send kill running openvpn command");
    }
}

void VPNServiceManager::sendNetdownCommand()
{
    Log::logt(QString("sendNetdownCommand called"));
    if (ensureConnected()) {
        QJsonObject jObj;

        Log::logt("Sending command 'cmdNetDown'");

        jObj["cmd"] = cmdNetdown;

        sendCommand(jObj);
    } else {
        Log::logt("gui is not connected to celovpn service to send netdown command");
    }
}

void VPNServiceManager::socket_readyRead()
{
    QString notifyStr = m_socket.readAll();
    QStringList notifyStrList = notifyStr.split('\n', QString::SkipEmptyParts);

    Q_FOREACH(const QString &notifyToken, notifyStrList) {
        qDebug() << "Notification token is : " << notifyToken;

        // parse json
        QJsonDocument jsonDoc = QJsonDocument::fromJson(notifyToken.toLatin1());
        QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.isEmpty()) {
            qDebug() << "json object is empty, so continuing";
            continue;
        }

        qint32 code = jsonObj.value("code").toInt();
        qDebug() << "Notification code is " << code;

        switch (code) {
        case notifyStatusChange: {
            qint32 state = jsonObj.value("state").toInt();
            qDebug() << "Got state change to " << state;

//            Log::logt(QString("Got state change to %1").arg(vpnStateWord((vpnState)state)));

            mState = (vpnState)state;
            // Now that we are connected, stop port loop
            if (mState == vpnStateConnected)
                mInPortLoop = false;
            else if (mState == vpnStateDisconnected && !mUserRequestedDisconnect)
                emit killSwitch(); // Emit kill switch signal, login window will perform if needed

            emit stateChanged(mState);
        }
        break;

        case notifyStatusWord: {
            qint32 state = jsonObj.value("state").toInt();
            qDebug() << "Got state word change to word " << state;
//            Log::logt(QString("Got status word change to %1").arg(vpnStateWord((vpnState)state)));
            emit stateWord((OpenVPNStateWord)state);
        }
        break;

        case notifyGotIP: {
            QString newIP = jsonObj.value("ip").toString();
            emit gotNewIp(newIP);
        }
        break;

        case notifyTimeout: {
            // Openvpn timed out, so switch to the next port or server
            // asking the user accordingly
            if (mInPortLoop)
                tryNextPort();
            else if (mPortDialogShown) {
                // Do nothing, wait for user to choose which
            } else {
                mPortDialogShown = true;
                emit timedOut();
            }
        }
        break;

        case notifyError: {
            QString message = jsonObj.value("message").toString();
            qDebug() << "Got error notification from service " << message;
            emit error(message);
        }

        default:
            break;
        }
    }
}

void VPNServiceManager::socket_disconnected()
{
    Log::logt("Socket disconnected from celo vpn service. it crashed or is restarting");
    disconnectFromCore();
}

void VPNServiceManager::socket_error(QLocalSocket::LocalSocketError error)
{
    Log::logt("Socket error " + QString::number(error));
}

void VPNServiceManager::startPortLoop(bool changePort)
{
    mChangingPorts = changePort;
    mPortDialogShown = false;
    mInPortLoop = true;
    tryNextPort();
}

void VPNServiceManager::tryNextPort()
{
    if (mChangingPorts)
        Setting::instance()->switchToNextPort();
    else
        Setting::instance()->switchToNextNode();
    sendConnectToVPNRequest();
}

