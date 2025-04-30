// imageviewercontent.cpp
#include "imageviewercontent.h"
#include "imageviewer.h"
#include "../core/imageloader.h"

#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>
#include <QApplication>
#include <QKeyEvent>
#include <QDebug>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QDir>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QPaintEvent>
#include <QScreen>
#include <QElapsedTimer>
#include <QDebug>

// TECHNICAL MODIFICATION: Increased visible margin for expanded loading window
// const int m_visibleMargin = 1000;  -> now in header with higher value

ImageViewerContent::ImageViewerContent(ImageViewer *parent)
    : QWidget(parent)
    , m_parent(parent)
{
    // Enable mouse tracking for proper event handling
    setMouseTracking(true);

    // Enable keyboard focus for key events
    setFocusPolicy(Qt::StrongFocus);

    // Add null check and use Qt::UniqueConnection
    if (m_parent && m_parent->getImageLoader()) {
        connect(m_parent->getImageLoader(), &ImageLoader::imageLoaded,
                this, &ImageViewerContent::onImageLoaded,
                Qt::UniqueConnection);
    }

    // Connect to scrollbar for virtual scrolling
    if (m_parent) {
        connect(m_parent->horizontalScrollBar(), &QScrollBar::valueChanged,
                this, &ImageViewerContent::onScrollValueChanged,
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

    // Set a safe initial size for the widget
    resize(m_maxWidgetWidth, height());

    // TECHNICAL MODIFICATION: Initialize debug flag
    qDebug() << "ImageViewerContent initialized with visibleMargin =" << m_visibleMargin;
}

ImageViewerContent::~ImageViewerContent()
{
}

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

    // Clear existing images and virtual layout data
    m_images.clear();
    m_imageOffsets.clear();
    m_imageWidths.clear();
    m_totalContentWidth = 0;
    m_physicalOffsetX = 0;
    m_currentScrollPosition = 0;

    // Connect to image loader using proper syntax
    if (m_parent && m_parent->getImageLoader()) {
        connect(m_parent->getImageLoader(), &ImageLoader::imageLoaded,
                this, &ImageViewerContent::onImageLoaded,
                Qt::UniqueConnection);
    }

    // TECHNICAL MODIFICATION: Add diagnostic message
    qDebug() << "Setting image paths - count:" << m_imagePaths.size();

    // Initialize virtual layout
    updateVirtualLayout();

    // Initialize physical layout for current viewport
    updatePhysicalLayout();

    // Update scrollbar range
    updateScrollbarRange();

    // Reset scroll position
    if (m_parent) {
        m_parent->horizontalScrollBar()->setValue(0);
    }

    // Update visible images
    updateVisibleImages();
}

void ImageViewerContent::updateVirtualLayout()
{
    // Skip if no images
    if (m_imagePaths.isEmpty()) return;

    QElapsedTimer timer;
    timer.start();

    // Calculate and store logical positions for all images
    qint64 currentOffset = 0;
    const int viewportHeight = height();

    for (int i = 0; i < m_imagePaths.size(); ++i) {
        // Store the logical offset for this image
        m_imageOffsets[i] = currentOffset;

        // Determine image dimensions
        QSize imageSize;
        if (m_images.contains(i) && m_images[i].loaded) {
            // Use actual dimensions for loaded images
            imageSize = m_images[i].pixmap.size();
        } else {
            // Use standard aspect ratio for unloaded images
            imageSize = QSize(16, 9);
        }

        // Calculate image width based on aspect ratio and viewport height
        int imgWidth = calculateImageWidth(imageSize, viewportHeight);
        m_imageWidths[i] = imgWidth;

        // Update total content width
        currentOffset += imgWidth;
    }

    // Store total content width
    m_totalContentWidth = currentOffset;

    // TECHNICAL MODIFICATION: Enhanced debug output
    qDebug() << "Virtual layout updated: Total width =" << m_totalContentWidth
             << "for" << m_imagePaths.size() << "images (took" << timer.elapsed() << "ms)";
}

int ImageViewerContent::calculateImageWidth(const QSize &imageSize, int viewportHeight) const
{
    // Critical: Validate input dimensions to prevent calculation errors
    if (imageSize.width() <= 0 || imageSize.height() <= 0)
        return 0;

    // Calculate image aspect ratio with double precision
    double imageAspectRatio = static_cast<double>(imageSize.width()) /
                              static_cast<double>(imageSize.height());

    // Calculate maximum available height (viewport constraint)
    int maxHeight = viewportHeight;

    // Determine best-fit dimensions based on aspect ratio
    int imgHeight = maxHeight;
    int imgWidth = static_cast<int>(imgHeight * imageAspectRatio);

    return imgWidth;
}

void ImageViewerContent::updatePhysicalLayout()
{
    if (m_imagePaths.isEmpty()) return;

    QElapsedTimer timer;
    timer.start();

    // Get current viewport logical range (with margins)
    int viewportWidth = m_parent ? m_parent->viewport()->width() : width();
    qint64 logicalScrollPos = m_currentScrollPosition;

    // TECHNICAL MODIFICATION: Use significantly expanded margins for viewport calculation
    // This ensures more images are included in the visible set
    qint64 logicalViewportStart = logicalScrollPos - m_visibleMargin;
    qint64 logicalViewportEnd = logicalScrollPos + viewportWidth + m_visibleMargin;

    // TECHNICAL MODIFICATION: Add diagnostic output
    qDebug() << "Viewport calculation: scroll=" << logicalScrollPos
             << "width=" << viewportWidth
             << "range=" << logicalViewportStart << "to" << logicalViewportEnd;

    // Calculate which images should be visible
    QList<int> visibleIndexes = calculateVisibleImageIndexes(logicalViewportStart, logicalViewportEnd);

    // TECHNICAL MODIFICATION: Add diagnostic output
    qDebug() << "Found" << visibleIndexes.size() << "visible images";
    if (visibleIndexes.size() < 10) {
        qDebug() << "Visible indexes:" << visibleIndexes;
    } else {
        qDebug() << "First 10 visible:" << visibleIndexes.mid(0, 10);
    }

    // Determine physical width needed for visible images
    int physicalWidth = 0;
    for (int index : visibleIndexes) {
        if (m_imageWidths.contains(index)) {
            physicalWidth += m_imageWidths[index];
        }
    }

    // Ensure we have sufficient physical space while staying under Qt's limit
    physicalWidth = qMin(physicalWidth, m_maxWidgetWidth);
    setMinimumWidth(physicalWidth);

    // Update viewable range
    m_viewportStartX = logicalViewportStart;
    m_viewportEndX = logicalViewportEnd;

    // Calculate physical offset for first visible image
    if (!visibleIndexes.isEmpty()) {
        int firstVisibleIndex = visibleIndexes.first();
        m_physicalOffsetX = static_cast<int>(logicalViewportStart - m_imageOffsets[firstVisibleIndex]);
    }

    // Create/update physical rectangles for visible images
    int currentPhysicalX = 0;

    for (int index : visibleIndexes) {
        if (!m_imageWidths.contains(index)) continue;

        int imgWidth = m_imageWidths[index];
        const int viewportHeight = height();

        // Calculate vertical centering offset
        int yOffset = 0;

        // Create rectangle with proper positioning
        QRect rect(currentPhysicalX, yOffset, imgWidth, viewportHeight);

        if (m_images.contains(index)) {
            m_images[index].rect = rect;
        } else {
            // Initialize new image info
            ImageInfo info;
            info.rect = rect;
            info.loaded = false;
            info.loading = false;
            m_images[index] = info;
        }

        // Update current physical position
        currentPhysicalX += imgWidth;
    }

    // TECHNICAL MODIFICATION: Explicitly convert list to set to ensure clear understanding
    m_visibleIndexes.clear();
    for (int index : visibleIndexes) {
        m_visibleIndexes.insert(index);
    }

    // TECHNICAL MODIFICATION: Enhanced diagnostic output
    qDebug() << "Physical layout updated: Created" << visibleIndexes.size()
             << "physical placeholders in" << timer.elapsed() << "ms";
    qDebug() << "Physical widget width set to" << physicalWidth;
}

QList<int> ImageViewerContent::calculateVisibleImageIndexes(qint64 startX, qint64 endX) const
{
    QList<int> result;

    // Safety checks
    if (m_imagePaths.isEmpty() || startX > m_totalContentWidth || endX < 0) {
        qDebug() << "calculateVisibleImageIndexes: No images or range outside content";
        return result;
    }

    // TECHNICAL MODIFICATION: Add diagnostic output
    qDebug() << "calculateVisibleImageIndexes: range" << startX << "to" << endX;

    // Ensure startX is not negative
    startX = qMax(static_cast<qint64>(0), startX);

    // TECHNICAL MODIFICATION: Complete algorithm revision
    // Instead of trying to find the first visible image, scan through all images
    // This ensures we don't miss any images due to calculation errors
    for (int i = 0; i < m_imagePaths.size(); ++i) {
        if (!m_imageOffsets.contains(i) || !m_imageWidths.contains(i)) continue;

        qint64 imgStart = m_imageOffsets[i];
        qint64 imgWidth = m_imageWidths[i];
        qint64 imgEnd = imgStart + imgWidth;

        // Check if image overlaps with visible range
        if (imgStart <= endX && imgEnd >= startX) {
            result.append(i);

            // TECHNICAL MODIFICATION: Add selective diagnostic output for first 10 results
            if (result.size() <= 10) {
                qDebug() << "  Image" << i << "visible at" << imgStart << "-" << imgEnd;
            }
        }

        // TECHNICAL MODIFICATION: Exit early for efficiency when we've gone far past the range
        // But only if we've already found some visible images
        if (imgStart > endX + m_visibleMargin && !result.isEmpty()) {
            break;
        }
    }

    // TECHNICAL MODIFICATION: Add diagnostic output for large ranges
    if (result.size() > 10) {
        qDebug() << "  ... and" << (result.size() - 10) << "more images";
    }

    qDebug() << "  Total visible images:" << result.size();

    return result;
}

int ImageViewerContent::logicalToPhysicalX(qint64 logicalX) const
{
    // Convert from logical to physical coordinate
    return static_cast<int>(logicalX - m_viewportStartX);
}

qint64 ImageViewerContent::physicalToLogicalX(int physicalX) const
{
    // Convert from physical to logical coordinate
    return static_cast<qint64>(physicalX) + m_viewportStartX;
}

void ImageViewerContent::updateScrollbarRange()
{
    if (!m_parent) return;

    QScrollBar *hScrollBar = m_parent->horizontalScrollBar();
    int viewportWidth = m_parent ? m_parent->viewport()->width() : width();

    // Calculate scrollable content width (ensuring it doesn't exceed INT_MAX)
    qint64 scrollableWidth = m_totalContentWidth - viewportWidth;
    int maxScrollValue = scrollableWidth > INT_MAX ? INT_MAX : static_cast<int>(scrollableWidth);
    maxScrollValue = qMax(0, maxScrollValue); // Ensure non-negative

    // Set scrollbar range to represent the full logical content
    hScrollBar->setRange(0, maxScrollValue);

    // Set page step to viewport width
    hScrollBar->setPageStep(viewportWidth);

    // TECHNICAL MODIFICATION: Add diagnostic output
    qDebug() << "Scrollbar range updated: 0 to" << maxScrollValue
             << "(content width:" << m_totalContentWidth
             << ", viewport width:" << viewportWidth << ")";
}

void ImageViewerContent::onScrollValueChanged(int value)
{
    // TECHNICAL MODIFICATION: Add diagnostic output
    QElapsedTimer timer;
    timer.start();
    qDebug() << "\n--- Scroll value changed to" << value
             << "(" << (value * 100 / qMax(1, m_parent->horizontalScrollBar()->maximum())) << "%)";

    // Update current scroll position
    m_currentScrollPosition = value;

    // TECHNICAL MODIFICATION: No need to update virtual layout on every scroll
    // updateVirtualLayout(); - This would be expensive for large collections

    // Update physical layout based on new scroll position
    updatePhysicalLayout();

    // Update visible images for loading/unloading
    updateVisibleImages();

    // TECHNICAL MODIFICATION: Add performance diagnostic output
    qDebug() << "Scroll processing completed in" << timer.elapsed() << "ms";
}

void ImageViewerContent::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // Update virtual layout when height changes
    if (event->oldSize().height() != height()) {
        updateVirtualLayout();
    }

    // Update scrollbar range
    updateScrollbarRange();

    // Update physical layout for current viewport
    updatePhysicalLayout();

    // Update visible images
    updateVisibleImages();
}

