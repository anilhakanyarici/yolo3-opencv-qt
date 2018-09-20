#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Capture.h"
#include "Yolo.h"

#include <QFileDialog>
#include <QMainWindow>
#include <QTimer>
#include <QTextStream>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    VideoCapture *_capturer = nullptr;
    Yolo *_yolo = nullptr;
    QTimer *_timer;
    bool _isOnDetection;

    QThread *_opencv_read_thread;
    QTextStream _extractStream;
    QFile _extractFile;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void browseClicked();
    void detectClicked();
    void extractClicked();

    void timerTick();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
