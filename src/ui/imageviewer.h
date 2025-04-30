// imageviewer.h
#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QScrollArea>
#include <QVector>
#include <QString>
#include <QSet>

// Forward declarations
class ImageViewerContent;
class ImageLoader;
class QResizeEvent;

/**
 * @brief The ImageViewer class provides scrollable image viewing capabilities.
 *
 * This widget is a container for ImageViewerContent that provides scrolling,
 * manages image loading, and coordinates higher-level features like favorites.
 */
class ImageViewer : public QScrollArea
{
    Q_OBJECT

public:
    /**
     * @brief Constructs an image viewer.
     * @param parent The parent widget.
     */
    explicit ImageViewer(QWidget *parent = nullptr);

    /**
     * @brief Destroys the image viewer.
     */
    ~ImageViewer();

    /**
     * @brief Sets the paths of images to display.
     * @param paths List of image file paths.
     */
    void setImagePaths(const QList<QString> &paths);

    /**
     * @brief Centers the view on a specific image.
     * @param index The index of the image to center on.
     */
    void centerOnImageIndex(int index);

    /**
     * @brief Centers the view on the next image.
     */
    void centerOnNextImage();

    /**
     * @brief Rotates the current image left (counterclockwise).
     */
    void rotateCurrentImageLeft();

    /**
     * @brief Rotates the current image right (clockwise).
     */
    void rotateCurrentImageRight();

    /**
     * @brief Loads favorite images from a file.
     * @param filePath The path to the favorites file (optional).
     */
    void loadFavorites(const QString &filePath = QString());

    /**
     * @brief Saves favorite images to a file.
     */
    void saveFavorites();

    /**
     * @brief Toggles between showing all images or only favorites.
     */
    void toggleFavoritesMode();

    /**
     * @brief Toggles whether the current image is a favorite.
     */
    void toggleCurrentImageFavorite();

    /**
     * @brief Checks if currently showing only favorites.
     * @return True if showing only favorites, false otherwise.
     */
    bool isShowingOnlyFavorites() const { return m_showOnlyFavorites; }

    /**
     * @brief Checks if an image is marked as a favorite.
     * @param path The path to the image.
     * @return True if the image is a favorite, false otherwise.
     */
    bool isImageFavorite(const QString &path) const { return m_favorites.contains(path); }

    /**
     * @brief Gets the image loader.
     * @return Pointer to the image loader.
     */
    ImageLoader* getImageLoader() { return m_imageLoader; }

signals:
    /**
     * @brief Signal emitted when the current image changes.
     * @param index The index of the new current image.
     */
    void currentImageChanged(int index);

protected:
    /**
     * @brief Handles resize events.
     * @param event The resize event.
     */
    void resizeEvent(QResizeEvent *event) override;

private:
    ImageViewerContent *m_content;     ///< The content widget
    ImageLoader *m_imageLoader;        ///< The image loader
    QVector<QString> m_allImagePaths;  ///< All loaded image paths
    QSet<QString> m_favorites;         ///< Set of favorite image paths
    QString m_favoritesFilePath;       ///< Path to favorites file
    bool m_showOnlyFavorites = false;  ///< Whether showing only favorites

    friend class ImageViewerContent;
};

#endif // IMAGEVIEWER_H
