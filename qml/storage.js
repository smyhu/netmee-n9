// Bookmarks in the QML offline storage (SQLite).
function db() {
    var d = openDatabaseSync("netmee", "1.0", "NetMee", 200000);
    d.transaction(function(tx) {
        tx.executeSql("CREATE TABLE IF NOT EXISTS bookmarks (id INTEGER PRIMARY KEY, title TEXT, url TEXT, added INTEGER)");
        tx.executeSql("CREATE TABLE IF NOT EXISTS meta (k TEXT PRIMARY KEY, v TEXT)");
        var r = tx.executeSql("SELECT v FROM meta WHERE k = 'seeded'");
        if (r.rows.length == 0) {
            var first = [["DuckDuckGo Lite", "https://lite.duckduckgo.com/lite/"],
                         ["FrogFind", "http://frogfind.com/"],
                         ["68k.news", "http://68k.news/"],
                         ["wiby.me", "http://wiby.me/"],
                         ["Wikipedia", "https://en.m.wikipedia.org/"],
                         ["CNN Lite", "http://lite.cnn.com/"],
                         ["NPR Text", "https://text.npr.org/"]];
            var now = Math.round(new Date().getTime() / 1000);
            for (var i = 0; i < first.length; ++i)
                tx.executeSql("INSERT INTO bookmarks (title, url, added) VALUES (?, ?, ?)", [first[i][0], first[i][1], now - i]);
            tx.executeSql("INSERT INTO meta VALUES ('seeded', '1')");
        }
    });
    return d;
}

function list(filter) {
    var out = [];
    db().readTransaction(function(tx) {
        var r = tx.executeSql("SELECT id, title, url FROM bookmarks ORDER BY added DESC");
        var f = (filter || "").toLowerCase();
        for (var i = 0; i < r.rows.length; ++i) {
            var b = r.rows.item(i);
            if (f == "" || b.title.toLowerCase().indexOf(f) >= 0 || b.url.toLowerCase().indexOf(f) >= 0)
                out.push({ "bid": b.id, "title": b.title, "url": b.url });
        }
    });
    return out;
}

function find(url) {
    var id = -1;
    db().readTransaction(function(tx) {
        var r = tx.executeSql("SELECT id FROM bookmarks WHERE url = ?", [url]);
        if (r.rows.length > 0) id = r.rows.item(0).id;
    });
    return id;
}

function add(title, url) {
    db().transaction(function(tx) {
        tx.executeSql("INSERT INTO bookmarks (title, url, added) VALUES (?, ?, ?)",
                      [title || url, url, Math.round(new Date().getTime() / 1000)]);
    });
}

function update(id, title, url) {
    db().transaction(function(tx) {
        tx.executeSql("UPDATE bookmarks SET title = ?, url = ? WHERE id = ?", [title, url, id]);
    });
}

function remove(id) {
    db().transaction(function(tx) { tx.executeSql("DELETE FROM bookmarks WHERE id = ?", [id]); });
}
