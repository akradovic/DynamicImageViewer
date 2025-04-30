// ImageViewer.cpp
#include "imageviewer.h"
#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>
#include <QApplication>
#include <QScreen>
#include <QPaintEvent>
#include <QDebug>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QApplication>
#include <QKeyEvent>
#include <QRandomGenerator>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QApplication>
#include <QDir>
#include <QMessageBox>

// Lines 25-45: ImageViewer constructor implementation
ImageViewer::ImageViewer(QWidget *parent)
    : QScrollArea(parent)
    , m_content(nullptr)  // Initialize to nullptr first
    , m_imageLoader(new ImageLoader(this))
    , m_favoritesFilePath(QDir::homePath() + "/.image_viewer_favorites.txt")
{
    // Create content after m_imageLoader is initialized
    m_content = new ImageViewerContent(this);
    setWidget(m_content);
    setWidgetResizable(true);

    // Set scroll policies
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Configure scroll behavior
    horizontalScrollBar()->setSingleStep(20);

    // Connect scrollbar signal
    connect(horizontalScrollBar(), &QScrollBar::valueChanged,
            m_content, &ImageViewerContent::updateVisibleImages);

    // Enable drag and drop
    setAcceptDrops(true);

    // Load favorites
    loadFavorites();
}

ImageViewer::~ImageViewer()
{
    delete m_imageLoader;
}

// Lines 50-60: Fix implementation of rotateCurrentImage methods
void ImageViewer::rotateCurrentImageLeft()
{
    m_content->rotateCurrentImage(-90);
}

void ImageViewer::rotateCurrentImageRight()
{
    m_content->rotateCurrentImage(90);
}

void ImageViewer::centerOnNextImage()
{
    m_content->centerOnNextImage();
}


// Lines 150-170: setImagePaths method with fixed signal connections
void ImageViewerContent::setImagePaths(const QList<QString> &paths)
{
    // Disconnect previous connections to avoid multiple signals
    if (m_parent && m_parent->getImageLoader()) {
        disconnect(m_parent->getImageLoader(), &ImageLoader::imageLoaded,
                   this, &ImageViewerContent::onImageLoaded);
    }

    // Convert QList to QVector for internal storage
    m_imagePaths.clear();
    for (const QString &path : paths) {
        m_imagePaths.append(path);
    }

    // Clear existing images and update layout
    m_images.clear();
    updateLayout();
    updateVisibleImages();

    // Connect to image loader using proper syntax
    if (m_parent && m_parent->getImageLoader()) {
        connect(m_parent->getImageLoader(), &ImageLoader::imageLoaded,
                this, &ImageViewerContent::onImageLoaded,
                Qt::UniqueConnection);
    }
}

// ImageViewer.cpp - MODIFY resizeEvent to add constraint check
void ImageViewerContent::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // Process height changes for best-fit recalculation
    if (event->oldSize().height() != height()) {
        int totalWidth = 0;
        int currentPos = m_parent->horizontalScrollBar()->value();
        double scrollRatio = (double)currentPos / (double)maximumWidth();

        const int viewportHeight = height();

        // Recalculate all image positions with best-fit dimensions
        for (int i = 0; i < m_imagePaths.size(); ++i) {
            if (m_images.contains(i)) {
                ImageInfo &info = m_images[i];

                // Use actual image size when available
                QSize imageSize = info.loaded ? info.pixmap.size() : QSize(16, 9);

                // Calculate proper rectangle with best-fit dimensions
                info.rect = calculateImageRect(imageSize, totalWidth, viewportHeight);

                // Update running width total
                totalWidth += info.rect.width();
            }
        }

        // Update content width
        setMinimumWidth(totalWidth);

        // Apply constraint to ensure image stays fully visible
        constrainPanOffset();

        // Restore approximate scroll position
        m_parent->horizontalScrollBar()->setValue(
            static_cast<int>(scrollRatio * totalWidth)
            );
    }

    // Update visible images
    updateVisibleImages();
}

int ImageViewer::findClosestImageIndex() const
{
    // Delegate to content widget
    if (m_content) {
        return m_content->findClosestImageIndex();
    }
    return -1;
}

ImageViewerContent::ImageViewerContent(ImageViewer *parent)
    : QWidget(parent)
    , m_parent(parent)
{
    // Enable mouse tracking for proper event handling
    setMouseTracking(true);

    // In ImageViewerContent constructor
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    m_isPanning = false;
    m_isPassiveScrolling = false;
    // Enable keyboard focus for key events
    setFocusPolicy(Qt::StrongFocus);

    // Add null check and use Qt::UniqueConnection
    if (m_parent && m_parent->getImageLoader()) {
        connect(m_parent->getImageLoader(), &ImageLoader::imageLoaded,
                this, &ImageViewerContent::onImageLoaded,
                Qt::UniqueConnection);
    }
    // Load favorite icon
    m_favoriteIcon = QPixmap(":/icons/favorite.png");

    // If icon resource doesn't exist, create a simple star icon
    if (m_favoriteIcon.isNull()) {
        m_favoriteIcon = QPixmap(m_favoriteIconSize, m_favoriteIconSize);
        m_favoriteIcon.fill(Qt::transparent);

        QPainter painter(&m_favoriteIcon);
        painter.setRenderHint(QPainter::Antialiasing);

        // Draw a yellow star
        painter.setPen(QPen(QColor(50, 50, 0), 1));
        painter.setBrush(QColor(255, 215, 0));

        QPolygonF star;
        const int points = 5;
        const double PI = 3.14159265358979323846;

        for (int i = 0; i < points * 2; ++i) {
            double radius = (i % 2 == 0) ? m_favoriteIconSize/2.0 : m_favoriteIconSize/4.0;
            double angle = i * PI / points;
            star << QPointF(m_favoriteIconSize/2.0 + radius * sin(angle),
                            m_favoriteIconSize/2.0 - radius * cos(angle));
        }

        painter.drawPolygon(star);
    }
}

