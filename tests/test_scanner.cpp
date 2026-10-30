#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include "scanner.h"
class ScannerTests : public QObject {
 Q_OBJECT
private slots:
 void openAndClosed() {
  QTcpServer server, reserved; QVERIFY(server.listen(QHostAddress::LocalHost)); QVERIFY(reserved.listen(QHostAddress::LocalHost));
  const auto closed=reserved.serverPort(); reserved.close();
  Scanner scanner; QSignalSpy results(&scanner,&Scanner::result), done(&scanner,&Scanner::finished), progress(&scanner,&Scanner::progress);
  scanner.start({"127.0.0.1"},{server.serverPort(),closed},200,1);
  QTRY_COMPARE(done.size(),1); QCOMPARE(done[0][0].toBool(),false); QCOMPARE(results.size(),2);
  QCOMPARE(results[0][2].toString(),QString("open")); QCOMPARE(results[1][2].toString(),QString("closed"));
  QCOMPARE(progress.last()[0].toInt(),2); QCOMPARE(progress.last()[1].toInt(),2);
 }
 void cancelAndRestart() {
  QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost)); Scanner scanner;
  QSignalSpy results(&scanner,&Scanner::result),done(&scanner,&Scanner::finished);
  scanner.start({"127.0.0.1"},{server.serverPort()},200,1); scanner.stop();
  QCOMPARE(done.size(),1); QVERIFY(done[0][0].toBool());
  scanner.start({"127.0.0.1"},{server.serverPort()},200,1);
  QTRY_COMPARE(done.size(),2); QCOMPARE(results.size(),1); QVERIFY(!done[1][0].toBool());
  scanner.stop(); QCOMPARE(done.size(),2);
 }
 void stopFromResult() {
  QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost)); Scanner scanner;
  QSignalSpy results(&scanner,&Scanner::result), done(&scanner,&Scanner::finished);
  connect(&scanner,&Scanner::result,&scanner,&Scanner::stop);
  scanner.start({"127.0.0.1","127.0.0.2"},{server.serverPort()},200,1);
  QTRY_COMPARE(done.size(),1); QVERIFY(done[0][0].toBool()); QCOMPARE(results.size(),1);
 }
 void restartFromResult() {
  QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost)); Scanner scanner;
  QSignalSpy results(&scanner,&Scanner::result), done(&scanner,&Scanner::finished);
  bool restarted=false;
  connect(&scanner,&Scanner::result,&scanner,[&] {
   if(!restarted) { restarted=true; scanner.start({"127.0.0.1"},{server.serverPort()},200,1); }
  });
  scanner.start({"127.0.0.1","127.0.0.2"},{server.serverPort()},200,1);
  QTRY_COMPARE(done.size(),2); QCOMPARE(results.size(),2);
  QVERIFY(done[0][0].toBool()); QVERIFY(!done[1][0].toBool());
 }
 void completeMany() {
  QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost));
  connect(&server,&QTcpServer::newConnection,&server,[&] {
   while(server.hasPendingConnections()) server.nextPendingConnection()->deleteLater();
  });
  Scanner scanner; QSignalSpy results(&scanner,&Scanner::result), done(&scanner,&Scanner::finished);
  QList<quint16> ports; for(int i=0;i<100;++i) ports<<server.serverPort();
  scanner.start({"127.0.0.1"},ports,1000,4);
  QTRY_COMPARE(done.size(),1); QCOMPARE(results.size(),100);
  for(const auto &row:results) QCOMPARE(row[2].toString(),QString("open"));
 }
 void invalidInputs() {
  Scanner scanner;
  QVERIFY_EXCEPTION_THROWN(scanner.start({}, {80}),std::invalid_argument);
  QVERIFY_EXCEPTION_THROWN(scanner.start({"bad"},{80}),std::invalid_argument);
  QVERIFY_EXCEPTION_THROWN(scanner.start({"127.0.0.1"},{0}),std::invalid_argument);
  QVERIFY_EXCEPTION_THROWN(scanner.start({"127.0.0.1"},{80},0),std::invalid_argument);
 }
};
QTEST_GUILESS_MAIN(ScannerTests)
#include "test_scanner.moc"
