/*
    Behavior Labelling GUI for rat/mouse
    @author: pheno
    +Generate full truth label from original files
    using qimage instead of opencv cv::Mat
    no opencv library used
    support both depth & color frames

    added clue points annotation (nose, neck, tail, plus others)
    label model
          Depth - 4 points
          Color - 7 points with limb lines (remove headstage & tail tip)
    added cropping if the input image is larger than label size
    Currently working on:
        Changeable JUMPSTEP
*/

#include "behaviorlabel.h"
#include "ui_behaviorlabel.h"

#define JUMPSTEP 15

#define LAB_WIDTH 800
#define LAB_HEIGHT 600

BehaviorLabel::BehaviorLabel(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::BehaviorLabel)
{
    ui->setupUi(this);

    ui->BtnSave->setDisabled(true);

    ui->sysLog->setReadOnly(true);

    ui->BtnLabelStart->setDisabled(true);

    ui->pushButton_2->setDisabled(true);

    sampleCounter = 1;
    //LCD display settings
    ui->lcdCounter->setSegmentStyle(QLCDNumber::Flat);
    QPalette lcdpat = ui->lcdCounter->palette();
    lcdpat.setColor(QPalette::Normal,QPalette::WindowText,QColor(138,43,226));
    ui->lcdCounter->setPalette(lcdpat);
    ui->lcdCounter->setStyleSheet("background-color: honeydew");
    ui->lcdCounter->display(sampleCounter);

    //Set lineEdit_Jump to only accept int (0~99999)
    ui->lineEdit_Jump->setValidator(new QIntValidator(0,99999,this));
    ui->lineEdit_cropx->setValidator(new QIntValidator(0,9999,this));
    ui->lineEdit_cropy->setValidator(new QIntValidator(0,9999,this));

    firstimeopen = true;
    pointEnabled = 0;
    mouseCounter = 0;
    mouseCounter_color = 0;

    ui->imgLab->setAlignment(Qt::AlignTop | Qt::AlignLeft); //set image alignment to top & left
    crop_x = crop_y = 0;

    for (int i = 0; i < 9; i++) {
        joint_x[i] = 0;
        joint_y[i] = 0;
    }

    joint_name[0] = tr("Nose");
    joint_name[1] = tr("Neck");
    joint_name[2] = tr("Tail Base");
    joint_name[3] = tr("Forelimb - Left");
    joint_name[4] = tr("Forelimb - Right");
    joint_name[5] = tr("Leg - Left");
    joint_name[6] = tr("Leg - Right");
    joint_name[7] = tr("Tail Tip");
    joint_name[8] = tr("Head-stage");
}

BehaviorLabel::~BehaviorLabel()
{
    delete ui;
}