ImageViewerContent::~ImageViewerContent()
{
}

// ImageViewer.cpp - REPLACE updateLayout method with best-fit layout implementation
void ImageViewerContent::updateLayout()
{
    if (m_imagePaths.isEmpty())
        return;

    int totalWidth = 0;
    const int viewportHeight = this->height();

    // Process all images to establish proper layout
    for (int i = 0; i < m_imagePaths.size(); ++i) {
        QSize imageSize;

        // Determine image dimensions based on loading status
        if (m_images.contains(i) && m_images[i].loaded) {
            // Use actual dimensions for loaded images
            imageSize = m_images[i].pixmap.size();
        } else {
            // Use standard aspect ratio for unloaded images
            imageSize = QSize(16, 9);
        }

        // Calculate image rectangle with best-fit dimensions
        QRect rect = calculateImageRect(imageSize, totalWidth, viewportHeight);

        if (m_images.contains(i)) {
            m_images[i].rect = rect;
        } else {
            // Initialize new image info
            ImageInfo info;
            info.rect = rect;
            info.loaded = false;
            info.loading = false;
            m_images[i] = info;
        }

        // Update total width for next image positioning
        totalWidth += rect.width();
    }

    // Set minimum width to accommodate all images
    setMinimumWidth(totalWidth);
}

int ImageViewerContent::calculateImageHeight(const QSize &imageSize) const
{
    // Calculate height while maintaining aspect ratio and filling width
    double ratio = static_cast<double>(imageSize.width()) / imageSize.height();
    return static_cast<int>(width() / ratio);
}

// ImageViewer.cpp - REPLACE calculateImageRect method with best-fit implementation
QRect ImageViewerContent::calculateImageRect(const QSize &imageSize, int xPos, int viewportHeight) const
{
    // Critical: Validate input dimensions to prevent calculation errors
    if (imageSize.width() <= 0 || imageSize.height() <= 0)
        return QRect(xPos, 0, 0, 0);

    // Calculate image aspect ratio with double precision
    double imageAspectRatio = static_cast<double>(imageSize.width()) /
                              static_cast<double>(imageSize.height());

    // Calculate maximum available height (viewport constraint)
    int maxHeight = viewportHeight;

    // Determine best-fit dimensions based on aspect ratio
    int imgHeight = maxHeight;
    int imgWidth = static_cast<int>(imgHeight * imageAspectRatio);

    // Calculate vertical centering offset
    int yOffset = (viewportHeight - imgHeight) / 2;

    // Create rectangle with proper positioning
    return QRect(xPos, yOffset, imgWidth, imgHeight);
}

// ImageViewer.cpp - MODIFY updateVisibleImages for zoom awareness
void ImageViewerContent::updateVisibleImages()
{
    int scrollPos = m_parent->horizontalScrollBar()->value();
    int viewportWidth = m_parent->viewport()->width();

    // Define visible region with margin and zoom
    // Apply zoom factor in reverse to account for zoomed content
    float inverseZoom = 1.0f / m_zoomFactor;
    int leftVisible = static_cast<int>((scrollPos - m_panOffset.x()) * inverseZoom - m_visibleMargin);
    int rightVisible = static_cast<int>((scrollPos + viewportWidth - m_panOffset.x()) * inverseZoom + m_visibleMargin);

    // Update current visible indexes
    m_visibleIndexes.clear();

    for (auto it = m_images.begin(); it != m_images.end(); ++it) {
        int index = it.key();
        const ImageInfo &info = it.value();

        // Check if image is visible in zoomed coordinate space
        if (info.rect.right() >= leftVisible && info.rect.left() <= rightVisible) {
            m_visibleIndexes.insert(index);
        }
    }

    // Load visible images and unload invisible ones
    loadVisibleImages();
    unloadInvisibleImages();

    // Store current scroll position
    m_currentScrollPosition = scrollPos;

    // Request repaint
    update();
}

// ImageViewer.cpp - MODIFY loadVisibleImages method
void ImageViewerContent::loadVisibleImages()
{
    for (int index : m_visibleIndexes) {
        ImageInfo &info = m_images[index];

        // If not loaded and not currently loading
        if (!info.loaded && !info.loading) {
            info.loading = true;
            m_parent->getImageLoader()->loadImage(index, m_imagePaths[index]);
        }
    }
}

