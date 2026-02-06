#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private slots:
    void onBrowseDefaultOutput();
    void accept() override;

private:
    void loadSettings();
    void saveSettings();

    QLineEdit *defaultOutputEdit;
    QSpinBox *maxParallelSpin;
    QCheckBox *defaultDeleteAfterCheck;
    QCheckBox *defaultOverwriteCheck;
    QCheckBox *clearListAfterCheck;
    QCheckBox *playSoundCheck;
    QCheckBox *verifyAfterExtractCheck;
};

#endif
