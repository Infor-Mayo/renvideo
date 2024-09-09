#ifndef REMVIDEO_H
#define REMVIDEO_H

#include <QMainWindow>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QSpinBox>
#include <QTextEdit>

QT_BEGIN_NAMESPACE
namespace Ui { class RemVideo; }
QT_END_NAMESPACE

class RemVideo : public QMainWindow
{
    Q_OBJECT

public:
    RemVideo(QWidget *parent = nullptr);
    ~RemVideo();

private slots:
    void on_selectFolderButton_clicked();
    void on_renombrar_clicked();
    void on_restaurar_clicked();

private:
    Ui::RemVideo *ui;
    void renameVideosInFolder(const QString& folderPath, int seasonNumber, const QString& customName);
    void restoreVideosInFolder(const QString& folderPath);
    void appendToLog(const QString& logText);
};

#endif // REMVIDEO_H
