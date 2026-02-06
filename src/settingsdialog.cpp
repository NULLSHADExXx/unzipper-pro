#include "settingsdialog.h"
#include <QDir>
#include <QThread>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QSettings>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    setMinimumWidth(450);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QGroupBox *defaultsGroup = new QGroupBox("Defaults");
    QVBoxLayout *defaultsLayout = new QVBoxLayout(defaultsGroup);

    QLabel *outputLabel = new QLabel("Default output folder:");
    defaultOutputEdit = new QLineEdit();
    defaultOutputEdit->setReadOnly(true);
    QPushButton *browseBtn = new QPushButton("Browse...");
    browseBtn->setMaximumWidth(90);
    connect(browseBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseDefaultOutput);

    QHBoxLayout *outputLayout = new QHBoxLayout();
    outputLayout->addWidget(defaultOutputEdit);
    outputLayout->addWidget(browseBtn);
    defaultsLayout->addWidget(outputLabel);
    defaultsLayout->addLayout(outputLayout);

    defaultDeleteAfterCheck = new QCheckBox("Delete archives after extraction");
    defaultOverwriteCheck = new QCheckBox("Overwrite existing files");
    defaultsLayout->addWidget(defaultDeleteAfterCheck);
    defaultsLayout->addWidget(defaultOverwriteCheck);

    QHBoxLayout *parallelLayout = new QHBoxLayout();
    parallelLayout->addWidget(new QLabel("Max parallel extractions:"));
    maxParallelSpin = new QSpinBox();
    maxParallelSpin->setRange(1, 8);
    maxParallelSpin->setSuffix(" archives");
    maxParallelSpin->setToolTip("Number of archives to extract simultaneously (1–8). Lower values reduce CPU load.");
    parallelLayout->addWidget(maxParallelSpin);
    parallelLayout->addStretch();
    defaultsLayout->addLayout(parallelLayout);

    layout->addWidget(defaultsGroup);

    QGroupBox *behaviorGroup = new QGroupBox("Behavior");
    QVBoxLayout *behaviorLayout = new QVBoxLayout(behaviorGroup);
    clearListAfterCheck = new QCheckBox("Clear archive list after successful extraction");
    playSoundCheck = new QCheckBox("Play sound when extraction completes");
    verifyAfterExtractCheck = new QCheckBox("Verify integrity before deleting archives");
    verifyAfterExtractCheck->setToolTip("Re-reads each archive after extraction to verify checksums. Prevents deleting corrupt archives. Doubles read time when enabled.");
    behaviorLayout->addWidget(clearListAfterCheck);
    behaviorLayout->addWidget(playSoundCheck);
    behaviorLayout->addWidget(verifyAfterExtractCheck);
    layout->addWidget(behaviorGroup);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    loadSettings();
}

void SettingsDialog::onBrowseDefaultOutput()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Default Output Folder",
        defaultOutputEdit->text().isEmpty() ? QDir::homePath() : defaultOutputEdit->text());
    if (!dir.isEmpty())
        defaultOutputEdit->setText(dir);
}

void SettingsDialog::loadSettings()
{
    QSettings settings("UnzipperPro", "UnzipperPro");
    defaultOutputEdit->setText(settings.value("defaultOutputPath", QDir::homePath()).toString());
    int ideal = qMin(8, qMax(1, QThread::idealThreadCount()));
    int saved = settings.value("maxParallelExtractions", ideal).toInt();
    maxParallelSpin->setValue(qBound(1, saved, 8));
    defaultDeleteAfterCheck->setChecked(settings.value("defaultDeleteAfter", false).toBool());
    defaultOverwriteCheck->setChecked(settings.value("defaultOverwrite", false).toBool());
    clearListAfterCheck->setChecked(settings.value("clearListAfterExtract", false).toBool());
    playSoundCheck->setChecked(settings.value("playSoundOnComplete", true).toBool());
    verifyAfterExtractCheck->setChecked(settings.value("verifyAfterExtract", false).toBool());
}

void SettingsDialog::saveSettings()
{
    QSettings settings("UnzipperPro", "UnzipperPro");
    settings.setValue("defaultOutputPath", defaultOutputEdit->text());
    settings.setValue("maxParallelExtractions", maxParallelSpin->value());
    settings.setValue("defaultDeleteAfter", defaultDeleteAfterCheck->isChecked());
    settings.setValue("defaultOverwrite", defaultOverwriteCheck->isChecked());
    settings.setValue("clearListAfterExtract", clearListAfterCheck->isChecked());
    settings.setValue("playSoundOnComplete", playSoundCheck->isChecked());
    settings.setValue("verifyAfterExtract", verifyAfterExtractCheck->isChecked());
}

void SettingsDialog::accept()
{
    saveSettings();
    QDialog::accept();
}
