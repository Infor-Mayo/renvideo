#include "remvideo.h"
#include "ui_remvideo.h"
#include <QFileDialog>
#include <QDirIterator>
#include <QDebug>
#include <QRegularExpression>
#include <QTextStream>

RemVideo::RemVideo(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::RemVideo)
    , lastLogTime(QDateTime())  // Inicializar con un valor nulo
{
    ui->setupUi(this);
    ui->noe->setChecked(true);
}

RemVideo::~RemVideo()
{
    delete ui;
}

void RemVideo::on_selectFolderButton_clicked()
{
    QString currentFolder = QFileDialog::getExistingDirectory(this, tr("Seleccionar Carpeta"));
    if (!currentFolder.isEmpty()) {
        ui->carpeta->setText(currentFolder);
    }
}

void RemVideo::renameVideosInFolder(const QString& folderPath, int seasonNumber, const QString& customName, bool deleteImages, bool deleteNfoFiles, bool deleteLog)
{
    const QStringList videoExtensions = {"*.mp4", "*.mkv", "*.avi", "*.mov", "*.flv", "*.ts"};
    const QStringList imageExtensions = {"*.jpg", "*.jpeg", "*.png", "*.gif", "*.bmp"};
    QDirIterator it(folderPath, videoExtensions + (deleteImages ? imageExtensions : QStringList()) + (deleteNfoFiles ? QStringList("*.nfo") : QStringList()), QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    QRegularExpression re("(\\d+)");

    QString logFilePath = folderPath + "/rename_log.txt";
    QFile logFile(logFilePath);
    if (!logFile.open(QIODevice::Append | QIODevice::Text)) {
        appendToLog(QString("-e- No se pudo abrir el archivo de log"));
        return;
    }
    QTextStream logStream(&logFile);

    while (it.hasNext()) {
        QFileInfo fileInfo(it.next());
        QString suffix = fileInfo.suffix().toLower();
        
        // Eliminar imágenes si deleteImages es verdadero
        if (deleteImages && imageExtensions.contains("*." + suffix)) {
            if (QFile::remove(fileInfo.filePath())) {
                QString logEntry = QString("-d- Imagen eliminada: %1").arg(fileInfo.fileName());
                appendToLog(logEntry);
                logStream << logEntry << "\n";
            } else {
                QString logEntry = QString("-e- Error al eliminar imagen: %1").arg(fileInfo.fileName());
                appendToLog(logEntry);
                logStream << logEntry << "\n";
            }
            continue;
        }
        
        // Eliminar archivos .nfo si deleteNfoFiles es verdadero
        if (deleteNfoFiles && suffix == "nfo") {
            if (QFile::remove(fileInfo.filePath())) {
                QString logEntry = QString("-d- Archivo NFO eliminado: %1").arg(fileInfo.fileName());
                appendToLog(logEntry);
                logStream << logEntry << "\n";
            } else {
                QString logEntry = QString("-e- Error al eliminar archivo NFO: %1").arg(fileInfo.fileName());
                appendToLog(logEntry);
                logStream << logEntry << "\n";
            }
            continue;
        }

        // Continuar con el renombrado de archivos de video
        QString baseName = fileInfo.completeBaseName();
        auto matchIt = re.globalMatch(baseName);

        QString chapterNumber = "01";
        QString detectedSeason = seasonNumber > 0 ? QString::number(seasonNumber) : "01";

        if (matchIt.hasNext()) {
            auto match = matchIt.next();
            if (seasonNumber == 0) {
                detectedSeason = match.captured(1);
            }

            if (matchIt.hasNext()) {
                chapterNumber = matchIt.next().captured(1);
            } else {
                chapterNumber = detectedSeason;
                detectedSeason = seasonNumber > 0 ? QString::number(seasonNumber) : "01";
            }
        }

        QString finalName = QString("capítulo %1x%2 (%3).%4")
                                .arg(detectedSeason, 2, '0')
                                .arg(chapterNumber, 2, '0')
                                .arg(!customName.isEmpty() ? customName : fileInfo.dir().dirName())
                                .arg(fileInfo.suffix());

        QString newFilePath = fileInfo.absolutePath() + "/" + finalName;

        if (fileInfo.filePath() != newFilePath) {
            if (QFile::rename(fileInfo.filePath(), newFilePath)) {
                QString logEntry = QString("-n- %1 -> %2").arg(fileInfo.fileName(), finalName);
                appendToLog(logEntry);
                logStream << logEntry << "\n";
            } else {
                QString logEntry = QString("-e- Error renaming: %1").arg(fileInfo.fileName());
                appendToLog(logEntry);
                logStream << logEntry << "\n";
            }
        } else {
            QString logEntry = QString("-i- El archivo ya tiene el nombre correcto: %1").arg(fileInfo.fileName());
            appendToLog(logEntry);
            logStream << logEntry << "\n";
        }
    }

    logFile.close();

    // Eliminar el archivo de log si deleteLog es verdadero
    if (deleteLog) {
        if (QFile::remove(logFilePath)) {
            QString logEntry = QString("-d- Archivo de log eliminado: %1").arg(logFilePath);
            appendToLog(logEntry);
        } else {
            QString logEntry = QString("-e- Error al eliminar archivo de log: %1").arg(logFilePath);
            appendToLog(logEntry);
        }
    }
}

void RemVideo::restoreVideosInFolder(const QString& folderPath)
{
    QFile globalLogFile(QCoreApplication::applicationDirPath() + "/global_log.txt");
    if (!globalLogFile.open(QIODevice::Append | QIODevice::Text)) {
        appendToLog(QString("-e- No se pudo abrir el archivo de log global"));
        return;
    }
    QTextStream globalLogStream(&globalLogFile);

    QDirIterator it(folderPath, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString currentFolder = it.next();
        QFile logFile(currentFolder + "/rename_log.txt");

        if (logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream logStream(&logFile);
            QString lastRenameEntry;

            while (!logStream.atEnd()) {
                QString line = logStream.readLine();
                if (line.startsWith("-n-")) {
                    lastRenameEntry = line;
                }
            }

            if (!lastRenameEntry.isEmpty()) {
                QStringList parts = lastRenameEntry.split(" -> ");
                if (parts.size() == 2) {
                    QString originalFileName = parts.at(0).mid(4);
                    QString newFileName = parts.at(1);
                    QString originalFilePath = currentFolder + "/" + originalFileName;
                    QString newFilePath = currentFolder + "/" + newFileName;

                    if (QFile::exists(newFilePath)) {
                        if (QFile::rename(newFilePath, originalFilePath)) {
                            QString logEntry = QString("-r- %1 -> %2").arg(newFileName, originalFileName);
                            appendToLog(logEntry);
                            globalLogStream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " - " << logEntry << "\n";
                        } else {
                            QString logEntry = QString("-e- Error restoring: %1").arg(newFileName);
                            appendToLog(logEntry);
                            globalLogStream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " - " << logEntry << "\n";
                        }
                    } else {
                        QString logEntry = QString("File not found for restoration: %1").arg(newFileName);
                        appendToLog(logEntry);
                        globalLogStream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " - " << logEntry << "\n";
                    }
                }
            }
        }
    }

    globalLogFile.close();
}

void RemVideo::appendToLog(const QString& logText)
{
    QDateTime currentTime = QDateTime::currentDateTime();
    QString logFilePath = QCoreApplication::applicationDirPath() + "/global_log.txt";
    QFile globalLogFile(logFilePath);
    
    if (globalLogFile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream globalLogStream(&globalLogFile);

        // Agregar el separador al archivo de log si han pasado más de 10 segundos
        if (lastLogTime.isNull() || lastLogTime.secsTo(currentTime) > 10) {
            QString separator = QString("------ [%1] --------\n").arg(currentTime.toString("yyyy-MM-dd hh:mm:ss"));
            globalLogStream << separator;
            lastLogTime = currentTime;
        }

        // Agregar el mensaje de log al archivo
        globalLogStream << logText << "\n";

        globalLogFile.close();
    } else {
        qDebug() << "No se pudo abrir el archivo de log:" << logFilePath;
    }

    // Agregar solo el mensaje de log a la interfaz de usuario
    ui->logTextEdit->append(logText);
}

void RemVideo::on_renombrar_clicked()
{
    QString folderPath = ui->carpeta->text();
    if (!folderPath.isEmpty()) {
        renameVideosInFolder(folderPath, ui->temporada->value(), ui->nombre->text(), ui->eimg->isChecked(), ui->enfo->isChecked(), ui->elog->isChecked());
    }
}

void RemVideo::on_restaurar_clicked()
{
    QString folderPath = ui->carpeta->text();
    if (!folderPath.isEmpty()) {
        restoreVideosInFolder(folderPath);
    }
}

void RemVideo::on_enfo_clicked(bool checked)
{
    if (checked){
        ui->noe->setChecked(false);
    }else if(!checked&&!ui->eimg->isChecked()&&!ui->elog->isChecked()){
        ui->noe->setChecked(true);
    }

}

void RemVideo::on_eimg_clicked(bool checked)
{
    if (checked){
        ui->noe->setChecked(false);
    }else if(!checked&&!ui->enfo->isChecked()&&!ui->elog->isChecked()){
        ui->noe->setChecked(true);
    }
}

void RemVideo::on_elog_clicked(bool checked)
{
    if (checked){
        ui->noe->setChecked(false);
    }else if(!checked&&!ui->enfo->isChecked()&&!ui->eimg->isChecked()){
        ui->noe->setChecked(true);
    }
}

