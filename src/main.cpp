/*
 * NetMee for Nokia N9 (Harmattan) - a light web browser.
 * Copyright (c) 2026 smyhu. MIT License (see LICENSE).
 * Designed and written by Claude (Anthropic) together with smyhu.
 */
#include <QtGui/QApplication>
#include <QtCore/QLocale>
#include <QtDeclarative/QDeclarativeView>
#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/qdeclarative.h>
#include <QtNetwork/QNetworkProxy>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include "webitem.h"
#include "opener.h"

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
    view.rootContext()->setContextProperty("appVersion", QString(NETMEE_VERSION));
    view.rootContext()->setContextProperty("uiLang", lang);
    view.rootContext()->setContextProperty("startUrl", startUrl);
    QObject::connect(view.engine(), SIGNAL(quit()), &app, SLOT(quit()));
    view.setSource(QUrl::fromLocalFile(base + "main.qml"));
    view.showFullScreen();
    opener.setReady();
    return app.exec();
}