void BehaviorLabel::on_actionOpen_Folder_triggered()
{
    //strFolderName = QFileDialog::getExistingDirectory(this,tr("Open Directory"),".",QFileDialog::ShowDirsOnly
              //                                              | QFileDialog::DontResolveSymlinks);
    //strFolderName = QString("D:/KinectData/20160824/V13_40_12");
    //strFolderName = QString("D:/KinectData/20160824/V13_51_51");
    strFolderName = QString("D:/KinectData/Y Maze/DeepRat/2017_11_17_14_50_58_Color");
    //strFolderName = QString("D:/KinectData/20171011/D2");
    //Display Source Path
    if(strFolderName.isEmpty()) {
        //QMessageBox::about(this,tr("Warning"),tr("Empty Path!"));
        return;
    } else {
        bool readOp = readImageFromFolder(sampleCounter);
        if (!readOp) {
            QMessageBox::about(this,tr("Warning"),tr("Folder Not Valid!"));
            return;
        } else {
            ui->dirLab->setText(strFolderName);
            //imshow("Depth Image",oriImg);
            //display in Qimage format
            //ui->imgLab->setPixmap(QPixmap::fromImage(oriImgQT));
            if ((oriImgQT.width() > LAB_WIDTH) || (oriImgQT.height() > LAB_HEIGHT)) {
                ui->sysLog->append("Image Width: "+QString::number(oriImgQT.width())
                                    +" Height: "+QString::number(oriImgQT.height())+" Cropped");
            } else {
                ui->sysLog->append("Image Width: "+QString::number(oriImgQT.width())
                                    +" Height: "+QString::number(oriImgQT.height()));
            }
            if (!ui->BtnSave->isEnabled()) {
                ui->BtnSave->setEnabled(true);
            }

            if (firstimeopen) {
                firstimeopen = false;
            } else {
                dataFile->close();
                pointFile->close();
            }

            QString dataFileName = QString("label_%1.txt").arg(QTime::currentTime().toString("hh_mm_ss"));
            QString pointFileName = QString("point_%1.txt").arg(QTime::currentTime().toString("hh_mm_ss"));

            dataFile = new QFile(dataFileName);
            pointFile = new QFile(pointFileName);

            if (dataFile->open(QIODevice::WriteOnly | QFile::Text)) {
                textStream.setDevice(dataFile);
                //textStream << "Label file opened!" << endl;
            }
            if (pointFile->open(QIODevice::WriteOnly | QFile::Text)) {
                pointStream.setDevice(pointFile);
                //pointStream << "Point file opened!" << endl;
            }

            ui->sysLog->append("Open new data successfully.");
            ui->pushButton_2->setEnabled(true);
        }
    }
}

void BehaviorLabel::on_BtnSave_clicked()
{
    int bodypose = 0;
    int headpose = 0;

    if (ui->radioButton->isChecked())
        bodypose = 1;
    else if (ui->radioButton_2->isChecked())
        bodypose = 2;
    else if (ui->radioButton_3->isChecked())
        bodypose = 3;
    else if (ui->radioButton_4->isChecked())
        bodypose = 4;

    if (ui->radioHead_1->isChecked())
        headpose = 1;
    else if (ui->radioHead_2->isChecked())
        headpose = 2;
    else if (ui->radioHead_3->isChecked())
        headpose = 3;
    else if (ui->radioHead_4->isChecked())
        headpose = 4;

    textStream << sampleCounter << "\t" << bodypose << "\t" << headpose << endl;
    ui->sysLog->append(QString("Frame %1").arg(sampleCounter, 6, 10, QLatin1Char('0'))
                       +"\t"+QString::number(bodypose)
                       +" "+QString::number(headpose));
}

void BehaviorLabel::on_BtnPrev_clicked()
{
    bool readOp = readImageFromFolder(sampleCounter-JUMPSTEP);
    if (!readOp) {
        QMessageBox::about(this,tr("Warning"),tr("Already at the first frame!"));
        return;
    } else {
        sampleCounter-=JUMPSTEP;
        ui->lcdCounter->display(sampleCounter);
    }
}

void BehaviorLabel::on_BtnNext_clicked()
{
    bool readOp = readImageFromFolder(sampleCounter+JUMPSTEP);
    if (!readOp) {
        QMessageBox::about(this,tr("Warning"),tr("Already at the last frame!"));
        return;
    } else {
        sampleCounter+=JUMPSTEP;
        ui->lcdCounter->display(sampleCounter);
    }
}

void BehaviorLabel::on_BtnJump_clicked()
{
    int tmp = ui->lineEdit_Jump->text().toInt();

    bool readOp = readImageFromFolder(tmp);

    if (!readOp) {
        QMessageBox::about(this,tr("Warning"),tr("Invalid frame number!"));
        return;
    } else {
        sampleCounter = tmp;
        ui->lcdCounter->display(sampleCounter);
    }
}

void BehaviorLabel::on_pushButton_clicked()
{
    ui->sysLog->clear();
}

