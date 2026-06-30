#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QSignalBlocker>

static const QRegularExpression kVerRe(R"((\d+)\.(\d+)\.(\d+))");

static const QRegularExpression kCodeRe(
    R"(((?:versionCode|lockfileVersion|version_code|VERSION_CODE)["'\s]*[:=]\s*["']?)(\d+))");

static QString detectVersionInContent(const QString &content) {
    auto m = kVerRe.match(content);
    return m.hasMatch() ? m.captured(0) : QString{};
}

static int detectVersionCodeInContent(const QString &content) {
    auto m = kCodeRe.match(content);
    return m.hasMatch() ? m.captured(2).toInt() : -1;
}

static QString applyBump(const QString &version, int segment) {
    auto m = kVerRe.match(version);
    if (!m.hasMatch()) return version;
    int major = m.captured(1).toInt();
    int minor = m.captured(2).toInt();
    int patch = m.captured(3).toInt();
    if (segment == 3) { major++; minor = 0; patch = 0; }
    else if (segment == 2) { minor++; patch = 0; }
    else { patch++; }
    return QString("%1.%2.%3").arg(major).arg(minor).arg(patch);
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Version Bumper");
    resize(640, 500);

    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *layout = new QVBoxLayout(central);
    layout->setSpacing(10);
    layout->setContentsMargins(12, 12, 12, 12);

    // Profile bar
    auto *profileBar = new QHBoxLayout;
    profileBar->addWidget(new QLabel("Profile:"));
    profileCombo = new QComboBox;
    profileCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    profileBar->addWidget(profileCombo);
    auto *newProfileBtn    = new QPushButton("New Profile");
    auto *renameProfileBtn = new QPushButton("Rename");
    auto *saveProfileBtn   = new QPushButton("Save Profile");
    profileBar->addWidget(newProfileBtn);
    profileBar->addWidget(renameProfileBtn);
    profileBar->addWidget(saveProfileBtn);
    layout->addLayout(profileBar);

    // File table
    fileTable = new QTableWidget(0, 2, this);
    fileTable->setHorizontalHeaderLabels({"File", "Occurrences"});
    fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    fileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(fileTable);

    auto *btnRow = new QHBoxLayout;
    auto *addBtn    = new QPushButton("Add File");
    auto *removeBtn = new QPushButton("Remove");
    btnRow->addWidget(addBtn);
    btnRow->addWidget(removeBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    auto *form = new QFormLayout;

    segmentSpin = new QSpinBox;
    segmentSpin->setRange(1, 3);
    segmentSpin->setValue(1);
    segmentSpin->setToolTip("1 = patch, 2 = minor, 3 = major");
    form->addRow("Segment (1=patch 2=minor 3=major):", segmentSpin);

    newCodeSpin = new QSpinBox;
    newCodeSpin->setRange(0, 999999);
    newCodeSpin->setValue(0);
    form->addRow("New Version Code:", newCodeSpin);

    currentVersionLabel = new QLabel("—");
    newVersionLabel     = new QLabel("—");
    currentCodeLabel    = new QLabel("—");
    form->addRow("Current Version:", currentVersionLabel);
    form->addRow("New Version:", newVersionLabel);
    form->addRow("Current Version Code:", currentCodeLabel);
    layout->addLayout(form);

    auto *applyBtn = new QPushButton("Apply");
    applyBtn->setFixedHeight(36);
    layout->addWidget(applyBtn);

    // Populate profile combo
    {
        QSignalBlocker b(profileCombo);
        for (const QString &name : saveSystem_.profileNames())
            profileCombo->addItem(name);
    }
    if (profileCombo->count() > 0)
        loadProfile(saveSystem_.load(profileCombo->currentText()));

    connect(newProfileBtn,    &QPushButton::clicked, this, &MainWindow::onNewProfile);
    connect(renameProfileBtn, &QPushButton::clicked, this, &MainWindow::onRenameProfile);
    connect(saveProfileBtn,   &QPushButton::clicked, this, &MainWindow::onSaveProfile);
    connect(profileCombo, &QComboBox::currentTextChanged, this, &MainWindow::onProfileChanged);
    connect(addBtn,    &QPushButton::clicked, this, &MainWindow::onAddFile);
    connect(removeBtn, &QPushButton::clicked, this, &MainWindow::onRemove);
    connect(applyBtn,  &QPushButton::clicked, this, &MainWindow::onApply);
    connect(segmentSpin, &QSpinBox::valueChanged, this, &MainWindow::refreshVersionPreview);
}

// --- Profile slots ---

void MainWindow::onNewProfile() {
    QString name = QInputDialog::getText(this, "New Profile", "Profile name:");
    if (name.isEmpty()) return;
    if (saveSystem_.exists(name)) {
        QMessageBox::warning(this, "Version Bumper", "A profile with that name already exists.");
        return;
    }
    saveSystem_.save(name, {});
    profileCombo->addItem(name);
    QSignalBlocker b(profileCombo);
    profileCombo->setCurrentText(name);
}

void MainWindow::onRenameProfile() {
    QString current = profileCombo->currentText();
    if (current.isEmpty()) return;
    QString newName = QInputDialog::getText(this, "Rename Profile", "New name:", QLineEdit::Normal, current);
    if (newName.isEmpty() || newName == current) return;
    if (saveSystem_.exists(newName)) {
        QMessageBox::warning(this, "Version Bumper", "A profile with that name already exists.");
        return;
    }
    saveSystem_.rename(current, newName);
    int idx = profileCombo->currentIndex();
    {
        QSignalBlocker b(profileCombo);
        profileCombo->setItemText(idx, newName);
    }
}

void MainWindow::onSaveProfile() {
    QString name = profileCombo->currentText();
    if (name.isEmpty()) {
        onNewProfile();
        return;
    }
    ProfileData data;
    data.segment        = segmentSpin->value();
    data.newVersionCode = newCodeSpin->value();
    data.files          = entries_;
    saveSystem_.save(name, data);
    QMessageBox::information(this, "Version Bumper", QString("Profile \"%1\" saved.").arg(name));
}

void MainWindow::onProfileChanged(const QString &name) {
    if (name.isEmpty()) return;
    loadProfile(saveSystem_.load(name));
}

// --- File slots ---

void MainWindow::onAddFile() {
    QString path = QFileDialog::getOpenFileName(this, "Select File");
    if (path.isEmpty()) return;

    bool wasEmpty = entries_.isEmpty();
    addFileRow(path, 1);

    if (wasEmpty) {
        int code = detectVersionCode(path);
        if (code >= 0)
            newCodeSpin->setValue(code + 1);
    }

    refreshVersionPreview();
}

void MainWindow::onRemove() {
    int row = fileTable->currentRow();
    if (row < 0) return;
    fileTable->removeRow(row);
    entries_.removeAt(row);
    refreshVersionPreview();
}

void MainWindow::onApply() {
    if (entries_.isEmpty()) {
        QMessageBox::information(this, "Version Bumper", "No files in the list.");
        return;
    }

    int segment      = segmentSpin->value();
    QString newCodeStr = QString::number(newCodeSpin->value());
    int errorCount   = 0;

    for (const auto &entry : entries_) {
        QFile file(entry.path);
        if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
            errorCount++;
            continue;
        }
        QString content = file.readAll();

        QString currentVer = detectVersionInContent(content);
        if (currentVer.isEmpty()) {
            errorCount++;
            file.close();
            continue;
        }
        QString newVer = applyBump(currentVer, segment);
        int pos = 0;
        for (int i = 0; i < entry.occurrences; ++i) {
            int idx = content.indexOf(currentVer, pos);
            if (idx < 0) break;
            content.replace(idx, currentVer.length(), newVer);
            pos = idx + newVer.length();
        }

        QList<QRegularExpressionMatch> codeMatches;
        auto it = kCodeRe.globalMatch(content);
        while (it.hasNext()) codeMatches.prepend(it.next());
        for (const auto &cm : codeMatches)
            content.replace(cm.capturedStart(2), cm.capturedLength(2), newCodeStr);

        file.seek(0);
        file.resize(0);
        QTextStream out(&file);
        out << content;
        file.close();
    }

    refreshVersionPreview();

    if (errorCount > 0)
        QMessageBox::warning(this, "Version Bumper",
            QString("%1 file(s) could not be updated.").arg(errorCount));
    else
        QMessageBox::information(this, "Version Bumper", "All files updated successfully.");
}

