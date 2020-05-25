#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QNetworkInterface>
#include <QFileDialog>
#include <QIntValidator>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QDateTime>
#include <QMetaEnum>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    settings(QApplication::organizationName(), QApplication::applicationName())
{
    ui->setupUi(this);
    setFixedSize(400, 500);
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
                QString surl = IP + ":" + ui->lineEdit_port->text();
                ui->label_ip->setText("<a href='http://" + surl + + "'>" + surl + "</a>");
            }
        }
    }

    connect(ui->pushButton_start, &QPushButton::toggled, [=](bool b){        
        if(b){
            QString path = ui->lineEdit_root_dir->text();
            int port = ui->lineEdit_port->text().toInt();
            if (QDir(path).exists()) {
                if (tcpServer->listen(QHostAddress::Any, port)) {
                    ui->textBrowser->clear();
                    ui->textBrowser->append(QDateTime::currentDateTime().toString("yyyy年MM月dd日 hh:mm:ss") + " " + path);
                    ui->textBrowser->append(QDateTime::currentDateTime().toString("yyyy年MM月dd日 hh:mm:ss") + " " + IP + ":" + QString::number(port));
                    ui->pushButton_start->setText("Stop");
                    ui->lineEdit_port->setEnabled(false);
                    ui->lineEdit_root_dir->setEnabled(false);
                    settings.setValue("Port", ui->lineEdit_port->text());
                    settings.setValue("Directory", ui->lineEdit_root_dir->text());
                }
            } else {
                ui->pushButton_start->toggled(false);
                QMessageBox::critical(this, "Error", "Directory [" + path + "] does not exists !");
            }
        } else {
            tcpServer->close();
            ui->pushButton_start->setText("Start");
            ui->lineEdit_port->setEnabled(true);
            ui->lineEdit_root_dir->setEnabled(true);
        }
    });

    connect(ui->pushButton_clear, &QPushButton::clicked, [=]{
        ui->textBrowser->clear();
    });

    tcpServer = new QTcpServer(this);
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newConnect()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::newConnect()
{
    tcpSocket = tcpServer->nextPendingConnection();
    ui->textBrowser->append(QDateTime::currentDateTime().toString("yyyy年MM月dd日 hh:mm:ss") + " " + "newConnect: " + tcpSocket->peerAddress().toString().replace("::ffff:",""));
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), [=](QAbstractSocket::SocketError socketError){
        qDebug() << socketError;
        QMetaEnum metaEnum = QMetaEnum::fromType<QAbstractSocket::SocketError>();
        QString errorString = metaEnum.valueToKey(socketError);
        ui->textBrowser->append(QDateTime::currentDateTime().toString("yyyy年MM月dd日 hh:mm:ss") + " " + errorString);
    });
}

void MainWindow::readyRead()
{
    // receive header
    if(!headerReceived)
    {
        receiveBuff += tcpSocket->readAll();
        if(receiveBuff.contains("\r\n\r\n")) {
            int bytes = receiveBuff.indexOf("\r\n\r\n") + 1;
            QByteArray message = receiveBuff.left(bytes + 1);
            headerReceived = true;
            int headerPos = message.toLower().indexOf("content-length:");

            if(headerPos != -1)
            {
                //bodyBuff.clear();
                contentLen = 0;
                QString contentLenHeader = message.mid(headerPos, message.indexOf("\r\n", headerPos) - headerPos);
                QRegExp rx("(\\d+)");
                rx.indexIn(contentLenHeader);
                QStringList list = rx.capturedTexts();
                contentLen = list[1].toLong();
                qDebug() << "Content-Length=" << contentLen;
                readyRead();
            }
            else
            {
                ProcessData(message);
                qDebug() << message;
            }
        }
    }
    // receive body
    else
    {
        receiveBuff += tcpSocket->readAll();
        QByteArray body = receiveBuff.mid(receiveBuff.indexOf("\r\n\r\n"));
        qDebug() << body.size() << " " << contentLen;
        if(receiveBuff.size() >= contentLen) {
            qDebug() << "DONE" << receiveBuff.size() << ", " << body.size();
            ProcessData(receiveBuff);
            qDebug() << receiveBuff;
        }
        else {
            qDebug() << "ReceivedBODY=" << receiveBuff.size();

        }
    }
}

void MainWindow::ProcessData(QByteArray receivedData)
{
    //QString response = "HTTP/1.1 200 OK\r\nContent-type: text/html; charset=utf-8\r\n\r\n%1";
    //QByteArray receivedData = socket->readAll();
    //qDebug() << socketDescriptor;
    qDebug() << "Headers: ";
    qDebug() << "_________________________________________________________________";
    //qDebug() << receivedData;
    QString path;
    QStringList SL = QString(receivedData).split("\r\n");
    for(int i=0; i<SL.length(); i++){
        qDebug() << "|" << SL.at(i);
        if(SL.at(i).startsWith("GET ")){
            QStringList SL_get = SL.at(i).split(" ");
            path = SL_get.at(1);
        }
    }
    qDebug() << "_________________________________________________________________";

    qDebug() << "Action:" << "DEFAULT LIST";
    ListSubjects(path);

    tcpSocket->disconnectFromHost();
    receiveBuff.clear();
    headerReceived = false;
}