void ImageViewerContent::updateVisibleImages()
{
    QElapsedTimer timer;
    timer.start();

    // TECHNICAL MODIFICATION: Add diagnostic output
    qDebug() << "Updating visible images. Visible count:" << m_visibleIndexes.size();

    // Load visible images and unload invisible ones
    loadVisibleImages();
    unloadInvisibleImages();

    // Request repaint
    update();

    // TECHNICAL MODIFICATION: Add performance diagnostic output
    qDebug() << "Visible images updated in" << timer.elapsed() << "ms";
}

void ImageViewerContent::loadVisibleImages()
{
    QElapsedTimer timer;
    timer.start();

    // TECHNICAL MODIFICATION: Add diagnostic output
    qDebug() << "Loading visible images. Count:" << m_visibleIndexes.size();

    // Tracker for images that will be loaded in this update
    int loadInitiatedCount = 0;

    for (int index : m_visibleIndexes) {
        if (index < 0 || index >= m_imagePaths.size()) {
            qDebug() << "  Warning: Image index" << index << "out of range";
            continue;
        }

        // Ensure image info exists for this index
        if (!m_images.contains(index)) {
            // This should not happen, but handle gracefully by skipping
            qDebug() << "  Warning: No ImageInfo for visible index" << index;
            continue;
        }

        ImageInfo &info = m_images[index];

        // If not loaded and not currently loading
        if (!info.loaded && !info.loading) {
            info.loading = true;
            m_parent->getImageLoader()->loadImage(index, m_imagePaths[index]);
            loadInitiatedCount++;

            // TECHNICAL MODIFICATION: Add diagnostic for first few images being loaded
            if (loadInitiatedCount <= 5) {
                qDebug() << "  Initiated loading for image" << index
                         << "path:" << m_imagePaths[index];
            }
        }
    }

    // TECHNICAL MODIFICATION: Add summary diagnostic output
    if (loadInitiatedCount > 5) {
        qDebug() << "  ... and" << (loadInitiatedCount - 5) << "more images";
    }
    qDebug() << "  Loading initiated for" << loadInitiatedCount << "images in"
             << timer.elapsed() << "ms";
}

