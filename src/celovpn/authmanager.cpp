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

#include "authmanager.h"

#include <cassert>
#include <algorithm>
#include <set>
#include <stdexcept>

#include <QDomDocument>

#include "pingwaiter.h"
#include "setting.h"
#include "mapscreen.h"
#include "loginwindow.h"
#include "osspecific.h"
#include "log.h"
#include "flag.h"
#include "common.h"
#include "update.h"
#include "version.h"
#include "vpnservicemanager.h"
#include "wndmanager.h"

std::auto_ptr<AuthManager> AuthManager::mInstance;
AuthManager * AuthManager::instance()
{
    if (!mInstance.get())
        mInstance.reset(new AuthManager());
    return mInstance.get();
}

bool AuthManager::exists()
{
    return (mInstance.get() != NULL);
}

void AuthManager::cleanup()
{
    if (mInstance.get() != NULL)
        delete mInstance.release();
}

AuthManager::AuthManager()
    :mLoggedIn(false),
     mCancellingLogin(false),
     mSeeded(false),
     mIPAttemptCount(0)
{
    connect(VPNServiceManager::instance(), &VPNServiceManager::gotNewIp,
            this, &AuthManager::setNewIp);
}

AuthManager::~AuthManager()
{
    for (size_t k = 0; k < mWorkers.size(); ++k) {
        if (mTimers.at(k) != NULL) {
            mTimers.at(k)->stop();
            delete mTimers.at(k);
        }
        /*              if (_workers.at(k) != NULL && _waiters.at(k) != NULL)
                        {
                                SjMainWindow * m = SjMainWindow::Instance();
                        }*/
        if (mWorkers.at(k) != NULL) {
            if (mWorkers.at(k)->state() != QProcess::NotRunning)
                mWorkers.at(k)->terminate();
            mWorkers.at(k)->deleteLater();
        }
        if (mWaiters.at(k) != NULL)
            delete mWaiters.at(k);
    }
    // TODO: -0 terminate Network Manager
//      _nam
}

bool AuthManager::loggedIn()
{
    // TODO: -0 not implemented
    return mLoggedIn;
}

void AuthManager::login(const QString & name, const QString & password)
{
    mVPNLogin = name;
    mVPNPassword = password;

    mLoggedIn = true;
    mCancellingLogin = false;

    VPNServiceManager::instance()->sendCredentials();

    mReply.reset(mNAM.get(BuildRequest(QUrl::fromUserInput("https://celo.net/celo-ovpn-servers.xml"))));
    Log::logt("request url is " + mReply->url().toString());
    connect(mReply.get(), &QNetworkReply::finished,
            this, &AuthManager::loginFinished);
}

void AuthManager::cancel()
{
    mCancellingLogin = true;
    if(NULL != mReply.get()) {
        mReply->abort();
    }
}

void AuthManager::logout()
{
    cancel();
    mLoggedIn = false;
    mVPNLogin.clear();
    mVPNPassword.clear();

    emit logoutCompleted();
}

const QString &AuthManager::VPNName()
{
    return mVPNLogin;    // TODO: -1 check: still valid
}

const QString &AuthManager::VPNPassword()
{
    return mVPNPassword;
}

const QString &AuthManager::newIP()
{
    return mNewIP;
}

const QString &AuthManager::oldIP()
{
    return mOldIP;
}

AServer AuthManager::getServer(int id)
{
    AServer s;
    //assert(id > -1);
    if (id > -1 && id < mServers.size()) {
        s = mServers.at(id);
    } else {
        Log::logt("getServer called with id " + QString::number(id));
    }
    return s;
}

void AuthManager::setNewIp(const QString & ip)
{
    static const QString self = "127.0.0.1";
    if (ip != self) {
        mNewIP = ip;
        emit newIpLoaded(ip);
    }
}

const QList<int> &AuthManager::currentEncryptionServers()
{
    int enc = Setting::instance()->encryption();
    if (enc != ENCRYPTION_RSA) {
        return mServerIds[enc];
    } else { // It's rsa, so check the current port type also (tcp or udp)
        if (Setting::instance()->tcpOrUdp() == "tcp")
            return mTcpServerIds;
        else
            return mUdpServerIds;
    }
}

