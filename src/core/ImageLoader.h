// imageloader.h
#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QObject>
#include <QString>
#include <QPixmap>
#include <QThreadPool>
#include <QMutex>

/**
 * @brief The ImageLoader class manages asynchronous loading of images.
 *
 * Utilizes a thread pool to load multiple images in parallel, improving
 * responsiveness for large image collections.
 */
class ImageLoader : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructs an image loader.
     * @param parent The parent object.
     */
    explicit ImageLoader(QObject *parent = nullptr);

    /**
     * @brief Destroys the image loader, stopping all pending tasks.
     */
    ~ImageLoader();

    /**
     * @brief Initiates asynchronous loading of an image.
     * @param index The index of the image in the collection.
     * @param path The file path to the image.
     */
    void loadImage(int index, const QString &path);

signals:
    /**
     * @brief Signal emitted when an image has been loaded.
     * @param index The index of the loaded image.
     * @param pixmap The loaded image pixmap.
     */
    void imageLoaded(int index, const QPixmap &pixmap);

private:
    QThreadPool m_threadPool;   ///< Thread pool for parallel image loading
    QMutex m_mutex;             ///< Mutex to protect thread-pool access
};

#endif // IMAGELOADER_H
