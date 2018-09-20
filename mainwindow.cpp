#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->_yolo = new Yolo();
    this->_yolo->loadNames("/home/anil/darknet/data/coco.names");
    this->_yolo->setClasses(80);
    this->_yolo->loadNetwork("/home/anil/darknet/cfg/yolov3.cfg", "/home/anil/darknet/yolov3.weights");

    this->_timer = new QTimer();
    QObject::connect(this->_timer, SIGNAL(timeout()), this, SLOT(timerTick()));
    this->_isOnDetection = false;
}

MainWindow::~MainWindow()
{
    if(this->_capturer != nullptr)
        delete this->_capturer;
    delete this->_timer;
    delete this->_yolo;
    delete ui;
}

void MainWindow::browseClicked(){
    if(this->_isOnDetection){
        QMessageBox::warning(this, "Uyarı", "Obje algılayıcı zaten çalışıyor!");
        return;
    }
    QFileDialog dialog(this, "Select a Video File", QDir::home().path(), tr("MPEG4 (*.mpeg4 *.mp4);; AVI (*.avi);; All files (*.*)"));
    dialog.setFileMode(QFileDialog::ExistingFile);
    if(dialog.exec()) {
        QString filename = dialog.selectedFiles()[0];
        this->ui->browseLineEdit->setText(filename);
        if(this->_capturer != nullptr)
            delete this->_capturer;
        this->_capturer = new VideoCapture(filename);
        this->_timer->setInterval(1000.0 / this->_capturer->getFPS());
        this->ui->detectProgress->setValue(0);
    }
}

void MainWindow::detectClicked()
{
    if(this->_isOnDetection){
        this->_timer->stop();
        this->ui->detectButton->setText("Detect");
        this->_isOnDetection = false;
        this->ui->videoLabel->setPixmap(QPixmap());
        this->ui->detectProgress->setValue(0);
        this->_capturer->reset();
        return;
    } else {
        this->_timer->start();
        this->ui->detectButton->setText("Stop");
        this->_isOnDetection = true;
    }
}

void MainWindow::extractClicked()
{
    if(this->_isOnDetection){
        QMessageBox::warning(this, "Uyarı", "Obje algılayıcı zaten çalışıyor!");
        return;
    }
    QFileDialog dialog(this, "Save Metadata", QDir::home().path(), tr("Text (*.txt)"));
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if(dialog.exec()){
        QString textname = dialog.selectedFiles()[0];
        this->ui->extractLineEdit->setText(textname);
        this->_extractFile.setFileName(textname);
        this->_extractFile.open(QIODevice::WriteOnly);
    }
}

void MainWindow::timerTick()
{
    if(this->_capturer->frame() >= this->_capturer->frameCount()){
        this->_timer->stop();
        this->ui->detectButton->setText("Detect");
        this->_isOnDetection = false;
        this->ui->videoLabel->setPixmap(QPixmap());
        QMessageBox::information(this, "Title", "Metadata çıkartma tamamlandı.");
        return;
    }

    if(this->_capturer->readNext()){
        QImage qim = this->_capturer->getCurrentFrame();
        QVector<Detection> dets = this->_yolo->detect(&qim);
        QString frameline = QString("%1%2/%3\n").arg("Frame: ", QString::number(this->_capturer->frame()), QString::number(this->_capturer->frameCount()));
        for(int i = 0; i < dets.size(); i++){
            frameline.append(dets[i].toString());
            frameline.append("\n");
        }
        frameline.append("\n");
        this->ui->metadataTextEdit->append(frameline);
        if(this->_extractFile.isOpen()) {
            QTextStream ts(&this->_extractFile);
            ts << frameline;
            ts.flush();
        }
        QImage drawed = this->_yolo->paintDetections(qim, dets);
        drawed = drawed.scaled(this->ui->videoLabel->size());
        this->ui->videoLabel->setPixmap(QPixmap::fromImage(drawed));
        int progress = (int)(100.0 * this->_capturer->frame() / this->_capturer->frameCount());
        this->ui->detectProgress->setValue(progress);
    }
}