int AuthManager::serverIxFromName(const QString & srv)
{
    int ix = -1;
    if (mServerNameToId.empty() && !mServers.isEmpty()) {
        for (size_t k = 0, sz = mServers.size(); k < sz; ++k)
            mServerNameToId.insert(SIMap::value_type(mServers.at(k).name.toStdString(), k));
    }
    SIMap::iterator it = mServerNameToId.find(srv.toStdString());
    if (it != mServerNameToId.end())
        ix = (*it).second;
    return ix;
}

int AuthManager::pingFromServerIx(int srv)
{
    int pn = -1;
    if (srv > -1 && srv < mPings.size())
        pn = mPings.at(srv);
    return pn;
}

void AuthManager::clearServerLists()
{
    mServerNameToId.clear();
    mServers.clear();
    mPings.clear();
}

void AuthManager::clearReply()
{
    if (mReply.get() != NULL) {
        mReply->abort();
        mReply->deleteLater();
        mReply.release();
    }
}

void AuthManager::checkUpdates()
{
    QString us(UPDATE_URL);
    if (!us.isEmpty()) {
        Log::logt(QString("Checking for updates from %1").arg(UPDATE_URL));
        mUpdateReply.reset(AuthManager::instance()->mNAM.get(BuildRequest(QUrl(us))));
        connect(mUpdateReply.get(), &QNetworkReply::finished,
                this, &AuthManager::processUpdatesXml);
    }
}

void AuthManager::getOldIP()
{
    ++mIPAttemptCount;
    Log::logt("StartDwnl_OldIp() attempt " + QString::number(mIPAttemptCount));
    static const QString us = "https://proxy.sh/ip.php";
    mIPReply.reset(AuthManager::instance()->mNAM.get(BuildRequest(QUrl(us))));
    connect(mIPReply.get(), &QNetworkReply::finished,
            this, &AuthManager::processOldIP);
}

void AuthManager::processUpdatesXml()
{
    if (mUpdateReply->error() != QNetworkReply::NoError) {
        Log::logt(mUpdateReply->errorString());
        return;
    }
    QByteArray ba = mUpdateReply->readAll();

    /*
    QByteArray ba =
    "<?xml version=\"1.1\" encoding=\"UTF-8\"?>"
    "<version>"
      "<stable>3.0</stable>"
      "<build>24</build>"
      "<files>"
            "<file url=\"/celovpn.exe\"/>"
      "</files>"
      "<date>2015-08-05</date>"
    "</version>";
    */
    if (ba.isEmpty()) {
        Log::logt("Cannot get Updates version info. Server response from " UPDATE_URL " is empty.");
        return;
    }
    // parse XML response

    // <?xml version="1.1" encoding="UTF-8"?>
    // <version>
    //  <stable>3.0</stable>
    //  <build>23</build>
    //  <files>
    //    <file url="/celovpn.exe"/>
    //  </files>
    //  <date>2015-08-05</date>
    // </version>
    QDomDocument doc;
    QString msg;
    if (!doc.setContent(QString(ba), &msg)) {
        Log::logt("Error parsing XML Updates info from url: " UPDATE_URL "\n" + msg);
        return;
    }
    QDomNodeList nl = doc.elementsByTagName("build");
    if (nl.size() <= 0) {
        Log::logt("Missing 'build' node");
        return;
    }
    QDomNode n = nl.item(0);
    QString ss = n.toElement().text();
    if (!ss.isEmpty()) {
        bool ok;
        int upd = ss.toInt(&ok);
        Log::logt(QString("Got updated xml, server version is %1, local version is %2").arg(upd).arg(BUILD_NUM));
        if (ok && BUILD_NUM < upd) {
            int result = WndManager::Instance()->Confirmation("New version " + ss + " available. Update?");
            Setting::instance()->updateMessageShown();
            if (result == QDialog::Accepted)
                launchUpdateUrl();           // TODO: -2 auto update self
        }
    }
}

