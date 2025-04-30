// imageviewer.cpp
#include "imageviewer.h"
#include "imageviewercontent.h"
#include "../core/imageloader.h"

#include <QScrollBar>
#include <QResizeEvent>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QList>
#include <QFileInfo>

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

void ImageViewer::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    m_content->updateVisibleImages();
}

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

void ImageViewer::centerOnImageIndex(int index)
{
    if (m_allImagePaths.isEmpty() || index < 0 || index >= m_allImagePaths.size())
        return;

    m_content->centerOnSpecificImage(index);
}

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
