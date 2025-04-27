// ImageLoader.h
#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QObject>
#include <QString>
#include <QPixmap>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
// Lines 10-55: Complete ImageLoadTask and ImageLoader class declarations
class ImageLoadTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    ImageLoadTask(int index, const QString &path);
    ~ImageLoadTask() override = default;

    void run() override;

    int index() const { return m_index; }
    QPixmap pixmap() const { return m_pixmap; }

signals:
    void loadCompleted(int index, const QPixmap &pixmap);

private:
    int m_index;
    QString m_path;
    QPixmap m_pixmap;
};

class ImageLoader : public QObject
{
    Q_OBJECT

public:
    explicit ImageLoader(QObject *parent = nullptr);
    ~ImageLoader();

    void loadImage(int index, const QString &path);

signals:
    void imageLoaded(int index, const QPixmap &pixmap);

private:
    QThreadPool m_threadPool;
    QMutex m_mutex;
};
#endif // IMAGELOADER_H
