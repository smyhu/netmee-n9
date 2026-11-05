/*
 * NetMee for Nokia N9 - web view item for QML.
 * Copyright (c) 2026 smyhu. MIT License (see LICENSE).
 * Designed and written by Claude (Anthropic) together with smyhu.
 */
#include "webitem.h"
#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtNetwork/QNetworkRequest>
#include <QtWebKit/QGraphicsWebView>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebHistory>
#include <QtWebKit/QWebSettings>

// Page looks: dark = white text on black, sepia = e-book reader colours.
static const char *kStyles[] = {
    "",
    "*{background-color:#000 !important;color:#e8e8e8 !important;border-color:#333 !important}"
    "a,a *{color:#cfe8a8 !important}a:visited,a:visited *{color:#a8b890 !important}"
    "img,video{opacity:.85}input,textarea,select,button{background-color:#111 !important;color:#fff !important}",
    "*{background-color:#f4ecd8 !important;color:#5b4636 !important;border-color:#d8ccb0 !important}"
    "a,a *{color:#8b4513 !important}a:visited,a:visited *{color:#6b4a30 !important}"
    "input,textarea,select,button{background-color:#fbf6ea !important;color:#3b2a1e !important}",
    0 };

static const char *kAgent =
    "Mozilla/5.0 (MeeGo; NokiaN9) AppleWebKit/534.13 (KHTML, like Gecko) NokiaBrowser/8.5.0 Mobile Safari/534.13 NetMee";

// Light sites that work without JavaScript: scripts start switched off there.
static const char *kLightSites[] = { "frogfind.com", "68k.news", "wiby.me", "wiby.org", "lite.cnn.com",
                                     "text.npr.org", "lite.duckduckgo.com", "html.duckduckgo.com", 0 };

// site part of a host name: news.bbc.co.uk -> bbc.co.uk, cdn.example.com -> example.com
static QString siteOf(const QString &host)
{
    QStringList p = host.toLower().split('.', QString::SkipEmptyParts);
    if (p.size() <= 2) return p.join(".");
    int keep = 2;
    QString second = p.at(p.size() - 2);
    if (p.last().size() == 2 && (second == "co" || second == "com" || second == "org" || second == "net"
                                 || second == "gov" || second == "ac" || second == "edu"))
        keep = 3;
    return QStringList(p.mid(p.size() - keep)).join(".");
}

// ---------------------------------------------------------------- BlockedReply

BlockedReply::BlockedReply(const QNetworkRequest &req, QNetworkAccessManager::Operation op, QObject *parent)
    : QNetworkReply(parent)
{
    setRequest(req);
    setUrl(req.url());
    setOperation(op);
    open(QIODevice::ReadOnly);
    QTimer::singleShot(0, this, SLOT(fire()));
}

void BlockedReply::fire()
{
    setError(QNetworkReply::ContentAccessDenied, "blocked by NetMee");
    emit error(QNetworkReply::ContentAccessDenied);
    emit finished();
}

// ---------------------------------------------------------------- FilterNam

FilterNam::FilterNam(QObject *parent)
    : QNetworkAccessManager(parent), blockThirdPartyScripts(true), maxScriptKB(200), blocked(0)
{
    QStringList files;
    files << "/opt/netmee-n9/blocklist.txt";
    QByteArray env = qgetenv("NETMEE_BLOCKLIST");
    if (!env.isEmpty()) files.prepend(QString::fromLocal8Bit(env));
    for (int i = 0; i < files.size(); ++i) {
        QFile f(files.at(i));
        if (!f.open(QIODevice::ReadOnly)) continue;
        QTextStream in(&f);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed().toLower();
            if (!line.isEmpty() && !line.startsWith('#')) blockList << line;
        }
        break;
    }
}

bool FilterNam::listed(const QString &host) const
{
    for (int i = 0; i < blockList.size(); ++i) {
        const QString &b = blockList.at(i);
        if (host == b || host.endsWith("." + b)) return true;
    }
    return false;
}

void FilterNam::count()
{
    blocked++;
    emit blockedChanged();
}

QNetworkReply *FilterNam::createRequest(Operation op, const QNetworkRequest &req, QIODevice *data)
{
    QUrl u = req.url();
    QString host = u.host().toLower();
    bool script = u.path().toLower().endsWith(".js");
    if (!host.isEmpty() && host != pageHost && listed(host)) {
        count();
        return new BlockedReply(req, op, this);
    }
    if (script && blockThirdPartyScripts && !pageHost.isEmpty() && siteOf(host) != siteOf(pageHost)) {
        count();
        return new BlockedReply(req, op, this);
    }
    QNetworkReply *r = QNetworkAccessManager::createRequest(op, req, data);
    if (maxScriptKB > 0 && (u.scheme() == "http" || u.scheme() == "https")) {
        r->setProperty("netmeeScript", script);
        connect(r, SIGNAL(metaDataChanged()), this, SLOT(checkSize()));
        connect(r, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(checkSize()));
    }
    return r;
}