QString AuthManager::processServersXml()
{
    QString message;
    int line, column;
    clearServerLists();
    if (mReply->error() != QNetworkReply::NoError) {
        mLoggedIn = false;
        return mReply->errorString();
    } else {
        QByteArray ba = mReply->readAll();
        if (ba.isEmpty()) {
            mLoggedIn = false;
            return "Cannot get server list xml. Server response is empty.";
        }

        // parse XML response
        QDomDocument doc;
        QString login, psw;
        if (!doc.setContent(QString(ba).trimmed(), &message, &line, &column)) {
            mLoggedIn = false;
            Log::logt(QString("Error parsing xml response, xml is: %1").arg(QString(ba)));
            return "Error parsing server XML response\n"
                   "message: " + message +"\n"
                   "line: " + QString::number(line) + "\n"
                   "column: " + QString::number(column) + "\n";
        }
        QDomNodeList nl = doc.documentElement().childNodes();
        if (nl.size() > 0) {
            for (int k = 0; k < nl.size(); ++k) {
                QDomNode se = nl.item(k);
                QString name = se.nodeName();
                QDomElement nameNode = se.firstChildElement("Name");
                QDomElement ipNode = se.firstChildElement("IP");
                if (nameNode.isNull() || name.isEmpty() || ipNode.isNull())
                    continue;
                AServer s2;
                s2.address = nameNode.text();
                s2.ip = ipNode.text();
                s2.name = name;
                s2.country = name;
                s2.handlesTcp = true;
                s2.handlesUdp = true;

                QDomElement config443 = se.firstChildElement("Config-443");
                QDomElement config53 = se.firstChildElement("Config-53");
                QDomElement config1194 = se.firstChildElement("Config-1194");
                s2.configUrls.insert(443, config443.text());
                s2.configUrls.insert(53, config53.text());
                s2.configUrls.insert(1194, config1194.text());

                mServers.append(s2);
                int serverId = mServers.size() - 1;
                mServerIds[ENCRYPTION_RSA].append(serverId);
                if (s2.handlesTcp)
                    mTcpServerIds.append(serverId);
                if (s2.handlesUdp)
                    mUdpServerIds.append(serverId);
            }
            mLoggedIn = true;
            emit serverListsLoaded();
        } else {
            mLoggedIn = false;
            return "Incorrect credentials. Make sure to use your VPN credentials and not your email.";
        }
    }

    if (!Setting::instance()->testing())
        pingAllServers(); // No need to ping servers when in testing mode

    return message;
}

void AuthManager::pingAllServers()
{
    Log::logt("pingAllServers called");
    // Fill mPings with as many -1 entries as there are servers
    while (mPings.size() < mServers.size())
        mPings.append(-1);
    for (int k = 0; k < mServers.size(); ++k)
        mToPing.push(k);

    if (mWorkers.empty()) {
        mInProgress.assign(PINGWORKERS_NUM, 0);
        LoginWindow * m = LoginWindow::Instance();
        for (size_t k = 0; k < PINGWORKERS_NUM; ++k) {
            mWorkers.push_back(NULL);
            mWaiters.push_back(new PingWaiter(k, m));
            mTimers.push_back(new QTimer());
        }
    }

    mPingsLoaded = false;
    for (size_t k = 0; k < mWorkers.size() && !mToPing.empty(); ++k)
        startWorker(k);
}

void AuthManager::startWorker(size_t id)
{
    Log::logt("startWorker called with id " + QString::number(id));
    if (!mToPing.empty()) {
        size_t srv = mToPing.front();
        mToPing.pop();
        Log::logt("startWorker will ping server number " + QString::number(srv) + " with address " + mServers.at(srv).address);

        mInProgress.at(id) = srv;
        LoginWindow * m = LoginWindow::Instance();

        if (mWorkers.at(id) !=NULL) {
            Log::logt("Workers at " + QString::number(id) + " not null, so disconnecting and terminating");
            disconnect(mWorkers.at(id), static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                       mWaiters.at(id), &PingWaiter::PingFinished);
            disconnect(mWorkers.at(id), &QProcess::errorOccurred,
                       mWaiters.at(id), &PingWaiter::PingError);
            if (mWorkers.at(id)->state() != QProcess::NotRunning) {
                mWorkers.at(id)->terminate();
                mWorkers.at(id)->deleteLater();
            }
        }
        mWorkers[id] = new QProcess(m);
        connect(mWorkers.at(id), static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                mWaiters.at(id), &PingWaiter::PingFinished);
        connect(mWorkers.at(id), &QProcess::errorOccurred,
                mWaiters.at(id), &PingWaiter::PingError);
        startPing(*mWorkers.at(id), mServers.at(srv).address);
        connect(mTimers.at(id), &QTimer::timeout,
                mWaiters.at(id), &PingWaiter::Timer_Terminate);
        mTimers.at(id)->setSingleShot(true);
        mTimers.at(id)->start(PINGWORKER_MAX_TIMEOUT);
    } else {
        if (!mPingsLoaded) {
            mPingsLoaded = true;
        }
    }
}