// ImageViewer.cpp - REPLACE unloadInvisibleImages method
void ImageViewerContent::unloadInvisibleImages()
{
    // Calculate extended visible area (3x margin)
    const int extendedMargin = m_visibleMargin * 3;
    int leftExtended = m_currentScrollPosition - extendedMargin;
    int rightExtended = m_currentScrollPosition + m_parent->viewport()->width() + extendedMargin;

    for (auto it = m_images.begin(); it != m_images.end(); ++it) {
        int index = it.key();
        ImageInfo &info = it.value();

        // Skip if not loaded
        if (!info.loaded)
            continue;

        // Check if image is far from visible area in horizontal direction
        if (info.rect.right() < leftExtended || info.rect.left() > rightExtended) {
            info.pixmap = QPixmap();  // Clear the pixmap
            info.loaded = false;
        }
    }
}

// Lines 260-300: onImageLoaded with additional safety checks
void ImageViewerContent::onImageLoaded(int index, const QPixmap &pixmap)
{
    // Safety checks
    if (!m_images.contains(index) || index < 0 || index >= m_imagePaths.size()) {
        return;
    }

    ImageInfo &info = m_images[index];
    info.loading = false;

    // Handle invalid pixmaps gracefully
    if (pixmap.isNull()) {
        info.loaded = false;
        update(info.rect);
        return;
    }

    info.pixmap = pixmap;
    info.loaded = true;

    // Update rect based on actual image size
    int xPos = info.rect.left();

    // Calculate best-fit rectangle based on actual image dimensions
    info.rect = calculateImageRect(pixmap.size(), xPos, height());

    // Determine width change for subsequent image repositioning
    int newRight = info.rect.right();
    int oldRight = xPos + info.rect.width();
    int widthDiff = newRight - oldRight;

    // Reposition all subsequent images if width changed
    if (widthDiff != 0) {
        int currentX = info.rect.right();

        for (int i = index + 1; i < m_imagePaths.size(); ++i) {
            if (m_images.contains(i)) {
                ImageInfo &nextInfo = m_images[i];
                nextInfo.rect.moveLeft(currentX);
                currentX += nextInfo.rect.width();
            }
        }

        // Update content width
        setMinimumWidth(qMax(minimumWidth() + widthDiff, 0));
    }

    // Request repaint only for the affected area
    update(info.rect);
}


// In imageviewer.cpp - REPLACE the wheelEvent method in ImageViewerContent class
void ImageViewerContent::wheelEvent(QWheelEvent *event)
{
    // Check for zoom operation (Shift key modifier)
    if (event->modifiers() & Qt::ShiftModifier) {
        // Handle zoom operation
        event->accept();

        // Get scroll amount for zoom sensitivity
        int delta = event->angleDelta().y();

        // Calculate zoom factor based on scroll direction
        // Use smaller increments for smoother zoom
        float factor = (delta > 0) ? 1.1f : 0.9f;

        // Apply zoom with center point at cursor position
        applyZoom(factor, event->position().toPoint());
    } else {
        // Handle normal scrolling with enhanced sensitivity
        event->accept();
        QScrollBar *hScrollBar = m_parent->horizontalScrollBar();

        // Determine scroll amount with adaptive sensitivity
        // The faster the wheel scrolls, the larger the delta
        int scrollAmount = event->angleDelta().y();

        // Scale the scroll amount for more natural feel
        // Invert direction for standard scroll wheel behavior
        int enhancedScrollAmount = static_cast<int>(-scrollAmount / 1.2);

        // Apply scroll position
        hScrollBar->setValue(hScrollBar->value() + enhancedScrollAmount);
    }
}

void ImageViewer::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    m_content->updateVisibleImages();
}

// ImageViewer.cpp - MODIFY keyPressEvent with zoom capability
void ImageViewerContent::keyPressEvent(QKeyEvent *event)
{
    // Check for zoom operations with Shift + Arrow keys
    if (event->modifiers() & Qt::ShiftModifier) {
        switch (event->key()) {
        case Qt::Key_Up:
            // Zoom in by 10%
            applyZoom(1.1f, QPoint(width() / 2, height() / 2));
            event->accept();
            return;

        case Qt::Key_Down:
            // Zoom out by 10%
            applyZoom(0.9f, QPoint(width() / 2, height() / 2));
            event->accept();
            return;
        }
    }

    // Check for favorites mode toggle with Ctrl+F
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->key() == Qt::Key_F) {
            m_parent->toggleFavoritesMode();
            event->accept();
            return;
        }
    }

    // Check for adding to favorites with F key
    if (event->key() == Qt::Key_F && !(event->modifiers() & Qt::ControlModifier)) {
        m_parent->toggleCurrentImageFavorite();
        event->accept();
        return;
    }

    // Handle other navigation keys
    switch (event->key()) {
    case Qt::Key_Space:
        centerOnNextImage();
        event->accept();
        break;

    case Qt::Key_Right:
        centerOnClosestRightImage();
        event->accept();
        break;

    case Qt::Key_Left:
        centerOnClosestLeftImage();
        event->accept();
        break;

    default:
        // Pass unhandled events to parent
        QWidget::keyPressEvent(event);
        break;
    }
}