void ImageViewerContent::unloadInvisibleImages()
{
    QElapsedTimer timer;
    timer.start();

    // Create a set of indexes to keep in memory (visible plus buffer)
    QSet<int> indexesToKeep = m_visibleIndexes;

    // Find images just outside visible area - extend buffer zone
    for (int i = 0; i < m_imagePaths.size(); ++i) {
        if (m_visibleIndexes.contains(i)) continue;

        if (!m_imageOffsets.contains(i) || !m_imageWidths.contains(i)) continue;

        qint64 imgStart = m_imageOffsets[i];
        qint64 imgEnd = imgStart + m_imageWidths[i];

        // TECHNICAL MODIFICATION: Extended buffer zone (3x margin)
        qint64 extendedStart = m_viewportStartX - m_visibleMargin * 3;
        qint64 extendedEnd = m_viewportEndX + m_visibleMargin * 3;

        // Keep images in extended buffer
        if (imgStart <= extendedEnd && imgEnd >= extendedStart) {
            indexesToKeep.insert(i);
        }
    }

    // Count unloaded images
    int unloadedCount = 0;

    // Unload images far from visible area
    for (auto it = m_images.begin(); it != m_images.end();) {
        int index = it.key();
        ImageInfo &info = it.value();

        // Skip if not loaded or in the keep set
        if (!info.loaded || indexesToKeep.contains(index)) {
            ++it;
            continue;
        }

        // Unload the image
        info.pixmap = QPixmap();
        info.loaded = false;
        unloadedCount++;

        // Remove from images if not visible
        if (!m_visibleIndexes.contains(index)) {
            it = m_images.erase(it);
        } else {
            ++it;
        }
    }

    // TECHNICAL MODIFICATION: Add diagnostic output
    qDebug() << "Unloaded" << unloadedCount << "images in" << timer.elapsed() << "ms";
}

