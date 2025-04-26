// ImageLoader.cpp
#include "ImageLoader.h"
#include <QImage>
#include <QMetaType>
#include <QDebug>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>

// ImageLoadTask implementation
class ImageLoadTaskPrivate : public QObject
{
    Q_OBJECT
public:
    explicit ImageLoadTaskPrivate(QObject *parent = nullptr) : QObject(parent) {}

signals:
    void loadCompleted(ImageLoadTask *task);
};

Q_GLOBAL_STATIC(ImageLoadTaskPrivate, taskPrivate)

// Lines 4-26: Replace the ImageLoadTaskPrivate implementation with:

ImageLoadTask::ImageLoadTask(int index, const QString &path)
    : QObject(nullptr), QRunnable()
    , m_index(index)
    , m_path(path)
{
    setAutoDelete(true);
}

void ImageLoadTask::run()
{
    // Load the image in the background thread
    QImage image(m_path);

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

// Lines 27-85: Replace the ImageLoader implementation with:

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

    // Create a task and start it
    ImageLoadTask *task = new ImageLoadTask(index, path);

    // Connect the task's signal to our signal
    connect(task, &ImageLoadTask::loadCompleted,
            this, &ImageLoader::imageLoaded,
            Qt::QueuedConnection);

    m_threadPool.start(task);
}

// Remove line 86: #include "ImageLoader.moc"

void ImageLoader::handleLoadCompleted(ImageLoadTask *task)
{
    if (!task)
        return;

    // Emit the signal with the loaded pixmap
    emit imageLoaded(task->index(), task->pixmap());
}

#include "imageloader.moc"
