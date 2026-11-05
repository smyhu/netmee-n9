/*
 * NetMee for Nokia N9 - web view item for QML.
 * Copyright (c) 2026 smyhu. MIT License (see LICENSE).
 * Designed and written by Claude (Anthropic) together with smyhu.
 */
#ifndef NETMEE_WEBITEM_H
#define NETMEE_WEBITEM_H

#include <QtDeclarative/QDeclarativeItem>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtWebKit/QWebPage>
#include <QtCore/QSet>
#include <QtCore/QStringList>

class QGraphicsWebView;

// Answers a blocked request at once with an error and no data.
class BlockedReply : public QNetworkReply
{
    Q_OBJECT
public:
    BlockedReply(const QNetworkRequest &req, QNetworkAccessManager::Operation op, QObject *parent);
    void abort() {}
    qint64 bytesAvailable() const { return 0; }
protected:
    qint64 readData(char *, qint64) { return -1; }
private slots:
    void fire();
};

// Looks at every request before it leaves the phone: ad and tracker
// servers from the block list are refused, scripts from other sites can be
// refused, and a script bigger than the limit is cut off as soon as its
// size is known (Content-Length), before WebKit starts to run it.
class FilterNam : public QNetworkAccessManager
{
    Q_OBJECT
public:
    FilterNam(QObject *parent);
    QStringList blockList;
    bool blockThirdPartyScripts;
    int maxScriptKB;
    QString pageHost;
    int blocked;
signals:
    void blockedChanged();
protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *data);
private slots:
    void checkSize();
private:
    bool listed(const QString &host) const;
    void count();
};

// Page with a script watchdog and a phone user agent.
class NetPage : public QWebPage
{
    Q_OBJECT
public:
    NetPage(QObject *parent);
    QSet<QString> jsOff;      // hosts with JavaScript switched off
    bool jsDefault;
    QString agent;
    int interrupted;
signals:
    void scriptStopped();
protected:
    bool shouldInterruptJavaScript();
    QString userAgentForUrl(const QUrl &) const { return agent; }
    bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &req, NavigationType type);
    void javaScriptAlert(QWebFrame *, const QString &) {}
    bool javaScriptConfirm(QWebFrame *, const QString &) { return false; }
    bool javaScriptPrompt(QWebFrame *, const QString &, const QString &, QString *) { return false; }
    QWebPage *createWindow(WebWindowType);
};

class WebItem : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(QString url READ url NOTIFY urlChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY historyChanged)
    Q_PROPERTY(qreal contentWidth READ contentWidth NOTIFY contentSizeChanged)
    Q_PROPERTY(qreal contentHeight READ contentHeight NOTIFY contentSizeChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(int viewportWidth READ viewportWidth WRITE setViewportWidth)
    Q_PROPERTY(bool jsEnabled READ jsEnabled NOTIFY jsChanged)
    Q_PROPERTY(int blocked READ blocked NOTIFY blockedChanged)
    Q_PROPERTY(int stoppedScripts READ stoppedScripts NOTIFY stoppedScriptsChanged)
    Q_PROPERTY(bool blockThirdPartyScripts READ blockThirdPartyScripts WRITE setBlockThirdPartyScripts NOTIFY filterChanged)
    Q_PROPERTY(int maxScriptKB READ maxScriptKB WRITE setMaxScriptKB NOTIFY filterChanged)
    Q_PROPERTY(bool jsDefault READ jsDefault WRITE setJsDefault NOTIFY filterChanged)
    Q_PROPERTY(int pageStyle READ pageStyle WRITE setPageStyle NOTIFY pageStyleChanged)
public:
    WebItem(QDeclarativeItem *parent = 0);
    QString url() const;
    QString title() const;
    int progress() const { return m_progress; }
    bool loading() const { return m_loading; }
    bool canGoBack() const;
    qreal contentWidth() const;
    qreal contentHeight() const;
    qreal zoom() const;
    void setZoom(qreal z);
    int viewportWidth() const { return m_vpWidth; }
    void setViewportWidth(int w);
    bool jsEnabled() const;
    int blocked() const { return m_nam->blocked; }
    int stoppedScripts() const { return m_page->interrupted; }
    bool blockThirdPartyScripts() const { return m_nam->blockThirdPartyScripts; }
    void setBlockThirdPartyScripts(bool b);
    int maxScriptKB() const { return m_nam->maxScriptKB; }
    void setMaxScriptKB(int kb);
    bool jsDefault() const { return m_page->jsDefault; }
    void setJsDefault(bool b);
    int pageStyle() const { return m_style; }
    void setPageStyle(int s);

    Q_INVOKABLE void load(const QString &url);
    Q_INVOKABLE void back();
    Q_INVOKABLE void reload();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void showMessage(const QString &html);
    Q_INVOKABLE bool jsAllowed(const QString &host) const;
    Q_INVOKABLE void setJsAllowed(const QString &host, bool on);
    Q_INVOKABLE QString hostOf(const QString &url) const;
    Q_INVOKABLE void setText(const QString &key, const QString &value);
    Q_INVOKABLE QString text(const QString &key, const QString &def) const;

signals:
    void urlChanged();
    void titleChanged();
    void progressChanged();
    void loadingChanged();
    void historyChanged();
    void contentSizeChanged();
    void zoomChanged();
    void jsChanged();
    void blockedChanged();
    void stoppedScriptsChanged();
    void filterChanged();
    void pageStyleChanged();
    void loadFailed(const QString &url);
    void linkOpened();

private slots:
    void onStarted();
    void onProgress(int p);
    void onFinished(bool ok);
    void onGeometry();
    void onUrl();

protected:
    void geometryChanged(const QRectF &n, const QRectF &o);

private:
    void applyLayout();
    void saveJs();
    QGraphicsWebView *m_view;
    NetPage *m_page;
    FilterNam *m_nam;
    int m_progress;
    bool m_loading;
    int m_vpWidth;
    int m_style;
    QString m_requested;
};

#endif
