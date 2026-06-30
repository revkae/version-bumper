#pragma once

#include "savesystem.h"

#include <QMainWindow>

class QComboBox;
class QTableWidget;
class QSpinBox;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onNewProfile();
    void onRenameProfile();
    void onSaveProfile();
    void onProfileChanged(const QString &name);
    void onAddFile();
    void onRemove();
    void onApply();
    void refreshVersionPreview();

private:
    QComboBox    *profileCombo;
    QTableWidget *fileTable;
    QSpinBox     *segmentSpin;
    QSpinBox     *newCodeSpin;
    QLabel       *currentVersionLabel;
    QLabel       *newVersionLabel;
    QLabel       *currentCodeLabel;

    QList<FileEntryData> entries_;
    SaveSystem           saveSystem_;

    void loadProfile(const ProfileData &data);
    void addFileRow(const QString &path, int occurrences);

    QString detectVersion(const QString &path) const;
    int     detectVersionCode(const QString &path) const;
};
