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
#include "webitem.h"

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
    qmlRegisterType<WebItem>("NetMee", 1, 0, "WebItem");
    QString base = "/opt/netmee-n9/qml/";
    QByteArray dir = qgetenv("NETMEE_QML");
    if (!dir.isEmpty()) base = QString::fromLocal8Bit(dir);
    QString lang = QLocale::system().name().left(2);
    QByteArray l = qgetenv("NETMEE_LANG");
    if (!l.isEmpty()) lang = QString::fromLatin1(l);

    QDeclarativeView view;
    view.rootContext()->setContextProperty("appVersion", QString(NETMEE_VERSION));
    view.rootContext()->setContextProperty("uiLang", lang);
    QObject::connect(view.engine(), SIGNAL(quit()), &app, SLOT(quit()));
    view.setSource(QUrl::fromLocalFile(base + "main.qml"));
    view.showFullScreen();
    return app.exec();
}