// cut off a script as soon as we know it is too big
void FilterNam::checkSize()
{
    QNetworkReply *r = qobject_cast<QNetworkReply *>(sender());
    if (!r || r->property("netmeeCut").toBool()) return;
    QString type = r->header(QNetworkRequest::ContentTypeHeader).toString().toLower();
    bool script = r->property("netmeeScript").toBool() || type.contains("javascript") || type.contains("ecmascript");
    if (!script) {
        disconnect(r, 0, this, 0);
        return;
    }
    qint64 limit = qint64(maxScriptKB) * 1024;
    qint64 size = r->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    if (size > limit || r->bytesAvailable() > limit) {
        r->setProperty("netmeeCut", true);
        count();
        r->abort();
    }
}

// ---------------------------------------------------------------- NetPage

NetPage::NetPage(QObject *parent) : QWebPage(parent), jsDefault(true), interrupted(0)
{
    agent = QString::fromLatin1(kAgent);
}

bool NetPage::shouldInterruptJavaScript()
{
    // a script ran too long: stop it instead of asking, so the app never hangs
    interrupted++;
    emit scriptStopped();
    return true;
}

bool NetPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &req, NavigationType)
{
    if (frame == mainFrame() || frame == 0) {
        QString host = req.url().host().toLower();
        WebItem *item = qobject_cast<WebItem *>(parent());
        bool on = item ? item->jsAllowed(host) : jsDefault;
        settings()->setAttribute(QWebSettings::JavascriptEnabled, on);
        FilterNam *nam = qobject_cast<FilterNam *>(networkAccessManager());
        if (nam) nam->pageHost = host;
    }
    return true;
}

// target="_blank" and window.open: open in the same view
QWebPage *NetPage::createWindow(WebWindowType)
{
    return this;
}

// ---------------------------------------------------------------- WebItem

WebItem::WebItem(QDeclarativeItem *parent)
    : QDeclarativeItem(parent), m_progress(0), m_loading(false), m_vpWidth(320), m_style(0)
{
    setFlag(QGraphicsItem::ItemHasNoContents, true);
    m_view = new QGraphicsWebView(this);
    m_page = new NetPage(this);
    m_nam = new FilterNam(m_page);
    m_page->setNetworkAccessManager(m_nam);
    m_view->setPage(m_page);
    m_view->setResizesToContents(true);
    QWebSettings *s = m_page->settings();
    s->setAttribute(QWebSettings::PluginsEnabled, false);
    s->setAttribute(QWebSettings::JavascriptCanOpenWindows, false);
    s->setAttribute(QWebSettings::FrameFlatteningEnabled, true);
    s->setAttribute(QWebSettings::LocalStorageEnabled, false);
    s->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, false);
    s->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, false);
    QWebSettings::globalSettings()->setMaximumPagesInCache(0);
    QWebSettings::globalSettings()->setObjectCacheCapacities(0, 2 * 1024 * 1024, 4 * 1024 * 1024);
    m_page->mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    m_page->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

    QSettings st("netmee-n9", "netmee");
    m_nam->blockThirdPartyScripts = st.value("filter/thirdPartyScripts", true).toBool();
    m_nam->maxScriptKB = st.value("filter/maxScriptKB", 200).toInt();
    m_page->jsDefault = st.value("filter/jsDefault", true).toBool();
    setPageStyle(st.value("app/pageStyle", 0).toInt());

    connect(m_page, SIGNAL(loadStarted()), this, SLOT(onStarted()));
    connect(m_page, SIGNAL(loadProgress(int)), this, SLOT(onProgress(int)));
    connect(m_page, SIGNAL(loadFinished(bool)), this, SLOT(onFinished(bool)));
    connect(m_view, SIGNAL(geometryChanged()), this, SLOT(onGeometry()));
    connect(m_view, SIGNAL(urlChanged(QUrl)), this, SLOT(onUrl()));
    connect(m_view, SIGNAL(titleChanged(QString)), this, SIGNAL(titleChanged()));
    connect(m_page, SIGNAL(linkClicked(QUrl)), this, SIGNAL(linkOpened()));
    connect(m_nam, SIGNAL(blockedChanged()), this, SIGNAL(blockedChanged()));
    connect(m_page, SIGNAL(scriptStopped()), this, SIGNAL(stoppedScriptsChanged()));
    applyLayout();
}

QString WebItem::url() const
{
    QString u = m_view->url().toString();
    if (u.isEmpty() || u == "about:blank") return m_requested;
    return u;
}

QString WebItem::title() const { return m_view->title(); }
bool WebItem::canGoBack() const { return m_page->history()->canGoBack(); }
// the page is laid out at viewportWidth CSS pixels and drawn scaled by zoom
qreal WebItem::contentWidth() const { return m_view->size().width() * m_view->scale(); }
qreal WebItem::contentHeight() const { return m_view->size().height() * m_view->scale(); }
qreal WebItem::zoom() const { return m_view->scale(); }

