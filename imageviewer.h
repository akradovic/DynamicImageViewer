// ImageViewer.h
#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QScrollArea>
#include <QVector>
#include <QString>
#include <QPixmap>
#include <QHash>
#include <QFuture>
#include <QMutex>
#include "ImageLoader.h"
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>

struct ImageInfo {
    QPixmap pixmap;
    QRect rect;
    bool loaded = false;
    bool loading = false;
};


class ImageViewerContent;

// Lines 15-40: Update ImageViewer class to make ImageLoader accessible
class ImageViewer : public QScrollArea
{
    Q_OBJECT

public:
    explicit ImageViewer(QWidget *parent = nullptr);
    ~ImageViewer();

    void setImagePaths(const QList<QString> &paths);  // Ensure it's QList, not QVector

    void centerOnImageIndex(int index);
    void centerOnNextImage();
    void rotateCurrentImageLeft();
    void rotateCurrentImageRight();
    void loadFavorites(const QString &filePath = QString());
    void saveFavorites();
    void toggleFavoritesMode();
    void toggleCurrentImageFavorite();
    bool isShowingOnlyFavorites() const { return m_showOnlyFavorites; }
    bool isImageFavorite(const QString &path) const { return m_favorites.contains(path); }
    ImageLoader* getImageLoader() { return m_imageLoader; }  // ADD THIS METHOD


signals:
    void currentImageChanged(int index);

protected:
    void resizeEvent(QResizeEvent *event) override;


private:
    ImageViewerContent *m_content;
    ImageLoader *m_imageLoader;
    QVector<QString> m_allImagePaths;    // Store all loaded images
    // Member variables
    QVector<QString> m_imagePaths;
    QHash<int, ImageInfo> m_images;
    QSet<int> m_visibleIndexes;
    int m_currentScrollPosition = 0;
    const int m_visibleMargin = 1000;

    // Favorite icon properties
    QPixmap m_favoriteIcon;
    const int m_favoriteIconSize = 32;
    const int m_favoriteIconMargin = 10;

    QSet<QString> m_favorites;  // Track favorites by path
    QString m_favoritesFilePath;
    bool m_showOnlyFavorites = false;

    friend class ImageViewerContent;

};

class ImageViewerContent : public QWidget
{
    Q_OBJECT

public:
    explicit ImageViewerContent(ImageViewer *parent = nullptr);
    ~ImageViewerContent();

    void setImagePaths(const QList<QString> &paths);  // Change from QVector to QList
    void updateVisibleImages();
    void centerOnSpecificImage(int index);
    void rotateCurrentImage(int degrees);  // Changed from private to public
    void centerOnNextImage();              // Changed from private to public
    void centerOnPreviousImage();          // Changed from private to public
    void centerOnClosestLeftImage();       // Changed from private to public
    void centerOnClosestRightImage();      // Changed from private to public
    void constrainPanOffset();
    int findClosestImageIndex();
    const QVector<QString>& getImagePaths() const { return m_imagePaths; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;  // Add this line
    void dragMoveEvent(QDragMoveEvent *event) override;    // Add this line
    void dropEvent(QDropEvent *event) override;

private:
    // Member variables
    ImageViewer *m_parent;
    QVector<QString> m_imagePaths;
    QHash<int, ImageInfo> m_images;
    QSet<int> m_visibleIndexes;
    int m_currentScrollPosition = 0;
    const int m_visibleMargin = 1000;

    // Favorite icon properties
    QPixmap m_favoriteIcon;
    const int m_favoriteIconSize = 32;
    const int m_favoriteIconMargin = 10;

    // Zoom and pan state tracking
    float m_zoomFactor = 1.0f;
    QPoint m_panOffset = QPoint(0, 0);
    bool m_isPanning = false;
    QPoint m_lastPanPosition;

    // Rotation state tracking
    QHash<int, int> m_imageRotations;

    // Private methods
    void loadVisibleImages();
    void unloadInvisibleImages();
    void updateLayout();
    int calculateImageHeight(const QSize &imageSize) const;
    QRect calculateImageRect(const QSize &imageSize, int xPos, int viewportHeight) const;
    QRect calculateZoomedRect(const QRect &originalRect) const;
    void applyZoom(float factor, const QPoint &centerPoint);
    void applyFixedZoom(float newZoom, const QPoint &centerPoint);
    QPoint mapToZoomedContent(const QPoint &viewportPoint) const;


private slots:
    void onImageLoaded(int index, const QPixmap &pixmap);
};


#endif // IMAGEVIEWER_H
