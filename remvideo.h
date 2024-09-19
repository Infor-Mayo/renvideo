#ifndef REMVIDEO_H
#define REMVIDEO_H

#include <QMainWindow>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QSpinBox>
#include <QTextEdit>
#include <QDateTime>
#include <QDebug>

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
    void on_enfo_clicked(bool checked);
    void on_eimg_clicked(bool checked);
    void on_elog_clicked(bool checked);

private:
    Ui::RemVideo *ui;
    void renameVideosInFolder(const QString& folderPath, int seasonNumber, const QString& customName, bool deleteImages, bool deleteNfoFiles, bool deleteLog);
    void restoreVideosInFolder(const QString& folderPath);
    void appendToLog(const QString& logText);

    QDateTime lastLogTime;
    const int batchTimeThresholdMs = 1000; // 1 segundo de umbral para considerar registros como lote
};

#endif // REMVIDEO_H
