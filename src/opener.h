/*
 * NetMee for Nokia N9 - D-Bus entry point: other apps (e.g. FeedMee on the
 * Events screen) open a page with com.github.smyhu.netmee / openUrl(url).
 * Copyright (c) 2026 smyhu. MIT License (see LICENSE).
 * Designed and written by Claude (Anthropic) together with smyhu.
 */
#ifndef OPENER_H
#define OPENER_H

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtDeclarative/QDeclarativeView>
#include <QtDeclarative/QDeclarativeItem>

#define NETMEE_SERVICE "com.github.smyhu.netmee"

class Opener : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.github.smyhu.netmee")
public:
    explicit Opener(QDeclarativeView *view) : QObject(view), m_view(view), m_ready(false) {}

    // short log for "the article does not open" reports: ~/.netmee-n9/dbus.log
    static void note(const QString &text)
    {
        QString dir = QDir::homePath() + "/.netmee-n9";
        QDir().mkpath(dir);
        QFile f(dir + "/dbus.log");
        if (f.size() > 20000)
            f.remove();
        if (f.open(QIODevice::Append | QIODevice::Text))
            f.write((QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ") + text + "\n").toUtf8());
    }

    // the QML page is loaded: open what came in while it was starting
    void setReady()
    {
        m_ready = true;
        if (!m_pending.isEmpty()) {
            QString url = m_pending;
            m_pending.clear();
            openUrl(url);
        }
    }

public slots:
    void openUrl(const QString &url)
    {
        note("openUrl " + url + (m_ready ? "" : " (waiting for the page)"));
        if (!m_ready) {
            m_pending = url;
            return;
        }
        QObject *root = m_view->rootObject();
        if (root && !url.isEmpty())
            QMetaObject::invokeMethod(root, "open", Q_ARG(QVariant, QVariant(url)));
        top();
    }
    void top()
    {
        m_view->showFullScreen();
        m_view->raise();
        m_view->activateWindow();
    }

private:
    QDeclarativeView *m_view;
    bool m_ready;
    QString m_pending;
};

#endif