void MainWindow::refreshVersionPreview() {
    if (entries_.isEmpty()) {
        currentVersionLabel->setText("—");
        newVersionLabel->setText("—");
        currentCodeLabel->setText("—");
        return;
    }

    const QString &firstPath = entries_.first().path;
    QString current = detectVersion(firstPath);
    if (current.isEmpty()) {
        currentVersionLabel->setText("(not found)");
        newVersionLabel->setText("—");
    } else {
        currentVersionLabel->setText(current);
        newVersionLabel->setText(applyBump(current, segmentSpin->value()));
    }

    int code = detectVersionCode(firstPath);
    currentCodeLabel->setText(code >= 0 ? QString::number(code) : "(not found)");
}

// --- Helpers ---

void MainWindow::loadProfile(const ProfileData &data) {
    fileTable->setRowCount(0);
    entries_.clear();

    for (const auto &f : data.files)
        addFileRow(f.path, f.occurrences);

    {
        QSignalBlocker b(segmentSpin);
        segmentSpin->setValue(data.segment > 0 ? data.segment : 1);
    }
    newCodeSpin->setValue(data.newVersionCode);

    refreshVersionPreview();
}

void MainWindow::addFileRow(const QString &path, int occurrences) {
    int row = fileTable->rowCount();
    fileTable->insertRow(row);
    fileTable->setItem(row, 0, new QTableWidgetItem(path));

    auto *occSpin = new QSpinBox;
    occSpin->setRange(1, 999);
    occSpin->setValue(occurrences);
    occSpin->setAlignment(Qt::AlignCenter);
    fileTable->setCellWidget(row, 1, occSpin);

    entries_.append({path, occurrences});

    connect(occSpin, &QSpinBox::valueChanged, this, [this, row](int val) {
        if (row < entries_.size())
            entries_[row].occurrences = val;
    });
}

QString MainWindow::detectVersion(const QString &path) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return detectVersionInContent(QString::fromUtf8(file.readAll()));
}

int MainWindow::detectVersionCode(const QString &path) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return -1;
    return detectVersionCodeInContent(QString::fromUtf8(file.readAll()));
}
