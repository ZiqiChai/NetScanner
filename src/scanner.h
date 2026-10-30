#pragma once
#include <QObject>
#include <QStringList>
#include <QHash>
#include <QElapsedTimer>
#include <stdexcept>
class QTcpSocket;
class Scanner : public QObject {
 Q_OBJECT
public:
 explicit Scanner(QObject *parent=nullptr);
 ~Scanner() override;
 void start(QStringList hosts,QList<quint16> ports,int timeoutMs=500,int concurrency=128);
 void stop();
 bool isRunning() const { return running_; }
signals:
 void result(QString host,quint16 port,QString state,double latencyMs);
 void progress(int completed,int total);
 void finished(bool cancelled);
private:
 struct Probe { QString host; quint16 port; QElapsedTimer elapsed; };
 void pump(quint64 generation);
 void complete(QTcpSocket *socket,const QString &state,quint64 generation);
 void clear();
 QStringList hosts_;
 QList<quint16> ports_;
 QHash<QTcpSocket*,Probe> active_;
 quint64 generation_=0;
 int next_=0, completed_=0, total_=0, timeout_=500, concurrency_=128;
 bool running_=false;
};
