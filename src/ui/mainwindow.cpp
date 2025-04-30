// mainwindow.cpp
#include "mainwindow.h"
#include "imageviewer.h"

#include <QDir>
#include <QFileDialog>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QInputDialog>
#include <QTimer>
#include <QImageReader>
#include <QFileInfo>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QHBoxLayout>
#include <QRandomGenerator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_imageViewer(new ImageViewer(this))
{
    setupUI();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("Dynamic Image Viewer");

    // Create menu
    QMenu *fileMenu = menuBar()->addMenu("&File");
    QAction *openAction = fileMenu->addAction("&Open Directory...");
    connect(openAction, &QAction::triggered, this, &MainWindow::openDirectory);

    // Add navigation menu
    QMenu *navMenu = menuBar()->addMenu("&Navigation");
    QAction *randomAction = navMenu->addAction("&Random Image");
    randomAction->setShortcut(Qt::Key_Shift + Qt::Key_Right); // Right Shift shortcut
    connect(randomAction, &QAction::triggered, this, &MainWindow::navigateToRandomImage);

    // Add slideshow features to navigation menu
    QAction *slideshowAction = navMenu->addAction("&Slideshow");
    slideshowAction->setShortcut(Qt::Key_F5);
    slideshowAction->setCheckable(true);
    connect(slideshowAction, &QAction::triggered, this, &MainWindow::toggleSlideshow);

    QAction *slideshowTimingAction = navMenu->addAction("Slideshow &Timing...");
    connect(slideshowTimingAction, &QAction::triggered, this, &MainWindow::setSlideshowInterval);

    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    // Set central widget
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    // Add view menu
    QMenu *viewMenu = menuBar()->addMenu("&View");

    // Add rotation actions
    QAction *rotateLeftAction = viewMenu->addAction("Rotate &Left");
    rotateLeftAction->setShortcut(Qt::Key_L);
    connect(rotateLeftAction, &QAction::triggered, [this]() {
        m_imageViewer->rotateCurrentImageLeft();
    });

    QAction *rotateRightAction = viewMenu->addAction("Rotate &Right");
    rotateRightAction->setShortcut(Qt::Key_R);
    connect(rotateRightAction, &QAction::triggered, [this]() {
        m_imageViewer->rotateCurrentImageRight();
    });

    QMenu *favoritesMenu = menuBar()->addMenu("&Favorites");

    QAction *toggleFavoriteAction = favoritesMenu->addAction("&Add/Remove Current Image");
    toggleFavoriteAction->setShortcut(Qt::Key_F);
    connect(toggleFavoriteAction, &QAction::triggered, this, &MainWindow::toggleCurrentImageFavorite);

    QAction *showFavoritesAction = favoritesMenu->addAction("&Show Only Favorites");
    showFavoritesAction->setShortcut(Qt::CTRL + Qt::Key_F);
    showFavoritesAction->setCheckable(true);
    connect(showFavoritesAction, &QAction::triggered, this, &MainWindow::toggleFavoritesMode);

    // Add help menu
    QMenu *helpMenu = menuBar()->addMenu("&Help");

    QAction *shortcutsAction = helpMenu->addAction("&Keyboard Shortcuts");
    shortcutsAction->setShortcut(Qt::Key_F1);
    connect(shortcutsAction, &QAction::triggered, this, &MainWindow::showKeyboardShortcuts);

    // Create button bar
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *randomButton = new QPushButton("Random", this);
    connect(randomButton, &QPushButton::clicked, this, &MainWindow::navigateToRandomImage);
    buttonLayout->addStretch();
    buttonLayout->addWidget(randomButton);

    // Add layouts
    layout->addLayout(buttonLayout);
    layout->addWidget(m_imageViewer);
    setCentralWidget(centralWidget);

    // Set up status bar with image info
    statusBar()->setSizeGripEnabled(true);
    m_imageInfoLabel = new QLabel(this);
    m_imageInfoLabel->setFrameStyle(QFrame::NoFrame);
    m_imageInfoLabel->setAlignment(Qt::AlignRight);
    statusBar()->addPermanentWidget(m_imageInfoLabel, 1);
    statusBar()->showMessage("Ready");

    // Connect to ImageViewer for update notifications
    connect(m_imageViewer, &ImageViewer::currentImageChanged,
            this, &MainWindow::updateImageInfo);
}

