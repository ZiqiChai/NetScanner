#include "scanner.h"
#include <QTcpSocket>
#include <QTimer>
#include <QHostAddress>
#include <limits>
Scanner::Scanner(QObject *parent):QObject(parent) {}
Scanner::~Scanner() { clear(); }
void Scanner::clear() {
 const auto sockets=active_.keys(); active_.clear();
 for(auto *socket:sockets) { socket->disconnect(this); for(auto *timer:socket->findChildren<QTimer*>()) timer->stop(); socket->abort(); socket->deleteLater(); }
}
void Scanner::stop() {
 if(!running_) return;
 running_=false; ++generation_; clear(); emit finished(true);
}
void Scanner::start(QStringList hosts,QList<quint16> ports,int timeoutMs,int concurrency) {
 if(hosts.isEmpty() || ports.isEmpty() || timeoutMs<1 || concurrency<1 || concurrency>1024 || qint64(hosts.size())*ports.size()>std::numeric_limits<int>::max())
  throw std::invalid_argument("无效的扫描参数");
 for(const auto &host:hosts) { QHostAddress address(host); if(address.protocol()!=QAbstractSocket::IPv4Protocol) throw std::invalid_argument("需要 IPv4 地址"); }
 for(auto port:ports) if(!port) throw std::invalid_argument("端口不能为零");
 // Restart invalidates all callbacks belonging to the previous run.
 const quint64 before=generation_;
 stop();
 // A finished handler may itself have started another scan; do not overwrite it.
 if(generation_>before+1) return;
 hosts_=std::move(hosts); ports_=std::move(ports); timeout_=timeoutMs; concurrency_=concurrency;
 total_=int(hosts_.size()*ports_.size()); next_=completed_=0; running_=true;
 const auto generation=++generation_;
 emit progress(0,total_);
 QTimer::singleShot(0,this,[this,generation]{ pump(generation); });
}
void Scanner::pump(quint64 generation) {
 if(!running_ || generation!=generation_) return;
 while(active_.size()<concurrency_ && next_<total_) {
  const int index=next_++;
  Probe probe{hosts_[index/ports_.size()],ports_[index%ports_.size()],{}}; probe.elapsed.start();
  auto *socket=new QTcpSocket(this); active_.insert(socket,probe);
  auto *timer=new QTimer(socket); timer->setSingleShot(true);
  connect(timer,&QTimer::timeout,this,[this,socket,generation]{ complete(socket,"timeout",generation); });
  connect(socket,&QTcpSocket::connected,this,[this,socket,generation]{ complete(socket,"open",generation); });
  connect(socket,&QTcpSocket::errorOccurred,this,[this,socket,generation](QAbstractSocket::SocketError error){
   complete(socket,error==QAbstractSocket::ConnectionRefusedError ? "closed" : error==QAbstractSocket::SocketTimeoutError ? "timeout" : "error",generation);
  });
  timer->start(timeout_);
  socket->connectToHost(QHostAddress(probe.host),probe.port);
  if(!running_ || generation!=generation_) return;
 }
 if(active_.isEmpty() && next_==total_) { running_=false; emit finished(false); }
}
void Scanner::complete(QTcpSocket *socket,const QString &state,quint64 generation) {
 if(!running_ || generation!=generation_ || !active_.contains(socket)) return;
 const auto probe=active_.take(socket);
 socket->disconnect(this);
 for(auto *timer:socket->findChildren<QTimer*>()) timer->stop();
 socket->abort(); socket->deleteLater(); ++completed_;
 emit result(probe.host,probe.port,state,probe.elapsed.nsecsElapsed()/1000000.0);
 if(!running_ || generation!=generation_) return;
 emit progress(completed_,total_);
 QTimer::singleShot(0,this,[this,generation]{ pump(generation); });
}