void BehaviorLabel::on_BtnOpenLabelFile_clicked()
{
    QString labelFileName = QFileDialog::getOpenFileName(this,tr("Open Data File"),".",tr("Data File (*.txt)"));

    if(labelFileName.isEmpty()) {
        //QMessageBox::about(this,tr("Warning"),tr("Empty Path!"));
        return;
    } else {
        if (!ui->BtnLabelStart->isEnabled())
            ui->BtnLabelStart->setEnabled(true);

        labelFile = new QFile(labelFileName);

        if (labelFile->open(QIODevice::ReadOnly | QFile::Text)) {
            labelStream.setDevice(labelFile);
        } else {
            QMessageBox::about(this,tr("Warning"),tr("Path Not Valid!"));
            return;
        }
        QFileInfo fileinfo = QFileInfo(labelFileName);
        originalLabelFileName = fileinfo.fileName();
        ui->sysLog->append("Open original label data successfully.");
        ui->sysLog->append(QString("file opened: %1").arg(originalLabelFileName));
    }
}

void BehaviorLabel::on_BtnLabelStart_clicked()
{
    QString fullFileName = QString("full_%1").arg(originalLabelFileName);

    fullFile = new QFile(fullFileName);

    if (fullFile->open(QIODevice::WriteOnly | QFile::Text)) {
        fullStream.setDevice(fullFile);
    }

    ui->sysLog->append("Start converting...");

    //read first line and initialize
    int curFrame, curType;
    int tmpFrame, tmpType;

    labelStream >> curFrame >> curType;
    fullStream << curFrame << "\t" << curType << endl;

    while (!labelStream.atEnd()) {
        labelStream >> tmpFrame >> tmpType;
        //generate labels in between
        if (tmpType == curType) {
            for (int i = curFrame + 1; i < tmpFrame; i++) {
                fullStream << i << "\t" << curType << endl;
            }
        } else {
            int midFrame = (curFrame + tmpFrame + 1) / 2;
            for (int i = curFrame + 1; i < midFrame; i++) {
                fullStream << i << "\t" << curType << endl;
            }
            for (int i = midFrame; i < tmpFrame; i++) {
                fullStream << i << "\t" << tmpType << endl;
            }
        }
        fullStream << tmpFrame << "\t" << tmpType << endl;
        curFrame = tmpFrame;
        curType = tmpType;
    }

    fullFile->close();
    ui->sysLog->append("Conversion finished.");
}

void BehaviorLabel::on_pushButton_2_clicked()
{
    if (pointEnabled == 0) {
        pointEnabled ++;
        ui->lab_point->setText("Point Annotation: Depth");
        ui->pushButton_2->setText("Swith to Color");
    } else if (pointEnabled == 1){
        pointEnabled ++;
        ui->lab_point->setText("Point Annotation: Color");
        ui->pushButton_2->setText("Disable");
    } else {
        pointEnabled = 0;
        ui->lab_point->setText("Point Annotation: Disabled");
        ui->pushButton_2->setText("Enable");
    }
}

