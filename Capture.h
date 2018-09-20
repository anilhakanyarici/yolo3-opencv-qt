#ifndef ICAPTURER_H
#define ICAPTURER_H

#include <QImage>
#include <QString>
#include <QThread>

class ICapturer {
    class ReadThread : public QThread { //beginReadNext() & endReadNext() Microsoft'un APM (Asynchronous Programming Model) tasarım kalıbına benzer şekilde yazıldı.
        ICapturer *_capturer;
    public:
        bool result;
        ReadThread(ICapturer *capturer) { this->_capturer = capturer; }
    protected:
        void run() override { this->result = this->_capturer->readNext(); }
    };
public:
    virtual ~ICapturer() {}

    QThread *beginReadNext();
    bool endReadNext(QThread *thrd);

    virtual QImage getCurrentFrame() = 0;
    virtual bool readNext() = 0;
    virtual long frame() = 0;
};

class VideoCapture : public ICapturer {
    struct pimpl;
    pimpl *_pimpl;

public:
    VideoCapture(const QString &file);
    ~VideoCapture();

    QImage getCurrentFrame() override;
    bool readNext() override;
    long frame() override;
    long frameCount() const;
    int getFPS() const;
    void reset();
};

class CameraCapture : public ICapturer {
    struct pimpl;
    pimpl *_pimpl;

public:
    CameraCapture(qint32 cam_index, qint32 w = 0, qint32 h = 0, qint32 fps = 0);
    ~CameraCapture();

    QImage getCurrentFrame() override;
    bool readNext() override;
    long frame() override;
};

#endif // ICAPTURER_H
