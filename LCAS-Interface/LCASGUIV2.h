/********************************************************************************
** Form generated from reading UI file 'LCASGUIV3pagxMF.ui'
**
** Created by: Qt User Interface Compiler version 6.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef LCASGUIV3PAGXMF_H
#define LCASGUIV3PAGXMF_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLCDNumber>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QFrame *frame;
    QFrame *frame_2;
    QFrame *frame_3;
    QFrame *frame_4;
    QLabel *label;
    QLabel *label_6;
    QLabel *label_7;
    QLCDNumber *lcdNumber;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;
    QLCDNumber *lcdNumber_2;
    QLCDNumber *lcdNumber_3;
    QLCDNumber *lcdNumber_4;
    QLabel *label_5;
    QLabel *label_8;
    QDoubleSpinBox *doubleSpinBox;
    QDoubleSpinBox *doubleSpinBox_2;
    QDoubleSpinBox *doubleSpinBox_3;
    QDoubleSpinBox *doubleSpinBox_4;
    QLabel *label_9;
    QLabel *label_10;
    QLabel *label_11;
    QDoubleSpinBox *doubleSpinBox_Vset;
    QDoubleSpinBox *doubleSpinBox_Iset;
    QPushButton *PSoutputButton;
    QFrame *OutIndicatorFrame;
    QDoubleSpinBox *doubleSpinBox_Vset_2;
    QLabel *label_12;
    QLabel *label_13;
    QPushButton *PSoutputButton_2;
    QFrame *OutIndicatorFrame_2;
    QLabel *label_14;
    QDoubleSpinBox *doubleSpinBox_Iset_2;
    QPushButton *EStop;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(796, 696);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        frame = new QFrame(centralwidget);
        frame->setObjectName("frame");
        frame->setGeometry(QRect(430, 50, 161, 161));
        frame->setFrameShape(QFrame::Shape::StyledPanel);
        frame->setFrameShadow(QFrame::Shadow::Raised);
        frame_2 = new QFrame(centralwidget);
        frame_2->setObjectName("frame_2");
        frame_2->setGeometry(QRect(600, 50, 161, 161));
        frame_2->setFrameShape(QFrame::Shape::StyledPanel);
        frame_2->setFrameShadow(QFrame::Shadow::Raised);
        frame_3 = new QFrame(centralwidget);
        frame_3->setObjectName("frame_3");
        frame_3->setGeometry(QRect(430, 220, 161, 161));
        frame_3->setFrameShape(QFrame::Shape::StyledPanel);
        frame_3->setFrameShadow(QFrame::Shadow::Raised);
        frame_4 = new QFrame(centralwidget);
        frame_4->setObjectName("frame_4");
        frame_4->setGeometry(QRect(600, 220, 161, 161));
        frame_4->setFrameShape(QFrame::Shape::StyledPanel);
        frame_4->setFrameShadow(QFrame::Shadow::Raised);
        frame_4->setLineWidth(1);
        label = new QLabel(centralwidget);
        label->setObjectName("label");
        label->setGeometry(QRect(420, 470, 91, 31));
        label->setTextFormat(Qt::TextFormat::AutoText);
        label->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label_6 = new QLabel(centralwidget);
        label_6->setObjectName("label_6");
        label_6->setGeometry(QRect(520, 410, 151, 20));
        label_6->setFrameShape(QFrame::Shape::StyledPanel);
        label_6->setScaledContents(true);
        label_6->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label_7 = new QLabel(centralwidget);
        label_7->setObjectName("label_7");
        label_7->setGeometry(QRect(500, 20, 191, 20));
        QFont font;
        font.setFamilies({QString::fromUtf8("Arial")});
        label_7->setFont(font);
        label_7->setFrameShape(QFrame::Shape::StyledPanel);
        label_7->setAlignment(Qt::AlignmentFlag::AlignCenter);
        lcdNumber = new QLCDNumber(centralwidget);
        lcdNumber->setObjectName("lcdNumber");
        lcdNumber->setGeometry(QRect(520, 470, 111, 31));
        lcdNumber->setFrameShape(QFrame::Shape::Box);
        lcdNumber->setDigitCount(9);
        lcdNumber->setSegmentStyle(QLCDNumber::SegmentStyle::Flat);
        lcdNumber->setProperty("value", QVariant(1.000000000000000));
        lcdNumber->setProperty("intValue", QVariant(1));
        label_2 = new QLabel(centralwidget);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(420, 510, 91, 31));
        label_2->setTextFormat(Qt::TextFormat::AutoText);
        label_2->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label_3 = new QLabel(centralwidget);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(420, 550, 91, 31));
        label_3->setTextFormat(Qt::TextFormat::AutoText);
        label_3->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label_4 = new QLabel(centralwidget);
        label_4->setObjectName("label_4");
        label_4->setGeometry(QRect(420, 590, 91, 31));
        label_4->setTextFormat(Qt::TextFormat::AutoText);
        label_4->setAlignment(Qt::AlignmentFlag::AlignCenter);
        lcdNumber_2 = new QLCDNumber(centralwidget);
        lcdNumber_2->setObjectName("lcdNumber_2");
        lcdNumber_2->setGeometry(QRect(520, 510, 111, 31));
        lcdNumber_2->setFrameShape(QFrame::Shape::Box);
        lcdNumber_2->setDigitCount(9);
        lcdNumber_2->setSegmentStyle(QLCDNumber::SegmentStyle::Flat);
        lcdNumber_2->setProperty("value", QVariant(1.000000000000000));
        lcdNumber_2->setProperty("intValue", QVariant(1));
        lcdNumber_3 = new QLCDNumber(centralwidget);
        lcdNumber_3->setObjectName("lcdNumber_3");
        lcdNumber_3->setGeometry(QRect(520, 550, 111, 31));
        lcdNumber_3->setFrameShape(QFrame::Shape::Box);
        lcdNumber_3->setDigitCount(9);
        lcdNumber_3->setSegmentStyle(QLCDNumber::SegmentStyle::Flat);
        lcdNumber_3->setProperty("value", QVariant(1.000000000000000));
        lcdNumber_3->setProperty("intValue", QVariant(1));
        lcdNumber_4 = new QLCDNumber(centralwidget);
        lcdNumber_4->setObjectName("lcdNumber_4");
        lcdNumber_4->setGeometry(QRect(520, 590, 111, 31));
        lcdNumber_4->setFrameShape(QFrame::Shape::Box);
        lcdNumber_4->setDigitCount(9);
        lcdNumber_4->setSegmentStyle(QLCDNumber::SegmentStyle::Flat);
        lcdNumber_4->setProperty("value", QVariant(1.000000000000000));
        lcdNumber_4->setProperty("intValue", QVariant(1));
        label_5 = new QLabel(centralwidget);
        label_5->setObjectName("label_5");
        label_5->setGeometry(QRect(540, 440, 71, 20));
        label_5->setFrameShape(QFrame::Shape::NoFrame);
        label_5->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label_8 = new QLabel(centralwidget);
        label_8->setObjectName("label_8");
        label_8->setGeometry(QRect(650, 440, 121, 20));
        label_8->setFrameShape(QFrame::Shape::NoFrame);
        label_8->setAlignment(Qt::AlignmentFlag::AlignCenter);
        doubleSpinBox = new QDoubleSpinBox(centralwidget);
        doubleSpinBox->setObjectName("doubleSpinBox");
        doubleSpinBox->setGeometry(QRect(660, 470, 101, 31));
        doubleSpinBox->setValue(6.000000000000000);
        doubleSpinBox_2 = new QDoubleSpinBox(centralwidget);
        doubleSpinBox_2->setObjectName("doubleSpinBox_2");
        doubleSpinBox_2->setGeometry(QRect(660, 510, 101, 31));
        doubleSpinBox_2->setValue(6.000000000000000);
        doubleSpinBox_3 = new QDoubleSpinBox(centralwidget);
        doubleSpinBox_3->setObjectName("doubleSpinBox_3");
        doubleSpinBox_3->setGeometry(QRect(660, 550, 101, 31));
        doubleSpinBox_3->setValue(6.000000000000000);
        doubleSpinBox_4 = new QDoubleSpinBox(centralwidget);
        doubleSpinBox_4->setObjectName("doubleSpinBox_4");
        doubleSpinBox_4->setGeometry(QRect(660, 590, 101, 31));
        doubleSpinBox_4->setValue(6.000000000000000);
        label_9 = new QLabel(centralwidget);
        label_9->setObjectName("label_9");
        label_9->setGeometry(QRect(110, 20, 191, 20));
        label_9->setFont(font);
        label_9->setFrameShape(QFrame::Shape::StyledPanel);
        label_9->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label_10 = new QLabel(centralwidget);
        label_10->setObjectName("label_10");
        label_10->setGeometry(QRect(30, 50, 91, 31));
        label_10->setTextFormat(Qt::TextFormat::AutoText);
        label_10->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label_11 = new QLabel(centralwidget);
        label_11->setObjectName("label_11");
        label_11->setGeometry(QRect(30, 80, 91, 31));
        label_11->setTextFormat(Qt::TextFormat::AutoText);
        label_11->setAlignment(Qt::AlignmentFlag::AlignCenter);
        doubleSpinBox_Vset = new QDoubleSpinBox(centralwidget);
        doubleSpinBox_Vset->setObjectName("doubleSpinBox_Vset");
        doubleSpinBox_Vset->setGeometry(QRect(110, 50, 101, 31));
        doubleSpinBox_Vset->setValue(0.000000000000000);
        doubleSpinBox_Iset = new QDoubleSpinBox(centralwidget);
        doubleSpinBox_Iset->setObjectName("doubleSpinBox_Iset");
        doubleSpinBox_Iset->setGeometry(QRect(110, 80, 101, 31));
        doubleSpinBox_Iset->setValue(0.000000000000000);
        PSoutputButton = new QPushButton(centralwidget);
        PSoutputButton->setObjectName("PSoutputButton");
        PSoutputButton->setGeometry(QRect(230, 60, 121, 41));
        OutIndicatorFrame = new QFrame(centralwidget);
        OutIndicatorFrame->setObjectName("OutIndicatorFrame");
        OutIndicatorFrame->setGeometry(QRect(360, 70, 21, 21));
        OutIndicatorFrame->setFrameShape(QFrame::Shape::StyledPanel);
        OutIndicatorFrame->setFrameShadow(QFrame::Shadow::Raised);
        doubleSpinBox_Vset_2 = new QDoubleSpinBox(centralwidget);
        doubleSpinBox_Vset_2->setObjectName("doubleSpinBox_Vset_2");
        doubleSpinBox_Vset_2->setGeometry(QRect(110, 160, 101, 31));
        doubleSpinBox_Vset_2->setValue(0.000000000000000);
        label_12 = new QLabel(centralwidget);
        label_12->setObjectName("label_12");
        label_12->setGeometry(QRect(30, 190, 91, 31));
        label_12->setTextFormat(Qt::TextFormat::AutoText);
        label_12->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label_13 = new QLabel(centralwidget);
        label_13->setObjectName("label_13");
        label_13->setGeometry(QRect(110, 130, 191, 20));
        label_13->setFont(font);
        label_13->setFrameShape(QFrame::Shape::StyledPanel);
        label_13->setAlignment(Qt::AlignmentFlag::AlignCenter);
        PSoutputButton_2 = new QPushButton(centralwidget);
        PSoutputButton_2->setObjectName("PSoutputButton_2");
        PSoutputButton_2->setGeometry(QRect(230, 170, 121, 41));
        OutIndicatorFrame_2 = new QFrame(centralwidget);
        OutIndicatorFrame_2->setObjectName("OutIndicatorFrame_2");
        OutIndicatorFrame_2->setGeometry(QRect(360, 180, 21, 21));
        OutIndicatorFrame_2->setFrameShape(QFrame::Shape::StyledPanel);
        OutIndicatorFrame_2->setFrameShadow(QFrame::Shadow::Raised);
        label_14 = new QLabel(centralwidget);
        label_14->setObjectName("label_14");
        label_14->setGeometry(QRect(30, 160, 91, 31));
        label_14->setTextFormat(Qt::TextFormat::AutoText);
        label_14->setAlignment(Qt::AlignmentFlag::AlignCenter);
        doubleSpinBox_Iset_2 = new QDoubleSpinBox(centralwidget);
        doubleSpinBox_Iset_2->setObjectName("doubleSpinBox_Iset_2");
        doubleSpinBox_Iset_2->setGeometry(QRect(110, 190, 101, 31));
        doubleSpinBox_Iset_2->setValue(0.000000000000000);
        EStop = new QPushButton(centralwidget);
        EStop->setObjectName("EStop");
        EStop->setGeometry(QRect(70, 240, 281, 61));
        QFont font1;
        font1.setFamilies({QString::fromUtf8("Arial")});
        font1.setPointSize(18);
        font1.setBold(true);
        EStop->setFont(font1);
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 796, 24));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Channel 1:", nullptr));
        label_6->setText(QCoreApplication::translate("MainWindow", "Stray Light Sensors", nullptr));
        label_7->setText(QCoreApplication::translate("MainWindow", "Thermal Camera Outputs", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Channel 2:", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "Channel 3:", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "Channel 4:", nullptr));
        label_5->setText(QCoreApplication::translate("MainWindow", "V out (V)", nullptr));
        label_8->setText(QCoreApplication::translate("MainWindow", "V threshold (V)", nullptr));
        label_9->setText(QCoreApplication::translate("MainWindow", "Power Supply 1", nullptr));
        label_10->setText(QCoreApplication::translate("MainWindow", "Set V:", nullptr));
        label_11->setText(QCoreApplication::translate("MainWindow", "Set I:", nullptr));
        PSoutputButton->setText(QCoreApplication::translate("MainWindow", "Out", nullptr));
        label_12->setText(QCoreApplication::translate("MainWindow", "Set I:", nullptr));
        label_13->setText(QCoreApplication::translate("MainWindow", "Power Supply 2", nullptr));
        PSoutputButton_2->setText(QCoreApplication::translate("MainWindow", "Out", nullptr));
        label_14->setText(QCoreApplication::translate("MainWindow", "Set V:", nullptr));
        EStop->setText(QCoreApplication::translate("MainWindow", "EMERGENCY STOP", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // LCASGUIV3PAGXMF_H
