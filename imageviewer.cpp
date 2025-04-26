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

// Lines 20-30: Fix constructor initialization
ImageViewer::ImageViewer(QWidget *parent)
    : QScrollArea(parent)
    , m_content(new ImageViewerContent(this))
    , m_imageLoader(new ImageLoader(this))  // Fixed initialization
{
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

void ImageViewer::setImagePaths(const QList<QString> &paths)
{
    // Convert QList to QVector for internal use
    QVector<QString> vectorPaths;
    for (const QString &path : paths) {
        vectorPaths.append(path);
    }

    // Forward to content widget
    m_content->setImagePaths(vectorPaths);

    // Emit signal for current image (first image)
    if (!paths.isEmpty()) {
        emit currentImageChanged(0);
    }
}

void ImageViewerContent::setImagePaths(const QList<QString> &paths)
{
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
    connect(m_parent->m_imageLoader, &ImageLoader::imageLoaded,
            this, &ImageViewerContent::onImageLoaded);
}

void ImageViewer::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    m_content->updateVisibleImages();
}

// ImageViewer.cpp - MODIFY ImageViewerContent constructor to enable keyboard focus
ImageViewerContent::ImageViewerContent(ImageViewer *parent)
    : QWidget(parent)
    , m_parent(parent)
{
    // Enable mouse tracking for proper event handling
    setMouseTracking(true);

    // Enable keyboard focus for key events
    setFocusPolicy(Qt::StrongFocus);

    // Connect to the image loader
    connect(m_parent->m_imageLoader, &ImageLoader::imageLoaded,
            this, &ImageViewerContent::onImageLoaded);
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
            m_parent->m_imageLoader->loadImage(index, m_imagePaths[index]);
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

// Lines 260-300: Add missing slot method
void ImageViewerContent::onImageLoaded(int index, const QPixmap &pixmap)
{
    if (!m_images.contains(index))
        return;

    ImageInfo &info = m_images[index];
    info.pixmap = pixmap;
    info.loaded = true;
    info.loading = false;

    // Update rect based on actual image size
    if (!pixmap.isNull()) {
        // Store current horizontal position
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
            setMinimumWidth(minimumWidth() + widthDiff);
        }
    }

    // Request repaint
    update(info.rect);
}


// ImageViewer.cpp - REPLACE wheelEvent with enhanced implementation
void ImageViewerContent::wheelEvent(QWheelEvent *event)
{
    // Check for zoom operation (Shift key modifier)
    if (event->modifiers() & Qt::ShiftModifier) {
        // Handle zoom operation
        event->accept();

        // Get scroll amount for zoom sensitivity
        int delta = event->angleDelta().y();

        // Calculate zoom factor based on scroll direction
        // Use cube root for smoother zoom progression
        float factor = (delta > 0) ? 1.05f : 0.95f;

        // Apply zoom with center point at cursor position
        applyZoom(factor, event->position().toPoint());
    } else {
        // Handle normal horizontal scrolling
        event->accept();
        QScrollBar *hScrollBar = m_parent->horizontalScrollBar();

        // Determine scroll amount with velocity enhancement
        int scrollAmount = event->angleDelta().y();
        int enhancedScrollAmount = static_cast<int>(-scrollAmount / 1.5);

        // Apply scroll position
        hScrollBar->setValue(hScrollBar->value() + enhancedScrollAmount);
    }
}


// ImageViewer.cpp - REPLACE resizeEvent method with best-fit handling
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

        // Restore approximate scroll position
        m_parent->horizontalScrollBar()->setValue(
            static_cast<int>(scrollRatio * totalWidth)
            );
    }

    // Update visible images
    updateVisibleImages();
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

// ImageViewer.cpp - ADD implementation after existing methods
void ImageViewer::centerOnImageIndex(int index)
{
    if (m_imagePaths.isEmpty() || index < 0 || index >= m_imagePaths.size())
        return;

    m_content->centerOnSpecificImage(index);
}

void ImageViewerContent::centerOnSpecificImage(int index)
{
    if (this->m_imagePaths.isEmpty() || !m_images.contains(index))  // Add 'this->' to clarify member variable
        return;

    // Get the image rectangle
    const ImageInfo &imageInfo = m_images[index];

    // Reset zoom and pan when navigating to a specific image
    m_zoomFactor = 1.0f;
    m_panOffset = QPoint(0, 0);

    // Calculate position to center the image
    QScrollBar *hScrollBar = m_parent->horizontalScrollBar();
    int viewportWidth = m_parent->viewport()->width();

    int imageCenter = imageInfo.rect.left() + (imageInfo.rect.width() / 2);
    int scrollPosition = imageCenter - (viewportWidth / 2);

    // Apply scroll position directly
    hScrollBar->setValue(scrollPosition);

    // Update layout with new zoom settings
    updateLayout();

    // Emit signal for current image change
    emit m_parent->currentImageChanged(index);
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

// Lines 600-650: Update paintEvent with rotation support
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
        if (!m_images.contains(index))
            continue;

        const ImageInfo &info = m_images[index];

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
void ImageViewerContent::applyZoom(float factor, const QPoint &centerPoint)
{
    // Store current position under cursor for centering preservation
    QPoint contentPoint = mapToZoomedContent(centerPoint);

    // Apply zoom with boundary constraints
    float previousZoom = m_zoomFactor;
    m_zoomFactor *= factor;

    // Constrain zoom factor within operational limits
    static const float MIN_ZOOM = 0.1f;
    static const float MAX_ZOOM = 10.0f;
    m_zoomFactor = qBound(MIN_ZOOM, m_zoomFactor, MAX_ZOOM);

    // If zoom factor changed, adjust pan offset to maintain center point
    if (m_zoomFactor != previousZoom) {
        // Calculate new position of contentPoint after zoom
        QPoint newCenter = QPoint(
            static_cast<int>(contentPoint.x() * m_zoomFactor / previousZoom),
            static_cast<int>(contentPoint.y() * m_zoomFactor / previousZoom)
            );

        // Calculate adjustment to keep contentPoint under the cursor
        QPoint adjustment = newCenter - contentPoint;

        // Apply adjustment to pan offset
        m_panOffset -= adjustment;

        // Trigger layout update and viewport refresh
        updateLayout();
        update();
    }
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

// Lines 440-460: Update mouse event methods to use modern Qt
void ImageViewerContent::mousePressEvent(QMouseEvent *event)
{
    // Initiate panning on left button press
    if (event->button() == Qt::LeftButton && m_zoomFactor > 1.0f) {
        m_isPanning = true;
        m_lastPanPosition = event->position().toPoint();  // Modern syntax
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void ImageViewerContent::mouseMoveEvent(QMouseEvent *event)
{
    // Process panning if active
    if (m_isPanning) {
        // Calculate movement delta
        QPoint delta = event->position().toPoint() - m_lastPanPosition;  // Modern syntax

        // Apply movement to pan offset
        m_panOffset += delta;

        // Update last position
        m_lastPanPosition = event->position().toPoint();  // Modern syntax

        // Refresh display
        update();
        event->accept();
    } else {
        QWidget::mouseMoveEvent(event);
    }
}

void ImageViewerContent::mouseReleaseEvent(QMouseEvent *event)
{
    // End panning operation
    if (event->button() == Qt::LeftButton && m_isPanning) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QWidget::mouseReleaseEvent(event);
    }
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
