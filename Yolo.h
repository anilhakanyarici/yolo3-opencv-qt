#ifndef YOLO_H
#define YOLO_H

#include <QStringList>
#include <QRectF>
#include <QVector>
#include <QThread>


struct Detection {
    QStringList names;
    QVector<float> probabilites;
    QRectF rect;
    float objectness;

    QString toString();
    QString labelString() const;
};

class IDetector {
    class DetectThread : public QThread {
        IDetector *_detector;
        QImage *_fr;
        QVector<Detection> _return;
    public:
        DetectThread(IDetector *detector, QImage *fr, QObject *parent = nullptr) : QThread(parent) {
            this->_detector = detector;
            this->_fr = fr;
        }
        QVector<Detection> returnValue() { return this->_return; }
    protected:
        void run() override {
            this->_return = this->_detector->detect(this->_fr);
        }
    };

public:
    virtual QVector<Detection> detect(QImage *fr) = 0;
    virtual ~IDetector(){}

    QThread *beginDetect(QImage *fr, QThread::Priority priority = QThread::InheritPriority){
        DetectThread *dt = new DetectThread(this, fr);
        dt->start(priority);
        return dt;
    }
    QVector<Detection> endDetect(QThread *th) {
        DetectThread *dt = dynamic_cast<DetectThread *>(th);
        if(dt == nullptr)
            qFatal("%s", "Hatalı apm parametresi.");
        dt->wait();
        QVector<Detection> ret = dt->returnValue();
        delete dt;
        return ret;
    }
};

class Yolo : public IDetector
{

private:
    struct pimpl;
    pimpl *_pimpl;

public: //Properties
    int getBatch() const;
    void setBatch(const int &batch);

    float getThresh() const;
    void setThresh(const float &t);

    float getHier() const;
    void setHier(const float &h);

    float getClasses() const;
    void setClasses(const float &c);

    float getNMS() const;
    void setNMS(const float &nms);

public:
    Yolo();
    ~Yolo();

    void loadNetwork(const QString &cfgfile, const QString &weightfile, qint32 clear = 0);
    bool loadNames(const QString &namesfile);

    QVector<Detection> detect(QImage *fr) override;

    void paintDetections(QImage *image, const QVector<Detection> &detections);
    QImage paintDetections(const QImage &image, const QVector<Detection> &detections);
};

#endif // YOLO_H
