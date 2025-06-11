/********************************************************************************
** Form generated from reading UI file 'LCAS-GUI-V1DgiuCu.ui'
**
** Created by: Qt User Interface Compiler version 6.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef LCAS_2D_GUI_2D_V1DGIUCU_H
#define LCAS_2D_GUI_2D_V1DGIUCU_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLCDNumber>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
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
    QDoubleSpinBox *spinBox_7;
    QDoubleSpinBox *spinBox_9;
    QDoubleSpinBox *spinBox_10;
    QDoubleSpinBox *spinBox_11;
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
        frame->setGeometry(QRect(430, 30, 161, 161));
        frame->setFrameShape(QFrame::Shape::StyledPanel);
        frame->setFrameShadow(QFrame::Shadow::Raised);
        frame_2 = new QFrame(centralwidget);
        frame_2->setObjectName("frame_2");
        frame_2->setGeometry(QRect(600, 30, 161, 161));
        frame_2->setFrameShape(QFrame::Shape::StyledPanel);
        frame_2->setFrameShadow(QFrame::Shadow::Raised);
        frame_3 = new QFrame(centralwidget);
        frame_3->setObjectName("frame_3");
        frame_3->setGeometry(QRect(430, 200, 161, 161));
        frame_3->setFrameShape(QFrame::Shape::StyledPanel);
        frame_3->setFrameShadow(QFrame::Shadow::Raised);
        frame_4 = new QFrame(centralwidget);
        frame_4->setObjectName("frame_4");
        frame_4->setGeometry(QRect(600, 200, 161, 161));
        frame_4->setFrameShape(QFrame::Shape::StyledPanel);
        frame_4->setFrameShadow(QFrame::Shadow::Raised);
        frame_4->setLineWidth(1);
        label = new QLabel(centralwidget);
        label->setObjectName("label");
        label->setGeometry(QRect(420, 430, 91, 31));
        label->setTextFormat(Qt::TextFormat::AutoText);
        label->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label_6 = new QLabel(centralwidget);
        label_6->setObjectName("label_6");
        label_6->setGeometry(QRect(520, 370, 151, 20));
        label_6->setFrameShape(QFrame::Shape::StyledPanel);
        label_6->setScaledContents(true);
        label_6->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label_7 = new QLabel(centralwidget);
        label_7->setObjectName("label_7");
        label_7->setGeometry(QRect(500, 0, 191, 20));
        QFont font;
        font.setFamilies({QString::fromUtf8("Arial")});
        label_7->setFont(font);
        label_7->setFrameShape(QFrame::Shape::StyledPanel);
        label_7->setAlignment(Qt::AlignmentFlag::AlignCenter);
        lcdNumber = new QLCDNumber(centralwidget);
        lcdNumber->setObjectName("lcdNumber");
        lcdNumber->setGeometry(QRect(520, 430, 111, 31));
        lcdNumber->setFrameShape(QFrame::Shape::Box);
        lcdNumber->setDigitCount(9);
        lcdNumber->setSegmentStyle(QLCDNumber::SegmentStyle::Flat);
        lcdNumber->setProperty("value", QVariant(1.023230000000000));
        lcdNumber->setProperty("intValue", QVariant(1));
        label_2 = new QLabel(centralwidget);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(420, 470, 91, 31));
        label_2->setTextFormat(Qt::TextFormat::AutoText);
        label_2->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label_3 = new QLabel(centralwidget);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(420, 510, 91, 31));
        label_3->setTextFormat(Qt::TextFormat::AutoText);
        label_3->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label_4 = new QLabel(centralwidget);
        label_4->setObjectName("label_4");
        label_4->setGeometry(QRect(420, 550, 91, 31));
        label_4->setTextFormat(Qt::TextFormat::AutoText);
        label_4->setAlignment(Qt::AlignmentFlag::AlignCenter);
        lcdNumber_2 = new QLCDNumber(centralwidget);
        lcdNumber_2->setObjectName("lcdNumber_2");
        lcdNumber_2->setGeometry(QRect(520, 470, 111, 31));
        lcdNumber_2->setFrameShape(QFrame::Shape::Box);
        lcdNumber_2->setDigitCount(9);
        lcdNumber_2->setSegmentStyle(QLCDNumber::SegmentStyle::Flat);
        lcdNumber_2->setProperty("value", QVariant(1.000000000000000));
        lcdNumber_2->setProperty("intValue", QVariant(1));
        lcdNumber_3 = new QLCDNumber(centralwidget);
        lcdNumber_3->setObjectName("lcdNumber_3");
        lcdNumber_3->setGeometry(QRect(520, 510, 111, 31));
        lcdNumber_3->setFrameShape(QFrame::Shape::Box);
        lcdNumber_3->setDigitCount(9);
        lcdNumber_3->setSegmentStyle(QLCDNumber::SegmentStyle::Flat);
        lcdNumber_3->setProperty("value", QVariant(1.000000000000000));
        lcdNumber_3->setProperty("intValue", QVariant(1));
        lcdNumber_4 = new QLCDNumber(centralwidget);
        lcdNumber_4->setObjectName("lcdNumber_4");
        lcdNumber_4->setGeometry(QRect(520, 550, 111, 31));
        lcdNumber_4->setFrameShape(QFrame::Shape::Box);
        lcdNumber_4->setDigitCount(9);
        lcdNumber_4->setSegmentStyle(QLCDNumber::SegmentStyle::Flat);
        lcdNumber_4->setProperty("value", QVariant(1.000000000000000));
        lcdNumber_4->setProperty("intValue", QVariant(1));
        label_5 = new QLabel(centralwidget);
        label_5->setObjectName("label_5");
        label_5->setGeometry(QRect(540, 400, 71, 20));
        label_5->setFrameShape(QFrame::Shape::NoFrame);
        label_5->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label_8 = new QLabel(centralwidget);
        label_8->setObjectName("label_8");
        label_8->setGeometry(QRect(650, 400, 121, 20));
        label_8->setFrameShape(QFrame::Shape::NoFrame);
        label_8->setAlignment(Qt::AlignmentFlag::AlignCenter);
        
        spinBox_7 = new QDoubleSpinBox(centralwidget);
        spinBox_7->setObjectName("spinBox_7");
        spinBox_7->setGeometry(QRect(660, 430, 104, 31));
        spinBox_7->setDecimals(3);
        spinBox_7->setRange(0.0, 1000.0);       // adjust range if needed
        spinBox_7->setSingleStep(0.01);
        spinBox_7->setAlignment(Qt::AlignCenter);
        spinBox_7->setToolTip("Set V threshold");

        spinBox_9 = new QDoubleSpinBox(centralwidget);
        spinBox_9->setObjectName("spinBox_9");
        spinBox_9->setGeometry(QRect(660, 470, 104, 31));
        spinBox_9->setDecimals(3);
        spinBox_9->setRange(0.0, 1000.0);
        spinBox_9->setSingleStep(0.01);
        spinBox_9->setAlignment(Qt::AlignCenter);
        spinBox_9->setToolTip("Set V threshold");

        spinBox_10 = new QDoubleSpinBox(centralwidget);
        spinBox_10->setObjectName("spinBox_10");
        spinBox_10->setGeometry(QRect(660, 510, 104, 31));
        spinBox_10->setDecimals(3);
        spinBox_10->setRange(0.0, 1000.0);
        spinBox_10->setSingleStep(0.01);
        spinBox_10->setAlignment(Qt::AlignCenter);
        spinBox_10->setToolTip("Set V threshold");

        spinBox_11 = new QDoubleSpinBox(centralwidget);
        spinBox_11->setObjectName("spinBox_11");
        spinBox_11->setGeometry(QRect(660, 550, 104, 31));
        spinBox_11->setDecimals(3);
        spinBox_11->setRange(0.0, 1000.0);
        spinBox_11->setSingleStep(0.01);
        spinBox_11->setAlignment(Qt::AlignCenter);
        spinBox_11->setToolTip("Set V threshold");

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
        spinBox_7->setToolTip(QCoreApplication::translate("MainWindow", "Set V threshold", nullptr));
        spinBox_9->setToolTip(QCoreApplication::translate("MainWindow", "Set V threshold", nullptr));
        spinBox_10->setToolTip(QCoreApplication::translate("MainWindow", "Set V threshold", nullptr));
        spinBox_11->setToolTip(QCoreApplication::translate("MainWindow", "Set V threshold", nullptr));

    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // LCAS_2D_GUI_2D_V1DGIUCU_H
