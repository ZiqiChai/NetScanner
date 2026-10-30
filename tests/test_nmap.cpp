#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QProcess>
#include <QStandardPaths>
#include <QXmlStreamReader>
#include <array>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "scanner.h"

// Bound but non-listening sockets reserve known-closed ports throughout both
// scans. Releasing an ephemeral listener first would introduce a port race.
struct ReservedPort {
 int fd=-1;
 quint16 port=0;
 bool bind() {
  fd=::socket(AF_INET,SOCK_STREAM,0);
  if(fd<0) return false;
  sockaddr_in address{}; address.sin_family=AF_INET;
  address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  if(::bind(fd,reinterpret_cast<sockaddr*>(&address),sizeof(address))!=0) return false;
  socklen_t size=sizeof(address);
  if(::getsockname(fd,reinterpret_cast<sockaddr*>(&address),&size)!=0) return false;
  port=ntohs(address.sin_port); return true;
 }
 ~ReservedPort() { if(fd>=0) ::close(fd); }
};
class NmapTests : public QObject {
 Q_OBJECT
private slots:
 void matchesConnectScan() {
  const QString nmap=QStandardPaths::findExecutable("nmap");
  if(nmap.isEmpty()) QSKIP("nmap is not installed; Nmap comparison was NOT performed");
  std::array<QTcpServer,3> listeners;
  std::array<ReservedPort,3> closed;
  QList<quint16> ports; QMap<quint16,QString> expected;
  for(auto &server:listeners) {
   QVERIFY(server.listen(QHostAddress::LocalHost));
   connect(&server,&QTcpServer::newConnection,&server,[&server] {
    while(server.hasPendingConnections()) server.nextPendingConnection()->deleteLater();
   });
   ports<<server.serverPort(); expected.insert(server.serverPort(),"open");
  }
  for(auto &reservation:closed) {
   QVERIFY(reservation.bind()); ports<<reservation.port; expected.insert(reservation.port,"closed");
  }
  QStringList portStrings;
  for(auto port:ports) portStrings<<QString::number(port);
  // Extra verbosity requests individual closed-port records rather than the
  // default extraports summary; absence of any requested port fails below.
  const QStringList arguments={"-sT","-Pn","-n","--reason","-vv","-p",portStrings.join(','),"-oX","-","127.0.0.1"};
  QProcess process;
  process.start(nmap,arguments);
  QVERIFY2(process.waitForStarted(3000),qPrintable(process.errorString()));
  QElapsedTimer deadline; deadline.start();
  while(process.state()!=QProcess::NotRunning && deadline.elapsed()<15000) QTest::qWait(10);
  if(process.state()!=QProcess::NotRunning) { process.kill(); process.waitForFinished(3000); QFAIL("Nmap comparison exceeded 15 seconds"); }
  const QByteArray output=process.readAllStandardOutput();
  const QByteArray errors=process.readAllStandardError();
  QCOMPARE(process.exitStatus(),QProcess::NormalExit);
  QVERIFY2(process.exitCode()==0,errors.constData());
  QMap<quint16,QString> nmapStates;
  QXmlStreamReader xml(output); quint16 port=0; bool inPort=false, successful=false;
  while(!xml.atEnd()) {
   xml.readNext();
   if(xml.isStartElement() && xml.name()==QStringLiteral("port")) {
    QVERIFY(xml.attributes().value("protocol")==QStringLiteral("tcp"));
    bool ok=false; const uint value=xml.attributes().value("portid").toUInt(&ok);
    QVERIFY(ok && value>=1 && value<=65535); port=quint16(value); inPort=true;
   } else if(xml.isStartElement() && xml.name()==QStringLiteral("state") && inPort) {
    QVERIFY(!nmapStates.contains(port)); nmapStates.insert(port,xml.attributes().value("state").toString());
   } else if(xml.isEndElement() && xml.name()==QStringLiteral("port")) inPort=false;
   else if(xml.isStartElement() && xml.name()==QStringLiteral("finished")) successful=xml.attributes().value("exit")==QStringLiteral("success");
  }
  QVERIFY2(!xml.hasError(),qPrintable(xml.errorString())); QVERIFY(successful);
  QCOMPARE(nmapStates,expected);
  Scanner scanner; QSignalSpy results(&scanner,&Scanner::result),done(&scanner,&Scanner::finished);
  scanner.start({"127.0.0.1"},ports,1000,3);
  QTRY_COMPARE_WITH_TIMEOUT(done.size(),1,10000); QVERIFY(!done[0][0].toBool());
  QCOMPARE(results.size(),ports.size()); QMap<quint16,QString> scannerStates;
  QSet<quint16> nmapOpen,scannerOpen;
  for(const auto &row:results) {
   QCOMPARE(row[0].toString(),QString("127.0.0.1"));
   const quint16 number=quint16(row[1].toUInt()); QVERIFY(!scannerStates.contains(number));
   scannerStates.insert(number,row[2].toString());
   if(row[2].toString()=="open") scannerOpen.insert(number);
  }
  for(auto it=nmapStates.cbegin();it!=nmapStates.cend();++it) if(it.value()=="open") nmapOpen.insert(it.key());
  QCOMPARE(scannerStates,nmapStates); QCOMPARE(scannerOpen,nmapOpen);
  qInfo().noquote()<<"Nmap TCP connect parity: 127.0.0.1; 3 open + 3 closed; all 6 port states and open sets match.";
 }
};
QTEST_GUILESS_MAIN(NmapTests)
#include "test_nmap.moc"
