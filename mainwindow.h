#pragma once

#include <QMainWindow>
#include <QList>
#include <QString>

class QTableWidget;
class QSpinBox;
class QLabel;

struct FileEntry {
    QString path;
    int occurrences; // how many version-name occurrences to replace
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onAddFile();
    void onRemove();
    void onApply();
    void refreshVersionPreview();

private:
    QTableWidget *fileTable;
    QSpinBox *segmentSpin;
    QSpinBox *newCodeSpin;
    QLabel *currentVersionLabel;
    QLabel *newVersionLabel;
    QLabel *currentCodeLabel;

    QList<FileEntry> entries_;

    QString detectVersion(const QString &path) const;
    int detectVersionCode(const QString &path) const;
};