void ImageViewerContent::centerOnNextImage()
{
    int closestIndex = findClosestImageIndex();
    if (closestIndex == -1)
        return;

    // Find the next image index
    int nextIndex = closestIndex + 1;

    // If we've reached the end, wrap to first image
    if (nextIndex >= m_imagePaths.size())
        nextIndex = 0;

    // Center on the next image
    centerOnSpecificImage(nextIndex);
}

// ImageViewer.cpp - ADD event handler implementations
void ImageViewerContent::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept operation if it contains URLs
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void ImageViewerContent::dragMoveEvent(QDragMoveEvent *event)
{
    // Continue accepting the drag operation
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void ImageViewerContent::dropEvent(QDropEvent *event)
{
    // Forward drop events to parent MainWindow
    // This preserves the centralized image loading logic
    QWidget::dropEvent(event);

    // Propagate event to parent widget for processing
    QDropEvent *newEvent = new QDropEvent(
        event->position().toPoint(),        // Use modern method
        event->dropAction(),
        event->mimeData(),
        event->buttons(),                   // Use modern method
        event->modifiers()                  // Use modern method
        );

    QApplication::sendEvent(m_parent->parentWidget(), newEvent);
    delete newEvent;

    // Accept the original event
    event->acceptProposedAction();
}

void ImageViewer::centerOnImageIndex(int index)
{
    // Validate index
    if (m_imagePaths.isEmpty()) {
        return;
    }

    if (index < 0 || index >= m_imagePaths.size()) {
        return;
    }

    // Store current index
    m_currentIndex = index;

    // Ensure the image is loaded
    if (m_imageLoader) {
        // Force load the image at the selected index if not already loaded
        m_imageLoader->loadImage(index, m_imagePaths[index]);
    }

    // Directly delegate to content widget for centering
    if (m_content) {
        m_content->centerOnSpecificImage(index);
    }

    // Emit signal for current image change
    emit currentImageChanged(index);
}

void ImageViewerContent::centerOnSpecificImage(int index)
{
    // Validate inputs
    if (m_imagePaths.isEmpty() || index < 0 || index >= m_imagePaths.size()) {
        return;
    }

    // Reset zoom and pan when navigating to a specific image
    m_zoomFactor = 1.0f;
    m_panOffset = QPoint(0, 0);

    // Apply boundary constraints to ensure full visibility
    constrainPanOffset();

    // Create a new image info entry if it doesn't exist yet
    if (!m_images.contains(index)) {
        // Initialize new image info with default size
        ImageInfo info;
        QSize defaultSize(800, 600); // Default placeholder size

        // Calculate position for this image
        int totalWidth = 0;
        for (int i = 0; i < index; ++i) {
            if (m_images.contains(i)) {
                totalWidth += m_images[i].rect.width();
            } else {
                // Use estimated width for unloaded images
                QRect estimatedRect = calculateImageRect(QSize(16, 9), totalWidth, height());
                totalWidth += estimatedRect.width();
            }
        }

        // Create a rectangle for this image
        info.rect = calculateImageRect(defaultSize, totalWidth, height());
        info.loaded = false;
        info.loading = false;
        m_images[index] = info;
    }

    // Get the image rectangle
    const ImageInfo &imageInfo = m_images[index];

    // Request image loading if not already loaded or loading
    if (!imageInfo.loaded && !imageInfo.loading && m_parent && m_parent->getImageLoader()) {
        m_parent->getImageLoader()->loadImage(index, m_imagePaths[index]);

        // Mark as loading
        m_images[index].loading = true;
    }

    // Calculate position to center the image
    QScrollBar *hScrollBar = m_parent->horizontalScrollBar();
    int viewportWidth = m_parent->viewport()->width();

    int imageCenter = imageInfo.rect.left() + (imageInfo.rect.width() / 2);
    int scrollPosition = imageCenter - (viewportWidth / 2);

    // Apply scroll position directly
    hScrollBar->setValue(scrollPosition);

    // Update layout with new zoom settings
    updateLayout();

    // Force update of visible images based on new position
    updateVisibleImages();

    // Force redraw
    update();
}


int ImageViewerContent::findClosestImageIndex()
{
    if (m_imagePaths.isEmpty() || m_images.isEmpty())
        return -1;

    // Calculate current viewport center position
    QScrollBar *hScrollBar = m_parent->horizontalScrollBar();
    int viewportWidth = m_parent->viewport()->width();
    int currentCenter = hScrollBar->value() + (viewportWidth / 2);

    // Variables to track closest image
    int closestImageIndex = -1;
    int minDistance = INT_MAX;

    // Find image closest to center
    for (auto it = m_images.begin(); it != m_images.end(); ++it) {
        int index = it.key();
        const ImageInfo &info = it.value();

        // Calculate image center
        int imageCenter = info.rect.left() + (info.rect.width() / 2);

        // Calculate distance to viewport center
        int distance = std::abs(imageCenter - currentCenter);

        // Update closest image if this one is closer
        if (distance < minDistance) {
            minDistance = distance;
            closestImageIndex = index;
        }
    }

    return closestImageIndex;
}


void ImageViewerContent::centerOnPreviousImage()
{
    int closestIndex = findClosestImageIndex();
    if (closestIndex == -1)
        return;

    // Find the previous image index
    int prevIndex = closestIndex - 1;

    // If we've reached the beginning, wrap to last image
    if (prevIndex < 0)
        prevIndex = m_imagePaths.size() - 1;

    // Center on the previous image
    centerOnSpecificImage(prevIndex);
}

void ImageViewerContent::centerOnClosestLeftImage()
{
    if (m_imagePaths.isEmpty() || m_images.isEmpty())
        return;

    // Calculate current viewport center position
    QScrollBar *hScrollBar = m_parent->horizontalScrollBar();
    int viewportWidth = m_parent->viewport()->width();
    int currentCenter = hScrollBar->value() + (viewportWidth / 2);

    // Variables to track closest image to the left
    int closestLeftIndex = -1;
    int maxLeftDistance = INT_MAX;

    // Find closest image to the left of center
    for (auto it = m_images.begin(); it != m_images.end(); ++it) {
        int index = it.key();
        const ImageInfo &info = it.value();

        // Calculate image center
        int imageCenter = info.rect.left() + (info.rect.width() / 2);

        // Check if image is to the left of viewport center
        if (imageCenter < currentCenter) {
            // Calculate distance from center
            int distance = currentCenter - imageCenter;

            // Update closest left image if this one is closer
            if (distance < maxLeftDistance) {
                maxLeftDistance = distance;
                closestLeftIndex = index;
            }
        }
    }

    // If no image found to the left, wrap to the rightmost image
    if (closestLeftIndex == -1) {
        int rightmostIndex = -1;
        int rightmostPosition = -1;

        // Find rightmost image
        for (auto it = m_images.begin(); it != m_images.end(); ++it) {
            int index = it.key();
            const ImageInfo &info = it.value();

            int imageRight = info.rect.right();
            if (imageRight > rightmostPosition) {
                rightmostPosition = imageRight;
                rightmostIndex = index;
            }
        }

        closestLeftIndex = rightmostIndex;
    }

    // Center on the closest left image
    if (closestLeftIndex != -1) {
        centerOnSpecificImage(closestLeftIndex);
    }
}

void ImageViewerContent::centerOnClosestRightImage()
{
    if (m_imagePaths.isEmpty() || m_images.isEmpty())
        return;

    // Calculate current viewport center position
    QScrollBar *hScrollBar = m_parent->horizontalScrollBar();
    int viewportWidth = m_parent->viewport()->width();
    int currentCenter = hScrollBar->value() + (viewportWidth / 2);

    // Variables to track closest image to the right
    int closestRightIndex = -1;
    int minRightDistance = INT_MAX;

    // Find closest image to the right of center
    for (auto it = m_images.begin(); it != m_images.end(); ++it) {
        int index = it.key();
        const ImageInfo &info = it.value();

        // Calculate image center
        int imageCenter = info.rect.left() + (info.rect.width() / 2);

        // Check if image is to the right of viewport center
        if (imageCenter > currentCenter) {
            // Calculate distance from center
            int distance = imageCenter - currentCenter;

            // Update closest right image if this one is closer
            if (distance < minRightDistance) {
                minRightDistance = distance;
                closestRightIndex = index;
            }
        }
    }

    // If no image found to the right, wrap to the leftmost image
    if (closestRightIndex == -1) {
        int leftmostIndex = -1;
        int leftmostPosition = INT_MAX;

        // Find leftmost image
        for (auto it = m_images.begin(); it != m_images.end(); ++it) {
            int index = it.key();
            const ImageInfo &info = it.value();

            int imageLeft = info.rect.left();
            if (imageLeft < leftmostPosition) {
                leftmostPosition = imageLeft;
                leftmostIndex = index;
            }
        }

        closestRightIndex = leftmostIndex;
    }

    // Center on the closest right image
    if (closestRightIndex != -1) {
        centerOnSpecificImage(closestRightIndex);
    }
}

// imageviewer.cpp - MODIFY paintEvent method (lines 600-650)
void ImageViewerContent::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    // Fill background with solid color
    painter.fillRect(event->rect(), Qt::black);

    // Enable smooth image scaling and antialiasing for rotation
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Process only visible images with zoom applied
    for (int index : m_visibleIndexes) {
        if (!m_images.contains(index) || index >= m_imagePaths.size())
            continue;

        const ImageInfo &info = m_images[index];
        const QString &imagePath = m_imagePaths[index];

        if (info.loaded && !info.pixmap.isNull()) {
            // Calculate zoomed rectangle
            QRect zoomedRect = calculateZoomedRect(info.rect);

            // Check for rotation
            int rotation = m_imageRotations.value(index, 0);

            // Verify intersection with paint area
            if (zoomedRect.intersects(event->rect())) {
                if (rotation == 0) {
                    // No rotation - draw directly
                    painter.drawPixmap(zoomedRect, info.pixmap, info.pixmap.rect());

                    // Draw favorite marker if applicable
                    if (m_parent->isImageFavorite(imagePath)) {
                        // Create star shape
                        QPolygonF star;
                        const int size = 24;
                        const int margin = 10;
                        const int points = 5;
                        const double PI = 3.14159265358979323846;

                        QPointF center(
                            zoomedRect.right() - size/2 - margin,
                            zoomedRect.top() + size/2 + margin
                            );

                        for (int i = 0; i < points * 2; ++i) {
                            double radius = (i % 2 == 0) ? size/2.0 : size/4.0;
                            double angle = i * PI / points;
                            star << QPointF(center.x() + radius * sin(angle),
                                            center.y() - radius * cos(angle));
                        }

                        // Draw star
                        painter.setPen(QPen(QColor(50, 50, 0), 1));
                        painter.setBrush(QColor(255, 215, 0, 220));
                        painter.drawPolygon(star);
                    }
                } else {
                    // Apply rotation
                    painter.save();

                    // Translate to center of the rectangle
                    painter.translate(zoomedRect.center());

                    // Rotate around center
                    painter.rotate(rotation);

                    // Calculate offset to keep image centered
                    QRect rotatedRect = QRect(-zoomedRect.width()/2, -zoomedRect.height()/2,
                                              zoomedRect.width(), zoomedRect.height());

                    // Draw rotated image
                    painter.drawPixmap(rotatedRect, info.pixmap, info.pixmap.rect());

                    // Draw favorite marker if applicable
                    if (m_parent->isImageFavorite(imagePath)) {
                        // Create rotated star shape
                        QPolygonF star;
                        const int size = 24;
                        const int margin = 10;
                        const int points = 5;
                        const double PI = 3.14159265358979323846;

                        QPointF center(
                            rotatedRect.right() - size/2 - margin,
                            rotatedRect.top() + size/2 + margin
                            );

                        for (int i = 0; i < points * 2; ++i) {
                            double radius = (i % 2 == 0) ? size/2.0 : size/4.0;
                            double angle = i * PI / points;
                            star << QPointF(center.x() + radius * sin(angle),
                                            center.y() - radius * cos(angle));
                        }

                        // Draw star
                        painter.setPen(QPen(QColor(50, 50, 0), 1));
                        painter.setBrush(QColor(255, 215, 0, 220));
                        painter.drawPolygon(star);
                    }

                    painter.restore();
                }
            }
        } else {
            // Draw placeholder for images being loaded
            QRect zoomedRect = calculateZoomedRect(info.rect);
            if (zoomedRect.intersects(event->rect())) {
                painter.fillRect(zoomedRect, QColor(40, 40, 40));

                if (info.loading) {
                    // Draw loading indicator
                    painter.setPen(Qt::white);
                    painter.drawText(zoomedRect, Qt::AlignCenter, "Loading...");
                }
            }
        }
    }
}

