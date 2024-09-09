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
{
    ui->setupUi(this);
}

RemVideo::~RemVideo()
{
    delete ui;
}

void RemVideo::on_selectFolderButton_clicked()
{
    QString currentFolder = QFileDialog::getExistingDirectory(this, "Select Folder");
    if (!currentFolder.isEmpty()) {
        ui->carpeta->setText(currentFolder);
    }
}

void RemVideo::renameVideosInFolder(const QString& folderPath, int seasonNumber, const QString& customName)
{
    // Extensiones de archivos de video permitidas
    QStringList videoExtensions = {"*.mp4", "*.mkv", "*.avi", "*.mov", "*.flv"};

    // Iterar sobre los archivos de video en la carpeta actual y sus subdirectorios
    QDirIterator it(folderPath, videoExtensions, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString filePath = it.next();
        QFileInfo fileInfo(filePath);

        QString baseName = fileInfo.completeBaseName();
        QRegularExpression re("(\\d+)");
        QRegularExpressionMatchIterator matchIt = re.globalMatch(baseName);

        QString chapterNumber = "01"; // Número de capítulo por defecto
        QString detectedSeason = (seasonNumber > 0) ? QString::number(seasonNumber) : "01"; // Temporada ingresada o "01"

        if (matchIt.hasNext()) {
            // Primer número detectado en el nombre
            QRegularExpressionMatch match = matchIt.next();
            if (seasonNumber == 0) {
                detectedSeason = match.captured(1); // Si no se ingresó temporada manual, usar el primer número
            }

            if (matchIt.hasNext()) {
                // Segundo número (capítulo)
                match = matchIt.next();
                chapterNumber = match.captured(1); // Usar segundo número como capítulo
            } else {
                // Si solo hay un número, este es el capítulo
                chapterNumber = detectedSeason;
                detectedSeason = (seasonNumber > 0) ? QString::number(seasonNumber) : "01"; // Usar temporada ingresada o "01"
            }
        }

        // Formato del nombre final
        QString finalName = QString("capítulo %1x%2 (%3).%4")
                                .arg(detectedSeason, 2, '0')
                                .arg(chapterNumber, 2, '0')
                                .arg(!customName.isEmpty() ? customName : fileInfo.dir().dirName()) // Nombre personalizado o carpeta contenedora
                                .arg(fileInfo.suffix());

        QString newFilePath = fileInfo.absolutePath() + "/" + finalName;

        if (QFile::rename(filePath, newFilePath)) { // Renombrar el archivo
            QString logEntry = QString("-n- %1 -> %2").arg(fileInfo.fileName()).arg(finalName);

            // Crear archivo de registro en la carpeta del archivo renombrado
            QString logFilePath = fileInfo.absolutePath() + "/rename_log.txt"; // Archivo de log individual
            QFile logFile(logFilePath);

            // Abrir el archivo de registro en modo de anexado
            if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream logStream(&logFile);
                logStream << logEntry << "\n"; // Guardar el renombre en el archivo de registro
                logFile.close();
            }

            appendToLog(logEntry);  // Mostrar en la interfaz
        } else {
            QString errorLogEntry = QString("-e- Error renaming: %1").arg(fileInfo.fileName());
            appendToLog(errorLogEntry);
        }
    }
}

void RemVideo::restoreVideosInFolder(const QString& folderPath)
{
    // Iterar sobre todas las subcarpetas y archivos de registro
    QDirIterator it(folderPath, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString currentFolder = it.next();
        QString logFilePath = currentFolder + "/rename_log.txt";
        QFile logFile(logFilePath);

        if (logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream logStream(&logFile);
            QStringList logEntries = logStream.readAll().split("\n", Qt::SkipEmptyParts);

            // Buscar la última línea que comienza con -n-
            QString lastRenameEntry;
            for (const QString &entry : logEntries) {
                if (entry.startsWith("-n-")) {
                    lastRenameEntry = entry; // Guardar la última entrada de renombre
                }
            }

            // Si se encontró una entrada de renombre, proceder a restaurar
            if (!lastRenameEntry.isEmpty()) {
                QStringList parts = lastRenameEntry.split(" -> ");
                if (parts.size() == 2) {
                    QString originalFileName = parts.at(0).mid(4); // Eliminar el prefijo -n-
                    QString newFileName = parts.at(1);
                    QString originalFilePath = currentFolder + "/" + originalFileName;
                    QString newFilePath = currentFolder + "/" + newFileName;

                    // Verificar si el nuevo archivo existe antes de renombrar
                    if (QFile::exists(newFilePath)) {
                        if (QFile::rename(newFilePath, originalFilePath)) {
                            QString restoreLogEntry = QString("-r- %1 -> %2").arg(newFileName).arg(originalFileName);
                            appendToLog(restoreLogEntry);
                        } else {
                            QString errorLogEntry = QString("-e- Error restoring: %1").arg(newFileName);
                            appendToLog(errorLogEntry);
                        }
                    } else {
                        appendToLog(QString("File not found for restoration: %1").arg(newFileName));
                    }
                }
            }
            logFile.close();
        }
    }
}

void RemVideo::appendToLog(const QString& logText)
{
    ui->logTextEdit->append(logText); // Mostrar en la interfaz
}

void RemVideo::on_renombrar_clicked()
{
    QString folderPath = ui->carpeta->text();
    QString customName = ui->nombre->text();
    int seasonNumber = ui->temporada->value(); // Valor ingresado de temporada

    if (!folderPath.isEmpty()) {
        renameVideosInFolder(folderPath, seasonNumber, customName); // Llamada a renombrar videos
    }
}

void RemVideo::on_restaurar_clicked()
{
    QString folderPath = ui->carpeta->text();

    if (!folderPath.isEmpty()) {
        restoreVideosInFolder(folderPath); // Restaurar nombres originales
    }
}