void AuthManager::pingComplete(size_t idWaiter)
{
    Log::logt("pingComplete called with id " + QString::number(idWaiter));
    mTimers.at(idWaiter)->stop();
    int p = extractPing(*mWorkers.at(idWaiter));
    Log::logt(mServers.at(mInProgress.at(idWaiter)).address + " Got ping " + QString::number(p));
    mPings[mInProgress.at(idWaiter)] = p;
    startWorker(idWaiter);
}

void AuthManager::pingError(size_t idWaiter)
{
    Log::logt("pingError called with id " + QString::number(idWaiter));
    mTimers.at(idWaiter)->stop();
    int p = extractPing(*mWorkers.at(idWaiter));
//      Log::logt(_servers.at(_inprogress.at(idWaiter)).address + " ping process error, extracted ping: " + QString::number(p));
    mPings[mInProgress.at(idWaiter)] = p;
    startWorker(idWaiter);
}

void AuthManager::pingTerminated(size_t idWaiter)
{
    Log::logt("pingTerminated called with id " + QString::number(idWaiter));
    mWorkers.at(idWaiter)->terminate();
    int p = extractPing(*mWorkers.at(idWaiter));
//      Log::logt(_servers.at(_inprogress.at(idWaiter)).address + " ping process terminated, extracted ping: " + QString::number(p));
    mPings[mInProgress.at(idWaiter)] = p;
    startWorker(idWaiter);
}

void AuthManager::seed()
{
    if (!mSeeded) {
        srand(time(NULL));
        mSeeded = true;
    }
}

std::vector<int> AuthManager::getPings(const std::vector<size_t> & toping)
{
    std::vector<int> v;
    v.assign(toping.size(), -1);
    if (mPings.empty())
        Log::logt("GetPings(): Empty pings collection");
    else {
        for (size_t k = 0; k < toping.size(); ++k) {
            if (toping.at(k) >= (size_t)mPings.size())
                Log::logt("GetPings(): Server id greater than size of pings coll");
            else
                v.at(k) = mPings.at(toping.at(k));
        }
    }
    return v;
}

typedef std::pair<int, size_t> IUPair;
typedef std::vector<IUPair> IUVec;
static bool PCmp(const IUPair & a, const IUPair & b)
{
    return a.first < b.first;
}

int AuthManager::getServerToJump()
{
    Log::logt("getServerToJump called");
    if (mServers.isEmpty()) {
        Log::logt("Server list not loaded, so using -1");
        return -1;
    }
    int srv = -1;
    int prev = Setting::instance()->serverID();
    Log::logt("Previous server is " + QString::number(prev));
    std::vector<size_t> toping;     // ix inside mServers
    int enc = Setting::instance()->encryption();
    // jump to server
    for (int k = 0; k < mServerIds[enc].size(); ++k) {
        if (mServerIds[enc].at(k) != prev)
            toping.push_back(mServerIds[enc].at(k));
    }
    Log::logt("getServerToJump pings list is " + QString::number(toping.size()));

    std::vector<int> pings = getPings(toping);      // from cache; do not wait for pings; return vec of the same size

    IUVec ping_ix;
    for (size_t k = 0; k < toping.size(); ++k) {
        if (pings.at(k) > -1)
            ping_ix.push_back(IUPair(pings.at(k), toping.at(k)));
    }

    if (!ping_ix.empty()) {
        std::sort(ping_ix.begin(), ping_ix.end(), PCmp);
        unsigned int num = Setting::instance()->showNodes() ? 20 : 6;      // pick this many from the top
        if (num >= ping_ix.size())
            num = ping_ix.size();
        int offset = rand() % num;
        srv = ping_ix.at(offset).second;
    }

    // pings can be unavailable
    if (srv < 0) {
        // just pick random
        if (!toping.empty()) {
            srv = toping.at(rand() % toping.size());
        } else {
            if (!mServers.isEmpty())
                srv = rand() % mServers.size();
        }
    }
//Log::logt("SrvToJump() returns " + QString::number(srv));
    return srv;
}

void AuthManager::jump()
{
    // TODO: -2 update lists
    int srv = getServerToJump();              // except current srv/hub
    if (srv > -1) {
// TODO: -0             SetNewIp("");
        Setting::instance()->setServer(srv);
        VPNServiceManager::instance()->sendConnectToVPNRequest();               // contains stop
    }
}

uint64_t AuthManager::getRandom64()
{
    seed();
    uint64_t v = 0
                 | ((uint64_t)rand() << 49)
                 | ((uint64_t)rand() << 34)
                 | ((uint64_t)rand() << 19)
                 | ((uint64_t)rand() & 0xf);
    return v;
}

