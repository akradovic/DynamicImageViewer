// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QVector>

// Forward declarations
class ImageViewer;
class QLabel;
class QTimer;
class QDragEnterEvent;
class QDropEvent;

/**
 * @brief The MainWindow class is the main application window.
 *
 * This class provides the UI container, menu system, and top-level
 * application controls for the Dynamic Image Viewer.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the main window.
     * @param parent The parent widget.
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief Destroys the main window.
     */
    ~MainWindow();

private slots:
    /**
     * @brief Opens a directory of images.
     */
    void openDirectory();

    /**
     * @brief Navigates to a random image.
     */
    void navigateToRandomImage();

    /**
     * @brief Toggles slideshow mode.
     */
    void toggleSlideshow();

    /**
     * @brief Advances to the next image in slideshow mode.
     */
    void slideshowAdvance();

    /**
     * @brief Sets the interval for slideshow transitions.
     */
    void setSlideshowInterval();

    /**
     * @brief Shows keyboard shortcuts help dialog.
     */
    void showKeyboardShortcuts();

    /**
     * @brief Toggles favorites-only mode.
     */
    void toggleFavoritesMode();

    /**
     * @brief Toggles the current image as a favorite.
     */
    void toggleCurrentImageFavorite();

    /**
     * @brief Updates image information in status bar.
     * @param index The index of the current image.
     */
    void updateImageInfo(int index);

private:
    /**
     * @brief Sets up the user interface.
     */
    void setupUI();

    /**
     * @brief Loads images from a directory.
     * @param dirPath The path to the directory.
     */
    void loadImagesFromDirectory(const QString &dirPath);

    ImageViewer *m_imageViewer;                ///< The image viewer widget
    QVector<QString> m_imagePaths;             ///< Paths to images
    QTimer *m_slideshowTimer;                  ///< Timer for slideshow
    int m_slideshowInterval = 3000;            ///< Slideshow interval in ms
    bool m_slideshowActive = false;            ///< Whether slideshow is active
    QLabel *m_imageInfoLabel;                  ///< Label for image information

protected:
    /**
     * @brief Handles drag enter events for drag and drop.
     * @param event The drag enter event.
     */
    void dragEnterEvent(QDragEnterEvent *event) override;

    /**
     * @brief Handles drop events for importing images.
     * @param event The drop event.
     */
    void dropEvent(QDropEvent *event) override;
};

#endif // MAINWINDOW_H
