#include "Capture.h"

#include <opencv2/opencv.hpp>

cv::Mat3b QImage2Mat(const QImage &src) {
    unsigned int height = src.height();
    unsigned int width = src.width();
    
    cv::Mat3b dest(height, width);
    for (unsigned int y = 0; y < height; ++y) {
        cv::Vec3b *destrow = dest[y];
        for (unsigned int x = 0; x < width; ++x) {
            QRgb pxl = src.pixel(x, y);
            destrow[x] = cv::Vec3b(qBlue(pxl), qGreen(pxl), qRed(pxl));
        }
    }
    return dest;
}

QThread *ICapturer::beginReadNext() {
    ReadThread *rt = new ReadThread(this);
    rt->start();
    return rt;
}

bool ICapturer::endReadNext(QThread *thrd) {
    ReadThread *rt = (ReadThread *)thrd;
    rt->wait();
    bool ret = rt->result;
    delete rt;
    return ret;
}


struct VideoCapture::pimpl
{
    cv::VideoCapture *_cap = nullptr;
    QImage _currentFrame;
    long _frame;
};

VideoCapture::VideoCapture(const QString &file)
{
    this->_pimpl = new pimpl();
    this->_pimpl->_frame = 0;
    QByteArray utf8 = file.toUtf8();
    this->_pimpl->_cap = new cv::VideoCapture(utf8.data());
}

VideoCapture::~VideoCapture()
{
    delete this->_pimpl->_cap;
    delete this->_pimpl;
}

QImage VideoCapture::getCurrentFrame()
{
    return this->_pimpl->_currentFrame;
}

bool VideoCapture::readNext()
{
    cv::Mat mat;
    if(!this->_pimpl->_cap->read(mat))
        return false;
    cv::cvtColor(mat, mat, CV_BGR2RGB);
    this->_pimpl->_currentFrame = QImage((uchar*) mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);

    this->_pimpl->_frame++;
    return true;
}

long VideoCapture::frame()
{
    return this->_pimpl->_frame;
}

long VideoCapture::frameCount() const
{
    return (long)this->_pimpl->_cap->get(CV_CAP_PROP_FRAME_COUNT);
}

int VideoCapture::getFPS() const
{
    return (int)this->_pimpl->_cap->get(CV_CAP_PROP_FPS);
}

void VideoCapture::reset()
{
    this->_pimpl->_cap->set(CV_CAP_PROP_POS_AVI_RATIO, 0);
    this->_pimpl->_frame = 0;
}

struct CameraCapture::pimpl{
    cv::VideoCapture *_cap = nullptr;
    QImage _currentFrame;
    long _frame;
};

CameraCapture::CameraCapture(qint32 cam_index, qint32 w, qint32 h, qint32 fps) {
    this->_pimpl = new CameraCapture::pimpl();
    this->_pimpl->_frame = -1;
    this->_pimpl->_cap = new cv::VideoCapture(cam_index);
    this->_pimpl->_cap->set(CV_CAP_PROP_FRAME_WIDTH, (int)w);
    this->_pimpl->_cap->set(CV_CAP_PROP_FRAME_HEIGHT, (int)h);
    this->_pimpl->_cap->set(CV_CAP_PROP_FPS, (int)fps);
}

CameraCapture::~CameraCapture()
{
    delete this->_pimpl->_cap;
    delete this->_pimpl;
}

QImage CameraCapture::getCurrentFrame()
{
    return this->_pimpl->_currentFrame;
}

bool CameraCapture::readNext()
{
    cv::Mat mat;
    if(!this->_pimpl->_cap->read(mat))
        return false;
    cv::cvtColor(mat, mat, CV_BGR2RGB);
    this->_pimpl->_currentFrame = QImage((uchar*) mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);

    this->_pimpl->_frame++;
    return true;
}

long CameraCapture::frame()
{
    return this->_pimpl->_frame;
}

