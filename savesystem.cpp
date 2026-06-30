#include "savesystem.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

static QString configPath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + "/profiles.json";
}

static QJsonObject toJson(const ProfileData &d) {
    QJsonArray files;
    for (const auto &f : d.files) {
        QJsonObject o;
        o["path"] = f.path;
        o["occurrences"] = f.occurrences;
        files.append(o);
    }
    QJsonObject root;
    root["files"] = files;
    root["segment"] = d.segment;
    root["newVersionCode"] = d.newVersionCode;
    return root;
}

static ProfileData fromJson(const QJsonObject &o) {
    ProfileData d;
    d.segment = o["segment"].toInt(1);
    d.newVersionCode = o["newVersionCode"].toInt(0);
    for (const auto &v : o["files"].toArray()) {
        QJsonObject f = v.toObject();
        d.files.append({f["path"].toString(), f["occurrences"].toInt(1)});
    }
    return d;
}

SaveSystem::SaveSystem() {
    filePath_ = configPath();
    readFromDisk();
}

void SaveSystem::save(const QString &name, const ProfileData &data) {
    profiles_[name] = data;
    writeToDisk();
}

ProfileData SaveSystem::load(const QString &name) const {
    return profiles_.value(name);
}

void SaveSystem::rename(const QString &oldName, const QString &newName) {
    if (!profiles_.contains(oldName) || oldName == newName) return;
    profiles_[newName] = profiles_.take(oldName);
    writeToDisk();
}

bool SaveSystem::exists(const QString &name) const {
    return profiles_.contains(name);
}

QStringList SaveSystem::profileNames() const {
    return profiles_.keys();
}

void SaveSystem::readFromDisk() {
    QFile f(filePath_);
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return;
    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it)
        if (it.value().isObject())
            profiles_[it.key()] = fromJson(it.value().toObject());
}

void SaveSystem::writeToDisk() {
    QJsonObject root;
    for (auto it = profiles_.cbegin(); it != profiles_.cend(); ++it)
        root[it.key()] = toJson(it.value());
    QFile f(filePath_);
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(root).toJson());
}
