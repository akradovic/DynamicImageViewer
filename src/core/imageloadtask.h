// imageloadtask.h
#ifndef IMAGELOADTASK_H
#define IMAGELOADTASK_H

#include <QObject>
#include <QRunnable>
#include <QString>
#include <QPixmap>

/**
 * @brief The ImageLoadTask class handles asynchronous loading of a single image.
 *
 * Implements both QObject and QRunnable to enable signal emission within thread pool.
 * Each task is responsible for loading a specific image identified by its index and path.
 */
class ImageLoadTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    /**
     * @brief Constructs an image loading task.
     * @param index The index of the image in the collection.
     * @param path The file path to the image.
     */
    ImageLoadTask(int index, const QString &path);

    /**
     * @brief Default destructor.
     */
    ~ImageLoadTask() override = default;

    /**
     * @brief Executes the image loading operation.
     *
     * This method runs in a worker thread and emits loadCompleted when done.
     */
    void run() override;

    /**
     * @brief Gets the index of the image.
     * @return The image index.
     */
    int index() const { return m_index; }

    /**
     * @brief Gets the loaded pixmap.
     * @return The loaded pixmap.
     */
    QPixmap pixmap() const { return m_pixmap; }

signals:
    /**
     * @brief Signal emitted when image loading completes.
     * @param index The index of the loaded image.
     * @param pixmap The loaded image pixmap.
     */
    void loadCompleted(int index, const QPixmap &pixmap);

private:
    int m_index;         ///< Index of the image in the collection
    QString m_path;      ///< File path to the image
    QPixmap m_pixmap;    ///< Loaded image pixmap
};

#endif // IMAGELOADTASK_H