void BehaviorLabel::mouseReleaseEvent(QMouseEvent *ev)
{
    if (pointEnabled == 0)
        return;
    //get mouse postion and check if in image range
    //int xx = ev->x();
    //int yy = ev->y();
    //ui->sysLog->append("ev-> : "+QString::number(xx)+' '+QString::number(yy));
    //xx = xx - ui->imgLab->pos().x();
    //yy = yy - ui->imgLab->pos().y() - ui->menuBar->height();

    //use default mapping function
    QPoint glo = ev->globalPos();
    //ui->sysLog->append("Glob : "+QString::number(glo.x())+' '+QString::number(glo.y()));
    QPoint mapped_pos = ui->imgLab->mapFromGlobal(glo);
    int xx = mapped_pos.x();
    int yy = mapped_pos.y();
    //ui->sysLog->append("Glob Map: "+QString::number(mapped_pos.x())+' '+QString::number(mapped_pos.y()));

    if (pointEnabled == 1) {
        if ((xx>=0)&&(yy>=0) && (xx<=ui->imgLab->width()) && (yy<=ui->imgLab->height())){
            //depth image
            mouseCounter++;

            //draw the point on image
            QPainter painter(&displayImgQT);    //oriImgQT may be color or grayscale
            QPen pen;
            pen.setWidth(2);
            if (mouseCounter <= 3) {
                pen.setColor(Qt::red);
            } else {
                pen.setColor(Qt::blue);
            }
            painter.setPen(pen);
            painter.drawPoint(xx,yy);
            painter.end();
            ui->imgLab->setPixmap(QPixmap::fromImage(displayImgQT));

            if (mouseCounter == 1) {
                nose_x = xx;
                nose_y = yy;
                ui->sysLog->append("Nose: "+QString::number(xx)+' '+QString::number(yy));
            } else if (mouseCounter == 2) {
                neck_x = xx;
                neck_y = yy;
                ui->sysLog->append("Neck: "+QString::number(xx)+' '+QString::number(yy));
            } else if (mouseCounter == 3) {
                tail_x = xx;
                tail_y = yy;
                ui->sysLog->append("Tail: "+QString::number(xx)+' '+QString::number(yy));
//            } else if (mouseCounter == 4) {
//                hs_x = xx;
//                hs_y = yy;
//                ui->sysLog->append("Headstage: "+QString::number(xx)+' '+QString::number(yy));
                //ask user if they want to save
                //return get result
                //QMessageBox *tmp;
                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(this, "Test", "Save Annotations?");
                if (reply == QMessageBox::Yes) {
                    //save point coordinates
                    pointStream << sampleCounter << "\t" << nose_x <<" " << nose_y
                                <<"\t" << neck_x <<" " << neck_y <<"\t" << tail_x <<" " << tail_y << endl;
//                                << "\t" << hs_x << " " << hs_y << endl;
                    ui->sysLog->append(QString::number(sampleCounter)+" Annotations Saved");
                } else {
                    ui->sysLog->append("Annotations Not Saved");
                    //if no points saved, reload the image
                    bool readOp = readImageFromFolder(sampleCounter);
                    if (!readOp) {
                        QMessageBox::about(this,tr("Warning"),tr("Image Loading Failure!"));
                        return;
                    }
                }
                mouseCounter = 0;
            }
        }
    }

    if (pointEnabled == 2) {
        if ((xx>=0)&&(yy>=0) && (xx<=ui->imgLab->width()) && (yy<=ui->imgLab->height())){
            // color image
            // with coordinates mapping using croping info
            joint_x[mouseCounter_color] = xx + crop_x;
            joint_y[mouseCounter_color] = yy + crop_y;

            ui->sysLog->append(joint_name[mouseCounter_color]+": "+QString::number(xx)+' '+QString::number(yy));
            //ui->sysLog->append("Mapped: "+QString::number(joint_x[mouseCounter_color])
            //                   +' '+QString::number(joint_y[mouseCounter_color]));

            mouseCounter_color++;

            // draw select point on image
            // -- 1~3 cyan
            // -- 4~7 red
            // -- 8~9 blue
            QPainter painter(&displayImgQT);
            if (mouseCounter_color <= 3) {
                painter.setBrush(Qt::cyan);
            } else if (mouseCounter_color <= 7) {
                if (mouseCounter_color % 2 ==0)
                    painter.setBrush(Qt::magenta);
                else
                    painter.setBrush(Qt::red);
            } else {
                painter.setBrush(Qt::blue);
            }
            painter.drawEllipse(QPoint(xx,yy), 3, 3);

            //draw lines for limbs
            if ((mouseCounter_color == 5)||(mouseCounter_color == 7)) {
                QPen pen;
                pen.setWidth(1);
                pen.setColor(Qt::red);
                painter.setPen(pen);
                painter.drawLine(joint_x[mouseCounter_color-1]-crop_x, joint_y[mouseCounter_color-1]-crop_y,
                                 joint_x[mouseCounter_color-2]-crop_x, joint_y[mouseCounter_color-2]-crop_y);
            }

            painter.end();
            ui->imgLab->setPixmap(QPixmap::fromImage(displayImgQT));

            //ask user if they want to save
            if (mouseCounter_color == 7) {
                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(this, "Test", "Save Annotations?");
                if (reply == QMessageBox::Yes) {
                    //save point coordinates
                    pointStream << sampleCounter;
                    for (int i = 0; i < 7; i++) {
                        pointStream << "\t" << joint_x[i] << " " << joint_y[i];
                    }
                    pointStream << endl;
                    ui->sysLog->append(QString::number(sampleCounter)+"Annotations Saved");
                } else {
                    ui->sysLog->append("Annotations Not Saved");
                    //if no points saved, reload the image
                    //must call painter.end() before refreshing displayImgQT, or the GUI will crash
                    bool readOp = readImageFromFolder(sampleCounter);
                    if (!readOp) {
                        QMessageBox::about(this,tr("Warning"),tr("Image Loading Failure!"));
                        return;
                    }
                }
                mouseCounter_color = 0;
            }
        }
    }
}

