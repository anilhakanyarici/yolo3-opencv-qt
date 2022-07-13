#include "Yolo.h"
extern "C" {
#include <darknet.h>
}

#include <QImage>
#include <QTextStream>
#include <QBuffer>
#include <QFile>
#include <QPainter>
#include <QtMath>

void QImage_In2_Image(QImage *src, image im) {

    int h = src->height();
    int w = src->width();
    int c = 3 + (int)src->hasAlphaChannel();

    unsigned char *data = (unsigned char *)src->scanLine(0);
    int step =  w * c;

    for(int i = 0; i < h; ++i) {
        for(int j = 0; j < w; ++j) {
            for(int k = 0; k < c; ++k) {
                im.data[k * w * h + i * w + j] = data[i * step + j * c + k] / 255.0;
            }
        }
    }
}
image QImage_2_Image(QImage *src) {
    int h = src->height();
    int w = src->width();
    int c = 3 + (int)src->hasAlphaChannel();
    image im;
    im.h = h;
    im.w = w;
    im.c = c;
    im.data = (float *)malloc(h * w * c * sizeof(float));
    QImage_In2_Image(src, im);
    return im;
}

struct Yolo::pimpl {
    network *_net = nullptr;
    float _thresh = 0.5;
    float _hier = 0.5;
    float _nms = 0.4;
    int _classes = 0;
    QStringList _names;
};

int Yolo::getBatch() const { return (this->_pimpl->_net == nullptr) ? -1 : get_current_batch(this->_pimpl->_net); }
void Yolo::setBatch(const int &batch) { if(this->_pimpl->_net != nullptr) set_batch_network(this->_pimpl->_net, batch); }
float Yolo::getThresh() const { return this->_pimpl->_thresh; }
void Yolo::setThresh(const float &t) { this->_pimpl->_thresh = t; }
float Yolo::getHier() const { return this->_pimpl->_hier; }
void Yolo::setHier(const float &h) { this->_pimpl->_hier = h; }
float Yolo::getClasses() const { return this->_pimpl->_classes; }
void Yolo::setClasses(const float &c) { this->_pimpl->_classes = c; }
float Yolo::getNMS() const { return this->_pimpl->_nms; }
void Yolo::setNMS(const float &nms) { this->_pimpl->_nms = nms; }

Yolo::Yolo()
{
    this->_pimpl = new Yolo::pimpl();
}
Yolo::~Yolo()
{
    if(this->_pimpl != nullptr){
        free_network(this->_pimpl->_net);
    }
    delete this->_pimpl;
}

void Yolo::loadNetwork(const QString &cfgfile, const QString &weightfile, qint32 clear)
{
    QByteArray cf_utf8 = cfgfile.toUtf8();
    QByteArray wf_utf8 = weightfile.toUtf8();

    this->_pimpl->_net = load_network(cf_utf8.data(), wf_utf8.data(), (int)clear);
    set_batch_network(this->_pimpl->_net, 1);
}

bool Yolo::loadNames(const QString &namesfile)
{
    QFile file(namesfile);
    if(!file.open(QIODevice::ReadOnly))
        return false;
    QTextStream ts(&file);
    while(!ts.atEnd()){
        QString line = ts.readLine();
        this->_pimpl->_names.append(line);
    }
    file.close();
    return true;
}

QString Detection::labelString() const
{
    QString name = "";
    for(int i = 0; i < this->names.size() - 1; i++){
        name.append(this->names[i]);
        name.append(": %");
        name.append(QString::number(qRound(100 * this->probabilites[i])));
        name.append(", ");
    }
    name.append(this->names[this->names.size() - 1]);
    name.append(": %");
    name.append(QString::number(qRound(100 * this->probabilites[this->names.size() - 1])));
    return name;
}

QVector<Detection> Yolo::detect(QImage *fr)
{
    int w = fr->width();
    int h = fr->height();
    image im = QImage_2_Image(fr); //13ms => i7 4790k (No OC)
    image letter_im = letterbox_image(im, this->_pimpl->_net->w, this->_pimpl->_net->h);
    network_predict(this->_pimpl->_net, letter_im.data);
    free_image(im);
    free_image(letter_im);

    int boxes_count = 0;
    detection *dets = get_network_boxes(this->_pimpl->_net, im.w, im.h, this->_pimpl->_thresh, this->_pimpl->_hier, 0, 1, &boxes_count);
    do_nms_obj(dets, boxes_count, this->_pimpl->_classes, this->_pimpl->_nms); //Neden thresh (demo) ile bununki (nms) aynı değil?

    QVector<Detection> detections;
    for(int i = 0; i < boxes_count; ++i){
        Detection det;
        for(int j = 0; j < this->_pimpl->_classes; ++j) {
            if (dets[i].prob[j] > this->_pimpl->_thresh) {
                det.names.append(this->_pimpl->_names[j]);
                det.probabilites.append(dets[i].prob[j]);
            }
        }
        if(det.names.size() > 0){
            det.objectness = dets[i].objectness;
            box b = dets[i].bbox;
            int rw = qRound(b.w * w);
            int rh = qRound(b.h * h);
            det.rect.setRect(b.x * w - rw / 2, b.y * h - rh / 2, rw, rh);
            detections.append(det);
        }
    }

    free_detections(dets, boxes_count);

    return detections;
}

void Yolo::paintDetections(QImage *image, const QVector<Detection> &detections){
    QPainter painter(image);
    QPen pen = painter.pen();
    pen.setWidth(4);
    painter.setPen(pen);
    for(int i = 0; i < detections.size(); i++){
        QRectF rect = detections[i].rect;
        pen.setColor(QColor::fromRgb(255, 255, 0));
        painter.setPen(pen);
        painter.drawRect(rect);
        QFont font = painter.font();
        font.setWeight(QFont::Thin);
        font.setPixelSize(24);
        QFontMetrics fm(font);
        QString label = detections[i].labelString();
        fm.horizontalAdvance(label);
        int hd = fm.height();
        painter.setFont(font);
        pen.setColor(QColor::fromRgb(0, 255, 255));
        painter.setPen(pen);
        painter.drawText(rect.x(), rect.y() - hd / 2, label);
    }

    painter.end();
}

QImage Yolo::paintDetections(const QImage &image, const QVector<Detection> &detections)
{
    QImage im = image.copy();
    this->paintDetections(&im, detections);
    return im;
}

QString Detection::toString()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QTextStream ts(&buffer);
    ts << this->names[0];
    for(int i = 1; i < this->names.size(); i++)
        ts << ", " << this->names[i];

    ts << " => Probabilities: ";
    ts << this->probabilites[0];
    for(int i = 1; i < this->probabilites.size(); i++)
        ts << ", " << this->probabilites[i];

    ts << " Rect: X=" << this->rect.x();
    ts << " Y=" << this->rect.y();
    ts << " W=" << this->rect.width();
    ts << " H=" << this->rect.height();

    ts << " objness: " << this->objectness;

    ts.flush();

    buffer.seek(0);
    QString str = ts.readAll();
    buffer.close();
    return str;
}