// ImageViewer.cpp - ADD zoom operation implementation methods
// imageviewer.cpp - REPLACE applyZoom method (lines 702-729)
// ImageViewer.cpp - REPLACE applyZoom method (lines 702-729)
// ImageViewer.cpp - REPLACE applyZoom method (lines 702-729)
void ImageViewerContent::applyZoom(float factor, const QPoint &viewportCenter)
{
    // Get content point under the cursor before zoom
    QPoint contentPoint = mapToZoomedContent(viewportCenter);

    // Store previous zoom for comparison
    float previousZoom = m_zoomFactor;

    // Calculate proposed new zoom factor
    float newZoomFactor = m_zoomFactor * factor;

    // Calculate minimum zoom required to ensure content fits viewport
    float minHeightZoom = static_cast<float>(m_parent->viewport()->height()) / height();
    float minWidthZoom = static_cast<float>(m_parent->viewport()->width()) / width();

    // Use the more restrictive of the two constraints
    float windowMinZoom = qMax(minHeightZoom, minWidthZoom);

    // Ensure we don't go below absolute minimum
    static const float ABSOLUTE_MIN_ZOOM = 0.1f;
    float effectiveMinZoom = qMax(windowMinZoom, ABSOLUTE_MIN_ZOOM);

    // Upper bound for zoom
    static const float MAX_ZOOM = 10.0f;

    // Apply constrained zoom
    m_zoomFactor = qBound(effectiveMinZoom, newZoomFactor, MAX_ZOOM);

    // If zoom factor changed, adjust pan offset to maintain content point under cursor
    if (m_zoomFactor != previousZoom) {
        // Calculate how the content point would appear in viewport coordinates after zoom
        QPoint viewportPointAfterZoom = QPoint(
            static_cast<int>(contentPoint.x() * m_zoomFactor) + m_panOffset.x(),
            static_cast<int>(contentPoint.y() * m_zoomFactor) + m_panOffset.y()
            );

        // Calculate adjustment needed to keep content point under cursor
        QPoint adjustment = viewportPointAfterZoom - viewportCenter;

        // Apply adjustment to pan offset
        m_panOffset -= adjustment;

        // Apply boundary constraints to prevent image from moving outside viewport
        constrainPanOffset();

        // Trigger layout update and viewport refresh
        updateLayout();
        update();
    }
}