void MainWindow::openDirectory()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        "Select Directory with Images",
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );

    if (!dirPath.isEmpty()) {
        loadImagesFromDirectory(dirPath);
    }
}

void MainWindow::loadImagesFromDirectory(const QString &dirPath)
{
    m_imagePaths.clear();

    QDir dir(dirPath);
    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.gif" << "*.webp";
    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

    QStringList fileList = dir.entryList();

    for (const QString &fileName : fileList) {
        m_imagePaths.append(dir.filePath(fileName));
    }

    if (m_imagePaths.isEmpty()) {
        QMessageBox::information(this, "No Images", "No image files found in the selected directory.");
        return;
    }

    m_imageViewer->setImagePaths(m_imagePaths);
    statusBar()->showMessage(QString("Loaded %1 images").arg(m_imagePaths.size()));
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    // Verify mime data contains valid URLs
    if (event->mimeData()->hasUrls()) {
        // Accept drag operation to enable drop
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    // Extract URLs from the mime data
    const QList<QUrl> urls = event->mimeData()->urls();

    // Container for valid image paths
    QList<QString> imagePaths;  // Must use QList to match ImageViewer method

    // Validate file extensions for all dropped items
    QStringList validExtensions = {"jpg", "jpeg", "png", "bmp", "gif", "webp"};

    for (const QUrl &url : urls) {
        // Convert URL to local file path
        QString filePath = url.toLocalFile();
        QFileInfo fileInfo(filePath);

        // Check if the file exists and has valid image extension
        if (fileInfo.exists() && fileInfo.isFile()) {
            QString extension = fileInfo.suffix().toLower();

            if (validExtensions.contains(extension)) {
                imagePaths.append(filePath);
            }
        }
    }

    // Process valid image paths
    if (!imagePaths.isEmpty()) {
        // Clear existing images
        m_imagePaths.clear();
        m_imagePaths = imagePaths;

        // Update image viewer with new paths
        m_imageViewer->setImagePaths(imagePaths);  // Make sure this is QList<QString>
        statusBar()->showMessage(QString("Loaded %1 images").arg(m_imagePaths.size()));
    }

    // Accept the drop event
    event->acceptProposedAction();
}

void MainWindow::navigateToRandomImage()
{
    if (m_imagePaths.isEmpty())
        return;

    // Generate random index using secure random number generation
    int randomIndex = QRandomGenerator::global()->bounded(m_imagePaths.size());

    // Delegate to image viewer for centering operation
    m_imageViewer->centerOnImageIndex(randomIndex);

    // Update status bar
    statusBar()->showMessage(QString("Image %1 of %2").arg(randomIndex + 1).arg(m_imagePaths.size()));
}

void MainWindow::toggleSlideshow()
{
    if (!m_slideshowTimer) {
        m_slideshowTimer = new QTimer(this);
        // Modern signal/slot connection syntax
        connect(m_slideshowTimer, &QTimer::timeout,
                this, &MainWindow::slideshowAdvance);
    }

    m_slideshowActive = !m_slideshowActive;

    if (m_slideshowActive) {
        m_slideshowTimer->start(m_slideshowInterval);
        statusBar()->showMessage("Slideshow started");
    } else {
        m_slideshowTimer->stop();
        statusBar()->showMessage("Slideshow stopped");
    }
}

void MainWindow::slideshowAdvance()
{
    // Advance to next image
    m_imageViewer->centerOnNextImage();
}

void MainWindow::setSlideshowInterval()
{
    bool ok;
    int interval = QInputDialog::getInt(this, "Slideshow Timing",
                                        "Interval in seconds:",
                                        m_slideshowInterval / 1000, 1, 60, 1, &ok);
    if (ok) {
        m_slideshowInterval = interval * 1000;
        if (m_slideshowActive) {
            m_slideshowTimer->setInterval(m_slideshowInterval);
        }
        statusBar()->showMessage(QString("Slideshow interval set to %1 seconds").arg(interval));
    }
}

void MainWindow::updateImageInfo(int index)
{
    if (index < 0 || index >= m_imagePaths.size()) {
        m_imageInfoLabel->setText("");
        return;
    }

    QString imagePath = m_imagePaths[index];
    QFileInfo fileInfo(imagePath);
    QImageReader reader(imagePath);
    QSize dimensions = reader.size();

    // Format file size
    qint64 bytes = fileInfo.size();
    QString sizeText;

    if (bytes > 1024*1024) {
        sizeText = QString::number(bytes/(1024.0*1024.0), 'f', 2) + " MB";
    } else if (bytes > 1024) {
        sizeText = QString::number(bytes/1024.0, 'f', 2) + " KB";
    } else {
        sizeText = QString::number(bytes) + " bytes";
    }

    // Create info text
    QString infoText = QString("%1 (%2) | %3x%4 | %5")
                           .arg(fileInfo.fileName())
                           .arg(fileInfo.suffix().toUpper())
                           .arg(dimensions.width())
                           .arg(dimensions.height())
                           .arg(sizeText);

    m_imageInfoLabel->setText(infoText);
}

void MainWindow::showKeyboardShortcuts()
{
    // Create shortcuts dialog
    QDialog dialog(this);
    dialog.setWindowTitle("Keyboard Shortcuts");
    dialog.setMinimumWidth(400);

    // Create layout
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // Create shortcuts table
    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << "Shortcut" << "Action");
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setAlternatingRowColors(true);

    // Add shortcuts to table
    const QList<QStringList> shortcuts = {
        {"Left Arrow", "Previous Image"},
        {"Right Arrow", "Next Image"},
        {"Space", "Next Image"},
        {"Shift + Up Arrow", "Zoom In (10%)"},
        {"Shift + Down Arrow", "Zoom Out (10%)"},
        {"Shift + Mouse Wheel", "Zoom In/Out"},
        {"Left Mouse Button Drag", "Pan (when zoomed)"},
        {"Shift + Right Arrow", "Random Image"},
        {"F5", "Toggle Slideshow"},
        {"L", "Rotate Left"},
        {"R", "Rotate Right"},
        {"F", "Toggle Current Image as Favorite"},
        {"Middle-Click", "Toggle Current Image as Favorite"},
        {"Ctrl + F", "Toggle Favorites Mode"},
        {"F1", "Show Keyboard Shortcuts"}
    };
    table->setRowCount(shortcuts.size());

    for (int i = 0; i < shortcuts.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(shortcuts[i][0]));
        table->setItem(i, 1, new QTableWidgetItem(shortcuts[i][1]));
    }

    // Add table to layout
    layout->addWidget(table);

    // Add close button
    QPushButton *closeButton = new QPushButton("Close", &dialog);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(closeButton);

    // Show dialog
    dialog.exec();
}

void MainWindow::toggleFavoritesMode()
{
    m_imageViewer->toggleFavoritesMode();

    // Update menu checkstate
    QList<QAction*> actions = menuBar()->findChild<QMenu*>("&Favorites")->actions();
    for (QAction* action : actions) {
        if (action->text() == "&Show Only Favorites") {
            action->setChecked(m_imageViewer->isShowingOnlyFavorites());
            break;
        }
    }
}

void MainWindow::toggleCurrentImageFavorite()
{
    m_imageViewer->toggleCurrentImageFavorite();
}
