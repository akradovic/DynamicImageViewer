// ImageLoader.cpp
// Lines 1-95: Complete ImageLoader implementation
#include "ImageLoader.h"
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

    QPixmap pixmap = QPixmap::fromImage(image);

    // Signal completion
    emit loadCompleted(m_index, pixmap);
}

ImageLoader::ImageLoader(QObject *parent)
    : QObject(parent)
{
    // Set thread pool limits - adjust based on system capabilities
    m_threadPool.setMaxThreadCount(QThread::idealThreadCount());
}

ImageLoader::~ImageLoader()
{
    m_threadPool.clear();
    m_threadPool.waitForDone();
}

void ImageLoader::loadImage(int index, const QString &path)
{
    QMutexLocker locker(&m_mutex);

    // Create a task
    ImageLoadTask *task = new ImageLoadTask(index, path);

    // Connect the task's signal directly to our signal
    connect(task, &ImageLoadTask::loadCompleted,
            this, &ImageLoader::imageLoaded,
            Qt::QueuedConnection);

    // Start the task
    m_threadPool.start(task);
}

#include "imageloader.moc"