void ImageViewerContent::onImageLoaded(int index, const QPixmap &pixmap)
{
    // Safety checks
    if (!m_images.contains(index) || index < 0 || index >= m_imagePaths.size()) {
        qDebug() << "onImageLoaded: Invalid image index" << index;
        return;
    }

    QElapsedTimer timer;
    timer.start();

    // TECHNICAL MODIFICATION: Add diagnostic output
    qDebug() << "Image loaded: index=" << index
             << "valid=" << !pixmap.isNull()
             << "size=" << (pixmap.isNull() ? "null" : QString("%1x%2").arg(pixmap.width()).arg(pixmap.height()));

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

    // Update virtual layout with actual image dimensions
    qint64 oldWidth = m_imageWidths.value(index, 0);
    int newWidth = calculateImageWidth(pixmap.size(), height());

    if (oldWidth != newWidth) {
        // TECHNICAL MODIFICATION: Add diagnostic output for size changes
        qDebug() << "Image" << index << "width changed: old=" << oldWidth << "new=" << newWidth;

        // Update image width
        m_imageWidths[index] = newWidth;

        // Update offsets for all subsequent images
        qint64 offset = m_imageOffsets[index];
        qint64 widthDiff = newWidth - oldWidth;

        // TECHNICAL MODIFICATION: Only update subsequent images if there's a significant change
        if (qAbs(widthDiff) > 5) { // Only update for non-trivial changes
            qDebug() << "Updating offsets for subsequent images, width diff =" << widthDiff;

            for (int i = index + 1; i < m_imagePaths.size(); ++i) {
                if (m_imageOffsets.contains(i)) {
                    m_imageOffsets[i] += widthDiff;
                }
            }

            // Update total content width
            m_totalContentWidth += widthDiff;

            // Update scrollbar range
            updateScrollbarRange();

            // Update physical layout
            updatePhysicalLayout();
        }
    }

    // Request repaint of the affected area
    update(info.rect);

    // TECHNICAL MODIFICATION: Add performance diagnostic output
    qDebug() << "Image" << index << "processed in" << timer.elapsed() << "ms";
}

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

    // TECHNICAL MODIFICATION: Draw enhanced status information
    if (m_parent) {
        QString posText = QString("Scroll: %1/%2 (%3%) | Images: %4 | Visible: %5")
        .arg(m_currentScrollPosition)
            .arg(m_totalContentWidth)
            .arg(m_totalContentWidth > 0 ?
                     static_cast<int>(100.0 * m_currentScrollPosition / m_totalContentWidth) : 0)
            .arg(m_imagePaths.size())
            .arg(m_visibleIndexes.size());

        // Create background for better readability
        QRect textRect = painter.fontMetrics().boundingRect(posText);
        textRect.adjust(-5, -2, 5, 2);
        textRect.moveTopLeft(QPoint(10, 10));
        painter.fillRect(textRect, QColor(0, 0, 0, 180));

        painter.setPen(Qt::white);
        painter.drawText(textRect, Qt::AlignCenter, posText);
    }
}

