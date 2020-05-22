#ifndef QSERVER_H
#define QSERVER_H

#include "qclientthread.h"
#include <QTcpServer>

class QServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit QServer(QString storageFolder, int port, QObject *parent = nullptr);
    void startServer();

protected:
    void incomingConnection(qintptr socketDescriptor);

private:
    QString path;
    int port;

};

#endif // QSERVER_H