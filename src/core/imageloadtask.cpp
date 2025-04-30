// imageloadtask.cpp
#include "imageloadtask.h"
#include <QImage>
#include <QDebug>
#include <QFileInfo>

ImageLoadTask::ImageLoadTask(int index, const QString &path)
    : QObject(nullptr), QRunnable()
    , m_index(index)
    , m_path(path)
{
    setAutoDelete(true);
}

void ImageLoadTask::run()
{
    // Check if path is valid before loading
    QFileInfo fileInfo(m_path);
    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        qDebug() << "Error: Cannot read image file:" << m_path;
        emit loadCompleted(m_index, QPixmap());
        return;
    }

    // Load the image in the background thread
    QImage image(m_path);

    if (image.isNull()) {
        qDebug() << "Error: Failed to load image:" << m_path;
        emit loadCompleted(m_index, QPixmap());
        return;
    }

    // Scale down very large images to save memory
    const int MAX_DIMENSION = 4096;
    if (image.width() > MAX_DIMENSION || image.height() > MAX_DIMENSION) {
        image = image.scaled(
            MAX_DIMENSION,
            MAX_DIMENSION,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
            );
    }

    m_pixmap = QPixmap::fromImage(image);

    // Signal completion
    emit loadCompleted(m_index, m_pixmap);
}