void WebItem::setZoom(qreal z)
{
    if (z < 0.5) z = 0.5;
    if (z > 4.0) z = 4.0;
    if (qAbs(z - m_view->scale()) < 0.001) return;
    m_view->setScale(z);
    emit zoomChanged();
    emit contentSizeChanged();
}

void WebItem::setViewportWidth(int w)
{
    m_vpWidth = w;
    applyLayout();
}

// layout width in CSS pixels = viewportWidth, drawn at the zoom factor
void WebItem::applyLayout()
{
    // layout height only matters for pages shorter than the screen
    m_page->setPreferredContentsSize(QSize(m_vpWidth, m_vpWidth * 16 / 10));
}

void WebItem::geometryChanged(const QRectF &n, const QRectF &o)
{
    QDeclarativeItem::geometryChanged(n, o);
}

bool WebItem::jsEnabled() const
{
    return m_page->settings()->testAttribute(QWebSettings::JavascriptEnabled);
}

void WebItem::setBlockThirdPartyScripts(bool b)
{
    m_nam->blockThirdPartyScripts = b;
    QSettings("netmee-n9", "netmee").setValue("filter/thirdPartyScripts", b);
    emit filterChanged();
}

void WebItem::setMaxScriptKB(int kb)
{
    m_nam->maxScriptKB = kb;
    QSettings("netmee-n9", "netmee").setValue("filter/maxScriptKB", kb);
    emit filterChanged();
}

void WebItem::setJsDefault(bool b)
{
    m_page->jsDefault = b;
    QSettings("netmee-n9", "netmee").setValue("filter/jsDefault", b);
    emit filterChanged();
}

void WebItem::setPageStyle(int s)
{
    if (s < 0 || s > 2) s = 0;
    m_style = s;
    QByteArray css(kStyles[s]);
    m_page->settings()->setUserStyleSheetUrl(css.isEmpty() ? QUrl()
        : QUrl(QString::fromLatin1("data:text/css;charset=utf-8;base64,") + QString::fromLatin1(css.toBase64())));
    QSettings("netmee-n9", "netmee").setValue("app/pageStyle", s);
    emit pageStyleChanged();
}

bool WebItem::jsAllowed(const QString &host) const
{
    QString site = siteOf(host);
    QSettings st("netmee-n9", "netmee");
    QVariant v = st.value("js/" + site);
    if (v.isValid()) return v.toBool();
    for (int i = 0; kLightSites[i]; ++i)
        if (host == kLightSites[i] || site == kLightSites[i]) return false;
    return m_page->jsDefault;
}

void WebItem::setJsAllowed(const QString &host, bool on)
{
    QSettings("netmee-n9", "netmee").setValue("js/" + siteOf(host), on);
    m_page->settings()->setAttribute(QWebSettings::JavascriptEnabled, on);
    emit jsChanged();
}

QString WebItem::hostOf(const QString &url) const { return QUrl(url).host().toLower(); }

void WebItem::setText(const QString &key, const QString &value)
{
    QSettings("netmee-n9", "netmee").setValue("app/" + key, value);
}

QString WebItem::text(const QString &key, const QString &def) const
{
    return QSettings("netmee-n9", "netmee").value("app/" + key, def).toString();
}

void WebItem::load(const QString &url)
{
    m_requested = url;
    m_nam->blocked = 0;
    m_page->interrupted = 0;
    emit blockedChanged();
    emit stoppedScriptsChanged();
    m_view->load(QUrl::fromUserInput(url));
    emit urlChanged();
}

void WebItem::back() { m_page->triggerAction(QWebPage::Back); }
void WebItem::reload() { m_page->triggerAction(QWebPage::Reload); }
void WebItem::stop() { m_page->triggerAction(QWebPage::Stop); }

void WebItem::showMessage(const QString &html)
{
    m_page->mainFrame()->setHtml(html);
}

void WebItem::onStarted()
{
    m_loading = true;
    m_progress = 0;
    emit loadingChanged();
    emit progressChanged();
}

void WebItem::onProgress(int p)
{
    m_progress = p;
    emit progressChanged();
}

void WebItem::onFinished(bool ok)
{
    m_loading = false;
    m_progress = 100;
    emit loadingChanged();
    emit progressChanged();
    emit historyChanged();
    emit jsChanged();
    QString u = m_view->url().toString();
    if (!ok && (u.isEmpty() || u == "about:blank" || m_page->mainFrame()->toPlainText().trimmed().isEmpty()))
        emit loadFailed(m_requested);
}

void WebItem::onGeometry() { emit contentSizeChanged(); }

void WebItem::onUrl()
{
    QString u = m_view->url().toString();
    if (!u.isEmpty() && u != "about:blank") m_requested = u;
    emit urlChanged();
    emit historyChanged();
    emit jsChanged();
}
