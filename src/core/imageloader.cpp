// imageloader.cpp
#include "imageloader.h"
#include "imageloadtask.h"
#include <QThread>
#include <QMutexLocker>

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