// Remainder of the implementation remains unchanged

void ImageViewerContent::centerOnSpecificImage(int index)
{
    if (this->m_imagePaths.isEmpty() || !m_imageOffsets.contains(index))
        return;

    // Reset zoom and pan when navigating to a specific image
    m_zoomFactor = 1.0f;
    m_panOffset = QPoint(0, 0);

    // Calculate scroll position to center the image
    qint64 imageOffset = m_imageOffsets[index];
    int imageWidth = m_imageWidths.value(index, 0);
    int viewportWidth = m_parent ? m_parent->viewport()->width() : width();

    // Calculate position to center the image
    int scrollPosition = static_cast<int>(imageOffset + (imageWidth / 2) - (viewportWidth / 2));
    scrollPosition = qBound(0, scrollPosition, static_cast<int>(m_totalContentWidth - viewportWidth));

    // Apply scroll position via scrollbar
    if (m_parent) {
        m_parent->horizontalScrollBar()->setValue(scrollPosition);
    }

    // Emit signal for current image change
    emit m_parent->currentImageChanged(index);
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
    if (m_imagePaths.isEmpty())
        return;

    // Find image to the left of current viewport center
    int viewportWidth = m_parent ? m_parent->viewport()->width() : width();
    qint64 currentCenter = m_currentScrollPosition + (viewportWidth / 2);

    // Find closest image to the left
    int closestLeftIndex = -1;
    qint64 maxLeftDistance = INT_MAX;

    for (int i = 0; i < m_imagePaths.size(); ++i) {
        if (!m_imageOffsets.contains(i) || !m_imageWidths.contains(i)) continue;

        qint64 imgCenter = m_imageOffsets[i] + (m_imageWidths[i] / 2);

        // Check if image is to the left of viewport center
        if (imgCenter < currentCenter) {
            qint64 distance = currentCenter - imgCenter;

            // Update closest left image if this one is closer
            if (distance < maxLeftDistance) {
                maxLeftDistance = distance;
                closestLeftIndex = i;
            }
        }
    }

    // If no image found to the left, wrap to the rightmost image
    if (closestLeftIndex == -1) {
        int rightmostIndex = -1;
        qint64 rightmostPosition = -1;

        for (int i = 0; i < m_imagePaths.size(); ++i) {
            if (!m_imageOffsets.contains(i) || !m_imageWidths.contains(i)) continue;

            qint64 imgRight = m_imageOffsets[i] + m_imageWidths[i];

            if (imgRight > rightmostPosition) {
                rightmostPosition = imgRight;
                rightmostIndex = i;
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
    if (m_imagePaths.isEmpty())
        return;

    // Find image to the right of current viewport center
    int viewportWidth = m_parent ? m_parent->viewport()->width() : width();
    qint64 currentCenter = m_currentScrollPosition + (viewportWidth / 2);

    // Find closest image to the right
    int closestRightIndex = -1;
    qint64 minRightDistance = INT_MAX;

    for (int i = 0; i < m_imagePaths.size(); ++i) {
        if (!m_imageOffsets.contains(i) || !m_imageWidths.contains(i)) continue;

        qint64 imgCenter = m_imageOffsets[i] + (m_imageWidths[i] / 2);

        // Check if image is to the right of viewport center
        if (imgCenter > currentCenter) {
            qint64 distance = imgCenter - currentCenter;

            // Update closest right image if this one is closer
            if (distance < minRightDistance) {
                minRightDistance = distance;
                closestRightIndex = i;
            }
        }
    }

    // If no image found to the right, wrap to the leftmost image
    if (closestRightIndex == -1) {
        int leftmostIndex = -1;
        qint64 leftmostPosition = INT_MAX;

        for (int i = 0; i < m_imagePaths.size(); ++i) {
            if (!m_imageOffsets.contains(i) || !m_imageWidths.contains(i)) continue;

            qint64 imgLeft = m_imageOffsets[i];

            if (imgLeft < leftmostPosition) {
                leftmostPosition = imgLeft;
                leftmostIndex = i;
            }
        }

        closestRightIndex = leftmostIndex;
    }

    // Center on the closest right image
    if (closestRightIndex != -1) {
        centerOnSpecificImage(closestRightIndex);
    }
}

int ImageViewerContent::findClosestImageIndex()
{
    if (m_imagePaths.isEmpty())
        return -1;

    // Find image closest to current viewport center
    int viewportWidth = m_parent ? m_parent->viewport()->width() : width();
    qint64 currentCenter = m_currentScrollPosition + (viewportWidth / 2);

    // Variables to track closest image
    int closestImageIndex = -1;
    qint64 minDistance = INT_MAX;

    for (int i = 0; i < m_imagePaths.size(); ++i) {
        if (!m_imageOffsets.contains(i) || !m_imageWidths.contains(i)) continue;

        qint64 imgCenter = m_imageOffsets[i] + (m_imageWidths[i] / 2);
        qint64 distance = qAbs(imgCenter - currentCenter);

        // Update closest image if this one is closer
        if (distance < minDistance) {
            minDistance = distance;
            closestImageIndex = i;
        }
    }

    return closestImageIndex;
}

void ImageViewerContent::wheelEvent(QWheelEvent *event)
{
    // Check for zoom operation (Shift key modifier)
    if (event->modifiers() & Qt::ShiftModifier) {
        // Handle zoom operation
        event->accept();

        // Get scroll amount for zoom sensitivity
        int delta = event->angleDelta().y();

        // Calculate zoom factor based on scroll direction
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

void ImageViewerContent::rotateCurrentImage(int degrees)
{
    // Find current image index
    int currentIndex = findClosestImageIndex();
    if (currentIndex == -1)
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

void ImageViewerContent::mousePressEvent(QMouseEvent *event)
{
    // Check for middle-click to toggle favorite
    if (event->button() == Qt::MiddleButton) {
        m_parent->toggleCurrentImageFavorite();
        event->accept();
        return;
    }

    // Initiate panning on left button press
    if (event->button() == Qt::LeftButton && m_zoomFactor > 1.0f) {
        m_isPanning = true;
        m_lastPanPosition = event->position().toPoint();
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
        QPoint delta = event->position().toPoint() - m_lastPanPosition;

        // Apply movement to pan offset
        m_panOffset += delta;

        // Apply constraints to keep image fully visible
        constrainPanOffset();

        // Update last position
        m_lastPanPosition = event->position().toPoint();

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
        event->position().toPoint(),
        event->dropAction(),
        event->mimeData(),
        event->buttons(),
        event->modifiers()
        );

    QApplication::sendEvent(m_parent->parentWidget(), newEvent);
    delete newEvent;

    // Accept the original event
    event->acceptProposedAction();
}

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

        // Refresh display
        update();
    }
}

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
