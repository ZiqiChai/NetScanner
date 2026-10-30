#include <QtTest>
#include "targets.h"
class TargetTests : public QObject {
 Q_OBJECT
private slots:
 void addresses() {
  QCOMPARE(parseTargets("192.168.1.1,192.168.1.1").size(), 1);
  QCOMPARE(parseTargets("192.168.1.0/30"), QStringList({"192.168.1.1", "192.168.1.2"}));
  QCOMPARE(parseTargets("10.0.0.1-10.0.0.3").size(), 3);
  QCOMPARE(parseTargets("127.0.0.1/32").size(), 1);
  QCOMPARE(parseTargets("10.0.0.0/31").size(), 2);
 }
 void invalid() {
  for (auto text : {"", "garbage", "::1", "10.0.0.0/0", "10.0.0.3-10.0.0.1", "1.2.3.999", "1.2.3.4/33"})
   QVERIFY_EXCEPTION_THROWN(parseTargets(text), std::invalid_argument);
 }
 void strictDecimalInput() {
  for (auto text : {"010.0.0.1", "0x7f.0.0.1", "127.1", "1.2.3.4/+32", "1.2.3.4/032", "1.2.3.04"})
   QVERIFY_EXCEPTION_THROWN(parseTargets(text), std::invalid_argument);
  for (auto text : {"+80", "22-+80", "080", "0x50"})
   QVERIFY_EXCEPTION_THROWN(parsePorts(text), std::invalid_argument);
 }
 void separators() {
  QCOMPARE(parseTargets(" 127.0.0.1,,; 127.0.0.2\n"), QStringList({"127.0.0.1","127.0.0.2"}));
  QCOMPARE(parsePorts("22,,; 80\n"), QList<quint16>({22,80}));
 }
 void boundaries() {
  QCOMPARE(parseTargets("255.255.255.255/32"),QStringList({"255.255.255.255"}));
  QCOMPARE(parseTargets("10.0.0.0-10.0.15.255").size(),4096);
  QVERIFY_EXCEPTION_THROWN(parseTargets("10.0.0.0-10.0.16.0"),std::invalid_argument);
  QCOMPARE(parsePorts("1,65535"),QList<quint16>({1,65535}));
 }
 void portErrorContext() {
  try { parsePorts("65536"); QFAIL("Expected invalid port"); }
  catch(const std::invalid_argument &error) {
   QVERIFY(QString::fromUtf8(error.what()).contains(QStringLiteral("端口")));
   QVERIFY(!QString::fromUtf8(error.what()).contains("4096"));
  }
 }
 void ports() {
  QCOMPARE(parsePorts("22,80,8000-8002,22"), QList<quint16>({22,80,8000,8001,8002}));
  for (auto text : {"", "0", "65536", "80-22", "abc"})
   QVERIFY_EXCEPTION_THROWN(parsePorts(text), std::invalid_argument);
 }
};
QTEST_GUILESS_MAIN(TargetTests)
#include "test_targets.moc"
