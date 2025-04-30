// imageviewercontent.h
#ifndef IMAGEVIEWERCONTENT_H
#define IMAGEVIEWERCONTENT_H

#include <QWidget>
#include <QVector>
#include <QString>
#include <QPixmap>
#include <QHash>
#include <QSet>
#include <QPoint>

// Forward declarations
class ImageViewer;
class QWheelEvent;
class QKeyEvent;
class QMouseEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

/**
 * @brief Structure to hold image information and state.
 */
struct ImageInfo {
    QPixmap pixmap;      ///< The image pixmap
    QRect rect;          ///< Rectangle for rendering
    bool loaded = false; ///< Whether the image is loaded
    bool loading = false;///< Whether the image is currently loading
};

/**
 * @brief The ImageViewerContent class handles rendering and interaction with images.
 *
 * This widget is responsible for displaying images, handling user interactions like
 * zooming, panning, and navigation, and optimizing performance through viewport-based
 * image loading and unloading.
 */
class ImageViewerContent : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the image viewer content widget.
     * @param parent The parent ImageViewer.
     */
    explicit ImageViewerContent(ImageViewer *parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~ImageViewerContent();

    /**
     * @brief Sets the paths of images to display.
     * @param paths List of image file paths.
     */
    void setImagePaths(const QList<QString> &paths);

    /**
     * @brief Updates which images are visible based on scrolling position.
     */
    void updateVisibleImages();

    /**
     * @brief Centers the view on a specific image.
     * @param index The index of the image to center on.
     */
    void centerOnSpecificImage(int index);

    /**
     * @brief Rotates the current image.
     * @param degrees The degrees to rotate (positive for clockwise).
     */
    void rotateCurrentImage(int degrees);

    /**
     * @brief Centers the view on the next image.
     */
    void centerOnNextImage();

    /**
     * @brief Centers the view on the previous image.
     */
    void centerOnPreviousImage();

    /**
     * @brief Centers the view on the closest image to the left.
     */
    void centerOnClosestLeftImage();

    /**
     * @brief Centers the view on the closest image to the right.
     */
    void centerOnClosestRightImage();

    /**
     * @brief Constrains the panning offset to keep content in view.
     */
    void constrainPanOffset();

    /**
     * @brief Finds the index of the image closest to the current view center.
     * @return The index of the closest image, or -1 if no images.
     */
    int findClosestImageIndex();

    /**
     * @brief Gets the current image paths.
     * @return Vector of image paths.
     */
    const QVector<QString>& getImagePaths() const { return m_imagePaths; }

protected:
    /**
     * @brief Paints the visible images.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Handles mouse wheel events for zooming.
     * @param event The wheel event.
     */
    void wheelEvent(QWheelEvent *event) override;

    /**
     * @brief Handles resize events to recalculate layouts.
     * @param event The resize event.
     */
    void resizeEvent(QResizeEvent *event) override;

    /**
     * @brief Handles keyboard input for navigation and shortcuts.
     * @param event The key press event.
     */
    void keyPressEvent(QKeyEvent *event) override;

    /**
     * @brief Handles mouse button press for panning and favorites.
     * @param event The mouse press event.
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief Handles mouse movement for panning.
     * @param event The mouse move event.
     */
    void mouseMoveEvent(QMouseEvent *event) override;

    /**
     * @brief Handles mouse button release.
     * @param event The mouse release event.
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

    /**
     * @brief Handles drag enter events for drag and drop.
     * @param event The drag enter event.
     */
    void dragEnterEvent(QDragEnterEvent *event) override;

    /**
     * @brief Handles drag move events.
     * @param event The drag move event.
     */
    void dragMoveEvent(QDragMoveEvent *event) override;

    /**
     * @brief Handles drop events for importing images.
     * @param event The drop event.
     */
    void dropEvent(QDropEvent *event) override;

private:
    // Member variables
    ImageViewer *m_parent;                    ///< Parent ImageViewer
    QVector<QString> m_imagePaths;            ///< Paths to images
    QHash<int, ImageInfo> m_images;           ///< Image data by index
    QSet<int> m_visibleIndexes;               ///< Currently visible image indexes
    int m_currentScrollPosition = 0;          ///< Current horizontal scroll position

    // TECHNICAL MODIFICATION: Significantly increased visible margin for expanded loading window
    const int m_visibleMargin = 10000;        ///< Margin for preloading (increased from 1000)

    // Favorite icon properties
    QPixmap m_favoriteIcon;                   ///< Icon for favorites
    const int m_favoriteIconSize = 32;        ///< Size of favorite icon
    const int m_favoriteIconMargin = 10;      ///< Margin around favorite icon

    // Zoom and pan state tracking
    float m_zoomFactor = 1.0f;                ///< Current zoom level
    QPoint m_panOffset = QPoint(0, 0);        ///< Offset for panning
    bool m_isPanning = false;                 ///< Whether panning is active
    QPoint m_lastPanPosition;                 ///< Last mouse position during panning

    // Rotation state tracking
    QHash<int, int> m_imageRotations;         ///< Rotation angle by image index

    // Virtual scrolling system
    qint64 m_totalContentWidth = 0;           ///< Total logical width of all images
    int m_viewportStartX = 0;                 ///< Start X position of current viewport in logical coordinates
    int m_viewportEndX = 0;                   ///< End X position of current viewport in logical coordinates
    QHash<int, qint64> m_imageOffsets;        ///< Map of image index to logical position
    QHash<int, int> m_imageWidths;            ///< Map of image index to width
    const int m_maxWidgetWidth = 30000;       ///< Maximum physical widget width (safely below Qt's limit)
    int m_physicalOffsetX = 0;                ///< Physical offset for mapping logical to physical coordinates

    // Private methods
    /**
     * @brief Loads images that are currently visible.
     */
    void loadVisibleImages();

    /**
     * @brief Unloads images that are far from the visible area.
     */
    void unloadInvisibleImages();

    /**
     * @brief Updates the layout of all images.
     */
    void updateLayout();

    /**
     * @brief Calculates the image height based on aspect ratio.
     * @param imageSize The original size of the image.
     * @return The calculated height.
     */
    int calculateImageHeight(const QSize &imageSize) const;

    /**
     * @brief Calculates the rectangle for an image based on position and viewport.
     * @param imageSize The original size of the image.
     * @param xPos The x position for the image.
     * @param viewportHeight The height of the viewport.
     * @return The calculated rectangle.
     */
    QRect calculateImageRect(const QSize &imageSize, int xPos, int viewportHeight) const;

    /**
     * @brief Calculates the zoomed rectangle for an image.
     * @param originalRect The original rectangle.
     * @return The zoomed rectangle.
     */
    QRect calculateZoomedRect(const QRect &originalRect) const;

    /**
     * @brief Applies zoom with a factor.
     * @param factor The zoom factor to apply.
     * @param centerPoint The center point for zooming.
     */
    void applyZoom(float factor, const QPoint &centerPoint);

    /**
     * @brief Applies a fixed zoom level.
     * @param newZoom The new zoom level.
     * @param centerPoint The center point for zooming.
     */
    void applyFixedZoom(float newZoom, const QPoint &centerPoint);

    /**
     * @brief Maps a viewport point to content coordinates.
     * @param viewportPoint The point in viewport coordinates.
     * @return The point in content coordinates.
     */
    QPoint mapToZoomedContent(const QPoint &viewportPoint) const;

    /**
     * @brief Updates the virtual layout of all images.
     *
     * Calculates and stores the logical positions of all images without
     * creating physical placeholders for images outside the viewport.
     */
    void updateVirtualLayout();

    /**
     * @brief Updates the physical layout based on current scroll position.
     *
     * Creates/updates physical placeholders only for images that should be
     * visible within the current viewport (plus margins).
     */
    void updatePhysicalLayout();

    /**
     * @brief Converts a logical X coordinate to a physical X coordinate.
     * @param logicalX The logical X coordinate.
     * @return The physical X coordinate.
     */
    int logicalToPhysicalX(qint64 logicalX) const;

    /**
     * @brief Converts a physical X coordinate to a logical X coordinate.
     * @param physicalX The physical X coordinate.
     * @return The logical X coordinate.
     */
    qint64 physicalToLogicalX(int physicalX) const;

    /**
     * @brief Updates the scrollbar range to represent the full logical content.
     */
    void updateScrollbarRange();

    /**
     * @brief Calculates the width of an image based on aspect ratio and viewport height.
     * @param imageSize The original size of the image.
     * @param viewportHeight The height of the viewport.
     * @return The calculated width.
     */
    int calculateImageWidth(const QSize &imageSize, int viewportHeight) const;

    /**
     * @brief Calculates which image indexes would be visible in a given logical range.
     * @param startX The logical start X coordinate.
     * @param endX The logical end X coordinate.
     * @return List of image indexes that would be visible.
     */
    QList<int> calculateVisibleImageIndexes(qint64 startX, qint64 endX) const;

private slots:
    /**
     * @brief Handles completion of image loading.
     * @param index The index of the loaded image.
     * @param pixmap The loaded pixmap.
     */
    void onImageLoaded(int index, const QPixmap &pixmap);

    /**
     * @brief Slot to handle scrollbar value changes.
     * @param value The new scrollbar value.
     */
    void onScrollValueChanged(int value);
};

#endif // IMAGEVIEWERCONTENT_H
