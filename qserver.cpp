#include "qserver.h"
#include <QDebug>

QServer::QServer(QString path, int port, QObject *parent) :
    QTcpServer(parent)
{    
    this->path = path;
    this->port = port;
    connect(this, &QTcpServer::acceptError, [=]{
        qDebug() << this->errorString();
    });
}

void QServer::startServer()
{
    if (this->listen(QHostAddress::Any, port)) {
        qDebug() << "Start listen" << this->serverAddress() << this->serverPort();
    } else {
        qDebug() << this->errorString();
    }
}

void QServer::incomingConnection(qintptr socketDescriptor)
{
    qDebug() << socketDescriptor << " Connecting...";
    QClientThread *thread = new QClientThread(path, socketDescriptor, this);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}