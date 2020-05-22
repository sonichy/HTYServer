#include "qclientthread.h"
#include <QDateTime>
#include <QHostAddress>
#include <QMimeDatabase>
#include <QDebug>
#include <QDir>

QClientThread::QClientThread(QString storageFolder, qintptr ID, QObject *parent) : QThread(parent)
{
    this->storageFolder = storageFolder;
    this->socketDescriptor = ID;
}

void QClientThread::run()
{
    qDebug() << socketDescriptor << " Starting thread";
    socket = new QTcpSocket();
    if(!socket->setSocketDescriptor(this->socketDescriptor))
    {
        emit error(socket->error());
        qDebug() << socket->errorString();
        return;
    }

    headerReceived = false;

    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()), Qt::DirectConnection);
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnect()), Qt::DirectConnection);

    qDebug() << socketDescriptor << " Client connected";

    exec();
}

void QClientThread::readyRead()
{
    qDebug() << "Received" << socket->peerAddress().toString();
    // receive header
    if(!headerReceived)
    {
        receiveBuff += socket->readAll();
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
        receiveBuff += socket->readAll();
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

void QClientThread::ProcessData(QByteArray receivedData)
{
    //QString response = "HTTP/1.1 200 OK\r\nContent-type: text/html; charset=utf-8\r\n\r\n%1";
    //QByteArray receivedData = socket->readAll();
    qDebug() << socketDescriptor;
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

    socket->disconnectFromHost();    
    receiveBuff.clear();    
    headerReceived = false;
}

void QClientThread::ListSubjects(QString path)
{
    path = QByteArray::fromPercentEncoding(path.toUtf8());  //%XX反转义
    QString path_abs = storageFolder + path;
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
        //dir.setFilter(QDir::NoDotAndDotDot);
        QFileInfoList fileInfoList = dir.entryInfoList();
        responseData = "<!DOCTYPE html>\n<html>\n<head>\n<title>File List</title>\n<style>\na { text-decoration:none; }\ntd { padding:0 10px; }\n</style>\n</head>\n<body>\n<h1>" + path + "</h1>\n<table>\n<tr><th>Name</th><th>Size</th><th>Time</th></tr>";
        for(int i=0; i<fileInfoList.count(); i++) {
            if(fileInfoList.at(i).isDir())
                if(fileInfoList.at(i).fileName() == "." || fileInfoList.at(i).fileName() == ".."){
                    responseData.append("<tr><td>[<a href='" + fileInfoList.at(i).fileName() + "'>" + fileInfoList.at(i).fileName() + "</a>]</td><td></td><td></td></tr>\n");
                }else{
                    responseData.append("<tr><td>[<a href='" + pathr + fileInfoList.at(i).fileName() + "'>" + fileInfoList.at(i).fileName() + "</a>]</td><td>" + QString::number(fileInfoList.at(i).size()) + "</td><td>" + fileInfoList.at(i).lastModified().toString("yyyy-MM-dd hh:mm:ss") + "</td></tr>\n");
                }
            else
                responseData.append("<tr><td><a href='" + pathr + fileInfoList.at(i).fileName() + "'>" + fileInfoList.at(i).fileName() + "</a></td><td>" + QString::number(fileInfoList.at(i).size()) + "</td><td>" + fileInfoList.at(i).lastModified().toString("yyyy-MM-dd hh:mm:ss") + "</td></tr>\n");
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
        socket->write(headers);
        socket->write(BA);
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
            //http += "Content-Type: application/octet-stream;charset=utf-8\r\n";
            http += "Content-Type: " + mime + ";charset=utf-8\r\n";
            http += "Connection: keep-alive\r\n";
            http += QString("Content-Length: %1\r\n\r\n").arg(QString::number(file.size()));
            QByteArray headers, BA;
            headers.append(http);
            socket->write(headers);
            qDebug() << headers;
            while (!file.atEnd()) {
                BA = file.read(10240);  //每次读取10k数据
                socket->write(BA);
                //qDebug() << "\b" << file.pos();
            }
        } else {
            qDebug() << path_abs << "open failed!";
        }
    }
}

void QClientThread::disconnect()
{
    qDebug() << socketDescriptor << " Disconnected";
    socket->deleteLater();
    exit(0);
}
