#ifndef BEHAVIORLABEL_H
#define BEHAVIORLABEL_H

#include <QMainWindow>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>
#include <QMouseEvent>
#include <QPainter>
#include <QInputDialog>

namespace Ui {
class BehaviorLabel;
}

class BehaviorLabel : public QMainWindow
{
    Q_OBJECT

public:
    explicit BehaviorLabel(QWidget *parent = 0);
    ~BehaviorLabel();

private slots:
    void on_actionOpen_Folder_triggered();

    void on_BtnSave_clicked();

    void on_BtnPrev_clicked();

    void on_BtnNext_clicked();

    void on_BtnJump_clicked();

    void on_pushButton_clicked();

    void on_BtnOpenLabelFile_clicked();

    void on_BtnLabelStart_clicked();

    void on_pushButton_2_clicked();

    void on_BtnCrop_clicked();

    void on_BtnNextFrame_clicked();

    void on_BtnPrevFrame_clicked();

private:
    Ui::BehaviorLabel *ui;

    QString strFolderName;

    int sampleCounter;

    QImage oriImgQT;
    QImage displayImgQT;

    QFile *dataFile;
    QTextStream textStream;

    QFile *labelFile;
    QTextStream labelStream;

    QFile *fullFile;
    QTextStream fullStream;

    QString originalLabelFileName;

    bool firstimeopen;

    int crop_x;
    int crop_y;
    int crop_width;
    int crop_height;

//mouse event
    void mouseReleaseEvent(QMouseEvent *ev);
    int mouseCounter;
    int nose_x;
    int nose_y;
    int neck_x;
    int neck_y;
    int tail_x;
    int tail_y;
    int hs_x;
    int hs_y;

    //for color
    int mouseCounter_color;
    int joint_x[9];
    int joint_y[9];
    QString joint_name[9];

    QFile *pointFile;
    QTextStream pointStream;

    int pointEnabled;
    //0 - disabled
    //1 - depth
    //2 - color

    bool readImageFromFolder(int sampleCounter_);
};

#endif // BEHAVIORLABEL_H