bool BehaviorLabel::readImageFromFolder(int sampleCounter_)
{
    QString strFileName = strFolderName + QString("/Sample%1.jpg")
            .arg(sampleCounter_, 6, 10, QLatin1Char('0'));

    oriImgQT.load(strFileName);

    if (oriImgQT.isNull()) {
        return false;
    } else {
        //show the config of oriImgQT
        //convert grayscale to RGB color
        //ui->sysLog->append("Format: "+QString::number(oriImgQT.format()));
        if (oriImgQT.format() == QImage::Format_Grayscale8) {
            oriImgQT = oriImgQT.convertToFormat(QImage::Format_RGB32);
        }

        int img_width = oriImgQT.width();
        int img_height = oriImgQT.height();
        if ((img_width > LAB_WIDTH) || (img_height > LAB_HEIGHT)) {
            //do cropping
            crop_x = ui->lineEdit_cropx->text().toInt();
            crop_y = ui->lineEdit_cropy->text().toInt();

            crop_width = (crop_x + LAB_WIDTH < img_width) ? LAB_WIDTH : (img_width - crop_x);
            crop_height = (crop_y + LAB_HEIGHT < img_height) ? LAB_HEIGHT : (img_height - crop_y);

            //ui->sysLog->append("Crop Height: "+QString::number(crop_height)
            //                   +" Width: "+QString::number(crop_width));

            displayImgQT = oriImgQT.copy(crop_x,crop_y,crop_width,crop_height);
            //ui->sysLog->append("Image Cropped");
        }
        else {
            displayImgQT = oriImgQT.copy();
        }
        ui->imgLab->setPixmap(QPixmap::fromImage(displayImgQT));
        //ui->sysLog->append("Image Height: "+QString::number(oriImgQT.height())
        //                   +" Width: "+QString::number(oriImgQT.width()));
        return true;
    }
}

void BehaviorLabel::on_BtnCrop_clicked()
{
    if(ui->BtnCrop->text()==tr("Fix")){
        ui->lineEdit_cropx->setDisabled(true);
        ui->lineEdit_cropy->setDisabled(true);
        ui->BtnCrop->setText(tr("Input"));
    }
    else {
        ui->lineEdit_cropx->setEnabled(true);
        ui->lineEdit_cropy->setEnabled(true);
        ui->BtnCrop->setText(tr("Fix"));
    }
}

void BehaviorLabel::on_BtnNextFrame_clicked()
{
    bool readOp = readImageFromFolder(sampleCounter+1);
    if (!readOp) {
        QMessageBox::about(this,tr("Warning"),tr("Already at the last frame!"));
        return;
    } else {
        sampleCounter++;
        ui->lcdCounter->display(sampleCounter);
    }
}

void BehaviorLabel::on_BtnPrevFrame_clicked()
{
    bool readOp = readImageFromFolder(sampleCounter-1);
    if (!readOp) {
        QMessageBox::about(this,tr("Warning"),tr("Already at the first frame!"));
        return;
    } else {
        sampleCounter--;
        ui->lcdCounter->display(sampleCounter);
    }
}
