#pragma once

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

struct FileEntryData {
    QString path;
    int occurrences;
};

struct ProfileData {
    QList<FileEntryData> files;
    int segment;
    int newVersionCode;
};

class SaveSystem {
public:
    SaveSystem();

    void        save(const QString &name, const ProfileData &data);
    ProfileData load(const QString &name) const;
    void        rename(const QString &oldName, const QString &newName);
    bool        exists(const QString &name) const;
    QStringList profileNames() const;

private:
    QMap<QString, ProfileData> profiles_;
    QString                    filePath_;

    void readFromDisk();
    void writeToDisk();
};
