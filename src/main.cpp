/*
 * NetMee for Nokia N9 (Harmattan) - a light web browser.
 * Copyright (c) 2026 smyhu. MIT License (see LICENSE).
 * Designed and written by Claude (Anthropic) together with smyhu.
 */
#include <QtGui/QApplication>
#include <QtCore/QLocale>
#include <QtCore/QTime>
#include <stdio.h>
#include <stdlib.h>
#include <QtDeclarative/QDeclarativeView>
#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/qdeclarative.h>
#include <QtNetwork/QNetworkProxy>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <execinfo.h>
#include "webitem.h"
#include "opener.h"

// Crash diary: ~/.netmee-n9/netmee.log keeps Qt warnings, visited pages and,
// after a crash, the signal and a raw backtrace. The previous run's log is
// kept as netmee.log.1, so a crash can be sent in after restarting.
static char g_log[512];

static void logLine(const char *s, bool always = false)
{
    int fd = open(g_log, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) return;
    if (!always && lseek(fd, 0, SEEK_END) > 200000) { close(fd); return; }
    ssize_t r = write(fd, s, strlen(s));
    r = write(fd, "\n", 1);
    (void)r;
    close(fd);
}

static void onMessage(QtMsgType type, const char *msg)
{
    static const char *tag[] = { "D ", "W ", "C ", "F " };
    QByteArray line = QByteArray(tag[type < 4 ? type : 1]) + QTime::currentTime().toString("hh:mm:ss ").toLatin1() + msg;
    logLine(line.constData());
    fprintf(stderr, "%s\n", msg);
    if (type == QtFatalMsg) abort();
}

static void onCrash(int sig)
{
    char head[64];
    snprintf(head, sizeof head, "CRASH signal %d", sig);
    logLine(head, true);
    void *frames[40];
    int n = backtrace(frames, 40);
    int fd = open(g_log, O_WRONLY | O_APPEND);
    if (fd >= 0) { backtrace_symbols_fd(frames, n, fd); close(fd); }
    signal(sig, SIG_DFL);
    raise(sig);
}

static void startDiary()
{
    QString dir = QDir::homePath() + "/.netmee-n9";
    QDir().mkpath(dir);
    QString path = dir + "/netmee.log";
    QFile::remove(path + ".1");
    QFile::rename(path, path + ".1");
    qstrncpy(g_log, QFile::encodeName(path).constData(), sizeof g_log);
    qInstallMsgHandler(onMessage);
    signal(SIGSEGV, onCrash);
    signal(SIGABRT, onCrash);
    signal(SIGBUS, onCrash);
    signal(SIGILL, onCrash);
    signal(SIGFPE, onCrash);
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("netmee-n9");
    app.setOrganizationName("netmee-n9");
    // optional proxy for testing: NETMEE_PROXY=host:port
    QByteArray px = qgetenv("NETMEE_PROXY");
    int colon = px.lastIndexOf(':');
    if (colon > 0)
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy, QString::fromLatin1(px.left(colon)),
                                                         QString::fromLatin1(px.mid(colon + 1)).toUShort()));
    // an address to open: "netmee-n9 http://..." (from FeedMee or another app)
    QString startUrl;
    QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i)
        if (args.at(i).startsWith("http://") || args.at(i).startsWith("https://"))
            startUrl = args.at(i);
    qmlRegisterType<WebItem>("NetMee", 1, 0, "WebItem");
    QString base = "/opt/netmee-n9/qml/";
    QByteArray dir = qgetenv("NETMEE_QML");
    if (!dir.isEmpty()) base = QString::fromLocal8Bit(dir);
    QString lang = QLocale::system().name().left(2);
    QByteArray l = qgetenv("NETMEE_LANG");
    if (!l.isEmpty()) lang = QString::fromLatin1(l);

    QDeclarativeView view;
    // D-Bus object first, the service name second: an openUrl call that
    // started NetMee (tap on an Events item) must find the object
    Opener opener(&view);
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (bus.isConnected()) {
        bus.registerObject("/", &opener, QDBusConnection::ExportAllSlots);
        // one copy only: if NetMee already runs, hand it the address and quit
        if (!bus.registerService(NETMEE_SERVICE)) {
            QDBusInterface running(NETMEE_SERVICE, "/", NETMEE_SERVICE, bus);
            if (startUrl.isEmpty())
                running.call("top");
            else
                running.call("openUrl", startUrl);
            return 0;
        }
    } else {
        Opener::note("no session bus");
    }
    Opener::note("started" + (startUrl.isEmpty() ? QString() : " with " + startUrl));
    startDiary();
    qDebug("NetMee %s started%s", NETMEE_VERSION, startUrl.isEmpty() ? "" : qPrintable(" with " + startUrl));
    view.rootContext()->setContextProperty("appVersion", QString(NETMEE_VERSION));
    view.rootContext()->setContextProperty("uiLang", lang);
    view.rootContext()->setContextProperty("startUrl", startUrl);
    QObject::connect(view.engine(), SIGNAL(quit()), &app, SLOT(quit()));
    view.setSource(QUrl::fromLocalFile(base + "main.qml"));
    view.showFullScreen();
    opener.setReady();
    return app.exec();
}
