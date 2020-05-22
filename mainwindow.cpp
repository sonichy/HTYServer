#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qserver.h"
#include <QDebug>
#include <QNetworkInterface>
#include <QFileDialog>
#include <QIntValidator>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    settings(QApplication::organizationName(), QApplication::applicationName())
{
    ui->setupUi(this);
    ui->mainToolBar->hide();
    setFixedSize(400,300);
    ui->lineEdit_port->setText(settings.value("Port", "8000").toString());
    ui->lineEdit_port->setValidator(new QIntValidator(1024, 65535, this));
    ui->lineEdit_root_dir->setText(settings.value("Directory", QApplication::applicationDirPath()).toString());
    QAction *action_browser = new QAction;
    action_browser->setIcon(QIcon::fromTheme("folder"));
    ui->lineEdit_root_dir->addAction(action_browser, QLineEdit::TrailingPosition);
    connect(action_browser, &QAction::triggered, [=](){
        QString dir = QFileDialog::getExistingDirectory(this, tr("Root Directory"),
                                                        settings.value("Directory", QApplication::applicationDirPath()).toString(),
                                                        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if(dir != "")
            ui->lineEdit_root_dir->setText(dir);
    });

    QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();
    foreach(QNetworkInterface interface, list) {
        QList<QNetworkAddressEntry> entryList = interface.addressEntries();
        foreach(QNetworkAddressEntry entry, entryList) {
            if(!entry.ip().toString().contains("::") && entry.ip().toString() != "127.0.0.1"){
                IP = entry.ip().toString();
                ui->label_ip->setText(IP);
            }
        }
    }

    connect(ui->pushButton_start, &QPushButton::toggled, [=](bool b){
        qDebug() << b;
        if(b){
            QString path = ui->lineEdit_root_dir->text();
            int port = ui->lineEdit_port->text().toInt();
            qDebug() << path << IP << port;
            if (QDir(path).exists()) {
                QServer Server(path, port);
                Server.startServer();
                ui->pushButton_start->setText("Stop");
                ui->lineEdit_port->setEnabled(false);
                ui->lineEdit_root_dir->setEnabled(false);
                settings.setValue("Port", ui->lineEdit_port->text());
                settings.setValue("Directory", ui->lineEdit_root_dir->text());
            } else {
                ui->pushButton_start->toggled(false);
                QMessageBox::critical(this, "Error", "Directory [" + path + "] does not exists !");
            }
        } else {
            ui->pushButton_start->setText("Start");
            ui->lineEdit_port->setEnabled(true);
            ui->lineEdit_root_dir->setEnabled(true);
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
