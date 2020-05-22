#ifndef QCLIENTTHREAD_H
#define QCLIENTTHREAD_H

#include <QThread>
#include <QTcpSocket>

class QClientThread : public QThread
{
    Q_OBJECT

public:
    explicit QClientThread(QString fodler, qintptr ID, QObject *parent = nullptr);
    void run();

signals:
    void error(QTcpSocket::SocketError socketError);

public slots:
    void readyRead();
    void disconnect();

private:
    QString storageFolder;
    qintptr socketDescriptor;
    QTcpSocket *socket;
    QByteArray receiveBuff;    
    bool headerReceived;    
    long contentLen;    
    void ProcessData(QByteArray receivedData);
    void ListSubjects(QString path);    

};

#endif // QCLIENTTHREAD_H