void AuthManager::processOldIP()
{
    Log::logt("ProcessOldIpHttp() attempt " + QString::number(mIPAttemptCount));
    QString ip;
    bool err = true;
    if (mIPReply->error() != QNetworkReply::NoError) {
        Log::logt(mIPReply->errorString());
    } else {
        QByteArray ba = mIPReply->readAll();
        if (ba.isEmpty()) {
            Log::logt("Cannot get old IP address. Server response is empty.");
        } else {
            QString s(ba);
            int p[3];
            int t = 0;
            bool ok = true;
            for (size_t k = 0; k < 3; ++k) {
                p[k] = s.indexOf('.', t);
                if (p[k] < 0) {
                    ok = false;
                    break;
                }
                t = p[k] + 1;
            }
            if (ok) {
                if (s.length() >= QString("2.2.2.2").length()
                        && s.length() <= QString("123.123.123.123").length()) {
                    ip = s;
                    err = false;
                }
            }
        }
    }

    if (err) {
        Log::logt("ProcessOldIpHttp() attempt " + QString::number(mIPAttemptCount) + " fails");
        if (mIPAttemptCount < 4)
            getOldIP();
        else
            Log::logt("ProcessOldIpHttp() conceide at attempt " + QString::number(mIPAttemptCount));
    } else {
        Log::logt("Determined old IP:  " + ip);
        mOldIP = ip;
        // try to push value (if Scr_Connect was constructed yet)
        emit oldIpLoaded(mOldIP);
    }
}

void AuthManager::forwardPorts()
{
//    UVec ports = Setting::instance()->forwardPorts();
//    if (!ports.empty()) {
//        mPortForwarderThread.reset(new PortForwarder(ports, mNAM, mAccountLogin, mAccountPassword));
//        mPortForwarderThread->StartFirst();
//        for (size_t k = 0, sz = ports.size(); k < sz; ++k) {

//            ;
//        }
//    }
}

void AuthManager::loginFinished()
{
    QString message = processServersXml();
    Log::logt("loginFinished called message is " + message);
    if (message.isEmpty())
        emit loginCompleted();
    else
        emit loginError(message);
}

void AuthManager::startPing(QProcess & pr, const QString & adr)
{
    pr.start(pingCommand(), formatArguments(adr));
}

int AuthManager::extractPing(QProcess & pr)
{
    int ping = -1;
    QByteArray ba = pr.readAllStandardOutput();
    QString s(ba);
    QStringList out = s.split("\n", QString::SkipEmptyParts);
    if (!out.isEmpty()) {
        const QString & sp = out.at(out.size() - 1).trimmed();	// last line
#ifndef Q_OS_WIN
        if (sp.indexOf("min/avg/max") > -1) {
            int e = sp.indexOf('=');
            int slash = sp.indexOf('/', e +1);
            int sl1 = sp.indexOf('/', slash +1);
            if (sl1 > -1) {
                QString sv = sp.mid(slash + 1, sl1 - slash - 1);
                bool ok;
                double d = sv.toDouble(&ok);
                if (ok)
                    ping = (int)d;
            }
        }
#else
        int a;
        if ((a = sp.indexOf("Average =")) > -1) {
            int e = sp.indexOf('=', a);
            if (e > -1) {
                QString val = sp.mid(e + 1, sp.length() - (e + 1 + 2));
                bool ok;
                int p = val.toInt(&ok);
                if (ok)
                    ping = p;
            }

        }
#endif
    }
    return ping;
}

QStringList AuthManager::formatArguments(const QString & adr)
{
    QStringList args;
    args
#ifndef Q_OS_WIN
            << "-c" << "1"		// one packet - Mac, Linux
#ifdef Q_OS_LINUX
            << "-w" << "1"		// 1s deadline - Linux
#endif
#ifdef Q_OS_DARWIN
            << "-t" << "1"		// 1s timeout - Mac
#endif
#else
            << "-n" << "1"		// one packet - Windows
            << "-w"	<< "1200"	// timeout in ms
#endif
            << adr
            ;
    return args;
}

const QString & AuthManager::pingCommand()
{
#ifdef  Q_OS_DARWIN
    static const QString cmd = "/sbin/ping";
#else
#ifdef  Q_OS_LINUX
    static const QString cmd = "/bin/ping";
#else
    static const QString cmd = "ping";
#endif
#endif
    return cmd;
}