void MainWindow::ListSubjects(QString path)
{
    path = QByteArray::fromPercentEncoding(path.toUtf8());  //%XX反转义
    ui->textBrowser->append(QDateTime::currentDateTime().toString("yyyy年MM月dd日 hh:mm:ss") + " " + path);
    QString path_abs = ui->lineEdit_root_dir->text() + path;
    QString pathr;
    if(path == "/")
        pathr = path;
    else
        pathr = path + "/";
    qDebug() << path << path_abs;
    QFileInfo fileInfo(path_abs);
    QString responseData;
    if(fileInfo.isDir()){
        QDir dir(path_abs);
        dir.setSorting(QDir::DirsFirst | QDir::Name);
        //dir.setFilter(QDir::NoDotAndDotDot);
        QFileInfoList fileInfoList = dir.entryInfoList();
        responseData = "<!DOCTYPE html>\n<html>\n<head>\n<meta charset='UTF-8'>\n<title>File List</title>\n<style>\na { text-decoration:none; }\ntd { padding:0 10px; white-space:nowrap; }\n</style>\n</head>\n<body>\n<h1>" + path + "</h1>\n<table>\n<tr><th>Name</th><th>Size</th><th>Time</th></tr>";
        for(int i=0; i<fileInfoList.count(); i++) {
            if(fileInfoList.at(i).isDir())
                if(fileInfoList.at(i).fileName() == "." || fileInfoList.at(i).fileName() == ".."){
                    responseData.append("<tr><td>[<a href='" + fileInfoList.at(i).fileName() + "'>" + fileInfoList.at(i).fileName() + "</a>]</td><td></td><td></td></tr>\n");
                }else{
                    responseData.append("<tr><td>[<a href='" + pathr + fileInfoList.at(i).fileName() + "'>" + fileInfoList.at(i).fileName() + "</a>]</td><td></td><td>" + fileInfoList.at(i).lastModified().toString("yyyy-MM-dd hh:mm:ss") + "</td></tr>\n");
                }
            else
                responseData.append("<tr><td><a href='" + pathr + fileInfoList.at(i).fileName() + "'>" + fileInfoList.at(i).fileName() + "</a></td><td>" + BS(fileInfoList.at(i).size()) + "</td><td>" + fileInfoList.at(i).lastModified().toString("yyyy-MM-dd hh:mm:ss") + "</td></tr>\n");
        }
        responseData.append("</table>\n</body>\n</html>");
        QByteArray headers, BA;
        BA = responseData.toUtf8();
        QString http = "HTTP/1.1 200 OK\r\n";
        http += "Server: QTcpSocket\r\n";
        http += "Content-Type: text/html;charset=utf-8\r\n";
        //http += "Connection: keep-alive\r\n";
        http += QString("Content-Length: %1\r\n\r\n").arg(QString::number(BA.length()));
        headers.append(http);
        tcpSocket->write(headers);
        tcpSocket->write(BA);
        qDebug() << headers;
        qDebug() << BA;
    }else if(fileInfo.isFile()){
        //https://blog.csdn.net/A18373279153/article/details/80364320
        QFile file(path_abs);
        QMimeDatabase MD;
        QMimeType mimeType = MD.mimeTypeForFile(path_abs);
        QString mime = mimeType.name();
        qDebug() << mime;
        if (file.open(QIODevice::ReadOnly)) {
            QString http = "HTTP/1.1 200 OK\r\n";
            http += "Server: QTcpSocket\r\n";
            http += "Content-Type: " + mime + ";charset=utf-8\r\n";
            http += "Connection: keep-alive\r\n";
            http += QString("Content-Length: %1\r\n\r\n").arg(QString::number(file.size()));
            QByteArray headers, BA;
            headers.append(http);
            tcpSocket->write(headers);
            qDebug() << headers;
            while (!file.atEnd()) {
                BA = file.read(10240);  //每次读取10k数据
                tcpSocket->write(BA);
                //qDebug() << "\b" << file.pos();
            }
        } else {
            qDebug() << path_abs << "open failed!";
        }
    }
}

QString MainWindow::BS(long b)
{
    QString s = "";
    if (b > 999999999) {
        s = QString::number(b/(1024*1024*1024.0), 'f', 2) + " GB";
    } else {
        if (b > 999999) {
            s = QString::number(b/(1024*1024.0), 'f', 2) + " MB";
        } else {
            if (b > 999) {
                s = QString::number(b/(1024.0), 'f',2) + " KB";
            } else {
                s = QString::number(b) + " B";
            }
        }
    }
    return s;
}