// ImageViewer.cpp - ADD this new method after applyZoom
void ImageViewerContent::constrainPanOffset()
{
    // Get viewport dimensions
    int viewportWidth = m_parent->viewport()->width();
    int viewportHeight = m_parent->viewport()->height();

    // Calculate content dimensions after zoom
    int zoomedWidth = static_cast<int>(width() * m_zoomFactor);
    int zoomedHeight = static_cast<int>(height() * m_zoomFactor);

    // Calculate the maximum allowed pan offsets to keep content visible
    int minX, maxX, minY, maxY;

    // For X axis constraints
    if (zoomedWidth <= viewportWidth) {
        // Content width is smaller than viewport - center it
        minX = (viewportWidth - zoomedWidth) / 2;
        maxX = minX;
    } else {
        // Content wider than viewport - allow panning within bounds
        minX = viewportWidth - zoomedWidth;
        maxX = 0;
    }

    // For Y axis constraints
    if (zoomedHeight <= viewportHeight) {
        // Content height is smaller than viewport - center it
        minY = (viewportHeight - zoomedHeight) / 2;
        maxY = minY;
    } else {
        // Content taller than viewport - allow panning within bounds
        minY = viewportHeight - zoomedHeight;
        maxY = 0;
    }

    // Apply constraints
    m_panOffset.setX(qBound(minX, m_panOffset.x(), maxX));
    m_panOffset.setY(qBound(minY, m_panOffset.y(), maxY));
}

