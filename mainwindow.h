// MainWindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QVector>
#include <QPushButton>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <qmessagebox.h>
#include "imageviewer.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openDirectory();
    void navigateToRandomImage();
    void toggleSlideshow();
    void slideshowAdvance();
    void setSlideshowInterval();
    void showKeyboardShortcuts();
    void toggleFavoritesMode();  // ADD THIS LINE
    void toggleCurrentImageFavorite();  // ADD THIS LINE
    // Lines 15-22: MainWindow.h - Add to private member variables
private:
    void setupUI();
    void loadImagesFromDirectory(const QString &dirPath);
    void updateImageInfo(int index);  // ADD THIS LINE

    ImageViewer *m_imageViewer;
    QVector<QString> m_imagePaths;
    QTimer *m_slideshowTimer;        // ADD THIS LINE
    int m_slideshowInterval = 3000;  // ADD THIS LINE (3 seconds default)
    bool m_slideshowActive = false;  // ADD THIS LINE
    QLabel *m_imageInfoLabel;         // ADD THIS LINE


    // MainWindow.h - ADD dragEnterEvent and dropEvent declarations to MainWindow class
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};

#endif // MAINWINDOW_H