void ImageViewerContent::applyFixedZoom(float newZoom, const QPoint &centerPoint)
{
    // Calculate zoom factor for fixed percentage change
    float factor = newZoom / m_zoomFactor;
    applyZoom(factor, centerPoint);
}

QRect ImageViewerContent::calculateZoomedRect(const QRect &originalRect) const
{
    // Calculate zoomed rectangle dimensions
    int zoomedWidth = static_cast<int>(originalRect.width() * m_zoomFactor);
    int zoomedHeight = static_cast<int>(originalRect.height() * m_zoomFactor);

    // Apply pan offset to position
    QPoint zoomedTopLeft = QPoint(
        static_cast<int>(originalRect.left() * m_zoomFactor) + m_panOffset.x(),
        static_cast<int>(originalRect.top() * m_zoomFactor) + m_panOffset.y()
        );

    return QRect(zoomedTopLeft, QSize(zoomedWidth, zoomedHeight));
}

QPoint ImageViewerContent::mapToZoomedContent(const QPoint &viewportPoint) const
{
    // Convert viewport coordinates to content coordinates with zoom factor
    return QPoint(
        static_cast<int>((viewportPoint.x() - m_panOffset.x()) / m_zoomFactor),
        static_cast<int>((viewportPoint.y() - m_panOffset.y()) / m_zoomFactor)
        );
}

void ImageViewerContent::mousePressEvent(QMouseEvent *event)
{
    // Always accept the event first to ensure it's processed
    event->accept();

    // Check for middle-click to toggle favorite
    if (event->button() == Qt::MiddleButton) {
        m_parent->toggleCurrentImageFavorite();
        return;
    }

    // Initiate panning on left button press - REMOVE ZOOM CONDITION
    if (event->button() == Qt::LeftButton) {
        m_isPanning = true;
        m_lastPanPosition = event->position().toPoint();
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    // Enable passive scrolling with right mouse button
    if (event->button() == Qt::RightButton) {
        m_isPassiveScrolling = true;
        m_lastScrollPosition = event->position().toPoint();
        setCursor(Qt::SizeHorCursor);
        return;
    }

    // Call base class implementation for other buttons
    QWidget::mousePressEvent(event);
}

void ImageViewerContent::mouseMoveEvent(QMouseEvent *event)
{
    // Always accept the event first
    event->accept();

    // Process panning if active (left button held down)
    if (m_isPanning) {
        // Calculate movement delta
        QPoint delta = event->position().toPoint() - m_lastPanPosition;

        // Apply movement to pan offset
        m_panOffset += delta;

        // Apply constraints to keep image fully visible
        constrainPanOffset();

        // Update last position
        m_lastPanPosition = event->position().toPoint();

        // Refresh display
        update();
        return;
    }

    // Process passive scrolling (right button held down)
    if (m_isPassiveScrolling) {
        // Get current cursor position
        QPoint currentPos = event->position().toPoint();

        // Calculate delta from last position
        QPoint delta = currentPos - m_lastScrollPosition;

        // Calculate horizontal scroll amount based on X movement
        int scrollAmount = -delta.x();  // Invert for natural feeling

        // Apply scroll with sensitivity factor
        QScrollBar *hScrollBar = m_parent->horizontalScrollBar();
        hScrollBar->setValue(hScrollBar->value() + scrollAmount);

        // Update last position
        m_lastScrollPosition = currentPos;
        return;
    }

    // Call base class implementation if no special handling
    QWidget::mouseMoveEvent(event);
}

// In imageviewer.cpp - REPLACE the mouseReleaseEvent method in ImageViewerContent class
void ImageViewerContent::mouseReleaseEvent(QMouseEvent *event)
{
    // Always accept the event first
    event->accept();

    // End panning operation with left button
    if (event->button() == Qt::LeftButton && m_isPanning) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        return;
    }

    // End passive scrolling with right button
    if (event->button() == Qt::RightButton && m_isPassiveScrolling) {
        m_isPassiveScrolling = false;
        setCursor(Qt::ArrowCursor);
        return;
    }

    // Call base class implementation for other buttons
    QWidget::mouseReleaseEvent(event);
}


// Lines 850-880: Implement rotateCurrentImage method
void ImageViewerContent::rotateCurrentImage(int degrees)
{
    // Find current image index
    int currentIndex = findClosestImageIndex();
    if (currentIndex == -1 || !m_images.contains(currentIndex))
        return;

    // Update rotation for this image
    if (!m_imageRotations.contains(currentIndex)) {
        m_imageRotations[currentIndex] = 0;
    }

    m_imageRotations[currentIndex] = (m_imageRotations[currentIndex] + degrees) % 360;
    if (m_imageRotations[currentIndex] < 0) {
        m_imageRotations[currentIndex] += 360;
    }

    // Force redraw
    update();
}

// imageviewer.cpp - ADD loadFavorites method (after constructor)
void ImageViewer::loadFavorites(const QString &filePath)
{
    // Use provided file path or default
    QString path = filePath.isEmpty() ? m_favoritesFilePath : filePath;
    m_favoritesFilePath = path;

    // Clear existing favorites
    m_favorites.clear();

    // Read from file
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                m_favorites.insert(line);
            }
        }
        file.close();
    }
}

// imageviewer.cpp - ADD saveFavorites method (after loadFavorites)
void ImageViewer::saveFavorites()
{
    QFile file(m_favoritesFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString &path : m_favorites) {
            out << path << "\n";
        }
        file.close();
    }
}
// imageviewer.cpp - ADD toggleFavoritesMode method (after saveFavorites)
void ImageViewer::toggleFavoritesMode()
{
    // Toggle between showing all images or only favorites
    m_showOnlyFavorites = !m_showOnlyFavorites;

    // Handle case where we're showing only favorites
    if (m_showOnlyFavorites) {
        // Create list of favorites from all loaded images
        QList<QString> favoritePaths;

        for (const QString &path : m_allImagePaths) {
            if (m_favorites.contains(path)) {
                favoritePaths.append(path);
            }
        }

        // Only proceed if we have favorites to display
        if (!favoritePaths.isEmpty()) {
            m_content->setImagePaths(favoritePaths);
        } else {
            // No favorites found, revert to normal mode
            m_showOnlyFavorites = false;
            QMessageBox::information(this, "No Favorites",
                                     "You don't have any favorites in the current directory.");
        }
    } else {
        // Convert back to normal mode - show all images
        QList<QString> allPathsList;
        for (const QString &path : m_allImagePaths) {
            allPathsList.append(path);
        }
        m_content->setImagePaths(allPathsList);
    }
}

// imageviewer.cpp - ADD toggleCurrentImageFavorite method (after toggleFavoritesMode)
void ImageViewer::toggleCurrentImageFavorite()
{
    // Get current image index
    int currentIndex = m_content->findClosestImageIndex();

    // Verify index is valid
    if (currentIndex < 0) {
        return; // No current image
    }

    // Get the paths from the content widget
    QVector<QString> currentPaths = m_content->getImagePaths();

    // Verify index is within range
    if (currentIndex >= currentPaths.size()) {
        return; // Index out of bounds
    }

    // Get path of current image
    QString path = currentPaths[currentIndex];

    // Toggle favorite status
    if (m_favorites.contains(path)) {
        m_favorites.remove(path);

        // If in favorites mode and removing a favorite, refresh view
        if (m_showOnlyFavorites) {
            toggleFavoritesMode(); // Turn off favorites mode
            toggleFavoritesMode(); // Turn it back on with updated list
        }
    } else {
        m_favorites.insert(path);
    }

    // Save to file
    saveFavorites();

    // Refresh display
    m_content->update();
}

// imageviewer.cpp - REPLACE setImagePaths method (lines 70-85)
void ImageViewer::setImagePaths(const QList<QString> &paths)
{
    // Store full list of paths
    m_allImagePaths.clear();
    for (const QString &path : paths) {
        m_allImagePaths.append(path);
    }

    if (m_showOnlyFavorites) {
        // Filter to only show favorites
        QList<QString> favoritePaths;
        for (const QString &path : m_allImagePaths) {
            if (m_favorites.contains(path)) {
                favoritePaths.append(path);
            }
        }

        if (!favoritePaths.isEmpty()) {
            // Pass only favorite paths to content
            m_content->setImagePaths(favoritePaths);
        } else {
            // No favorites available, switch back to normal mode
            m_showOnlyFavorites = false;
            m_content->setImagePaths(paths);
            QMessageBox::information(this, "No Favorites",
                                     "No favorites found. Showing all images.");
        }
    } else {
        // Normal mode - show all images
        m_content->setImagePaths(paths);
    }

    // Emit signal for current image (first image)
    if (!paths.isEmpty()) {
        emit currentImageChanged(0);
    }
}
