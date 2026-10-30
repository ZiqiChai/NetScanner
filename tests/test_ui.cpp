#include <QtTest>
#include <QTcpServer>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QComboBox>
#include <QTemporaryDir>
#include <QFile>
#include <QSettings>
#include <QCheckBox>
#include <QLabel>
#include <QScrollArea>
#include <QSpinBox>
#include <QDesktopServices>
#include <QUrl>
#include "scanner.h"
#include "mainwindow.h"
class UrlCapture : public QObject {
 Q_OBJECT
public:
 QList<QUrl> urls;
 UrlCapture(){for(const auto &scheme:{"http","https","ftp"})QDesktopServices::setUrlHandler(scheme,this,"capture");}
 ~UrlCapture(){for(const auto &scheme:{"http","https","ftp"})QDesktopServices::unsetUrlHandler(scheme);}
public slots:
 void capture(const QUrl &url){urls.append(url);}
};
class UiTests : public QObject {
 Q_OBJECT
private slots:
 void initTestCase() { QCoreApplication::setOrganizationName("NetScannerTests"); QCoreApplication::setApplicationName("UI"); QSettings().clear(); }
 void cleanupTestCase() { QSettings().clear(); }
 void doubleClickOpenService() {
  UrlCapture capture;MainWindow window;
  window.findChild<QComboBox*>("mode")->setCurrentIndex(1);
  window.findChild<QLineEdit*>("targets")->setText("127.0.0.1");window.findChild<QLineEdit*>("ports")->setText("443,8080,21,80,8000,8888,8443,12345");
  window.findChild<QPushButton*>("start")->click();window.findChild<QPushButton*>("stop")->click();
  auto scanner=window.findChild<Scanner*>();auto tree=window.findChild<QTreeWidget*>("results");
  auto host=tree->topLevelItem(0);
  tree->itemDoubleClicked(host,0);QVERIFY(capture.urls.isEmpty());
  const QList<quint16> ports={443,8080,21,8000,8888,8443};
  for(auto port:ports){scanner->result("127.0.0.1",port,"open",1);auto child=tree->findItems(QString::number(port),Qt::MatchExactly|Qt::MatchRecursive).first();tree->itemDoubleClicked(child,2);}
  QCOMPARE(capture.urls,QList<QUrl>({QUrl("https://127.0.0.1:443/"),QUrl("http://127.0.0.1:8080/"),QUrl("ftp://127.0.0.1:21/"),QUrl("http://127.0.0.1:8000/"),QUrl("http://127.0.0.1:8888/"),QUrl("https://127.0.0.1:8443/")}));
  scanner->result("127.0.0.1",80,"closed",1);scanner->result("127.0.0.1",12345,"open",1);
  tree->itemDoubleClicked(tree->findItems("80",Qt::MatchExactly|Qt::MatchRecursive).first(),0);
  tree->itemDoubleClicked(tree->findItems("12345",Qt::MatchExactly|Qt::MatchRecursive).first(),0);
  scanner->result("127.0.0.1",443,"timeout",600);
  for(int i=0;i<host->childCount();++i)if(host->child(i)->data(0,Qt::UserRole).toString()=="timeout")tree->itemDoubleClicked(host->child(i),0);
  QCOMPARE(capture.urls.size(),6);
 }
 void consistentRowSelection(){
  MainWindow window;auto tree=window.findChild<QTreeWidget*>("results");
  QCOMPARE(tree->selectionBehavior(),QAbstractItemView::SelectRows);QCOMPARE(tree->selectionMode(),QAbstractItemView::SingleSelection);
  QCOMPARE(tree->palette().color(QPalette::Active,QPalette::Highlight),QColor("#e8f0fe"));QCOMPARE(tree->palette().color(QPalette::Inactive,QPalette::Highlight),QColor("#e8f0fe"));
 }
 void numericOrderAndLabels() {
  MainWindow window;QCOMPARE(window.windowTitle(),QString("NetScanner"));
  auto mode=window.findChild<QComboBox*>("mode");QCOMPARE(mode->itemText(0),QString("IP扫描"));QCOMPARE(mode->itemText(1),QString("端口扫描"));mode->setCurrentIndex(1);
  window.findChild<QLineEdit*>("targets")->setText("127.0.0.10,127.0.0.2,127.0.0.1");
  window.findChild<QLineEdit*>("ports")->setText("100,22,80");
  window.findChild<QPushButton*>("start")->click();
  auto tree=window.findChild<QTreeWidget*>("results");
  QCOMPARE(tree->topLevelItem(0)->text(0),QString("127.0.0.1"));QCOMPARE(tree->topLevelItem(1)->text(0),QString("127.0.0.2"));QCOMPARE(tree->topLevelItem(2)->text(0),QString("127.0.0.10"));
  auto scanner=window.findChild<Scanner*>();scanner->result("127.0.0.1",100,"open",1);scanner->result("127.0.0.1",22,"open",1);scanner->result("127.0.0.1",80,"open",1);
  auto host=tree->topLevelItem(0);QCOMPARE(host->text(1),QString("在线"));QVERIFY(host->text(2).isEmpty());QVERIFY(host->toolTip(1).contains("TCP"));
  QCOMPARE(host->child(0)->text(0),QString("22"));QCOMPARE(host->child(1)->text(0),QString("80"));QCOMPARE(host->child(2)->text(0),QString("100"));QCOMPARE(host->child(0)->text(2),QString("SSH"));
  window.findChild<QPushButton*>("stop")->click();QCOMPARE(tree->topLevelItem(1)->text(0),QString("127.0.0.2"));QCOMPARE(host->child(1)->text(0),QString("80"));
 }
 void emptyFavoritesAndStepControls() {
  QSettings().clear();MainWindow window;
  auto favorites=window.findChild<QComboBox*>("favorites");QCOMPARE(favorites->currentText(),QString("暂无收藏"));QVERIFY(!favorites->isEnabled());
  auto spin=window.findChild<QSpinBox*>("timeout");auto minus=window.findChild<QPushButton*>("timeoutMinus");auto plus=window.findChild<QPushButton*>("timeoutPlus");QVERIFY(spin);QVERIFY(minus);QVERIFY(plus);QVERIFY(!plus->accessibleName().isEmpty());
  int value=spin->value();plus->click();QCOMPARE(spin->value(),value+spin->singleStep());minus->click();QCOMPARE(spin->value(),value);
  window.findChild<QPushButton*>("start")->click();QVERIFY(!plus->isEnabled());QVERIFY(!minus->isEnabled());window.findChild<QPushButton*>("stop")->click();QVERIFY(plus->isEnabled());QVERIFY(!favorites->isEnabled());
  window.findChild<QPushButton*>("saveFavorite")->click();QVERIFY(favorites->isEnabled());QSettings().clear();
 }
 void responseVisibilityAndSearch() {
  MainWindow window;
  auto hide=window.findChild<QCheckBox*>("hideUnresponsive");QVERIFY(hide);QVERIFY(hide->isChecked());
  window.findChild<QLineEdit*>("targets")->setText("127.0.0.1-127.0.0.3");
  window.findChild<QLineEdit*>("ports")->setText("80");
  window.findChild<QPushButton*>("start")->click();
  auto tree=window.findChild<QTreeWidget*>("results");QCOMPARE(tree->topLevelItemCount(),3);
  for(int i=0;i<3;++i)QVERIFY(tree->topLevelItem(i)->isHidden());
  window.findChild<QPushButton*>("stop")->click();
  hide->setChecked(false);for(int i=0;i<3;++i)QVERIFY(!tree->topLevelItem(i)->isHidden());
  hide->setChecked(true);
  auto scanner=window.findChild<Scanner*>();
  scanner->result("127.0.0.1",80,"open",1.0);
  scanner->result("127.0.0.2",80,"closed",1.0);
  scanner->result("127.0.0.3",80,"timeout",600.0);
  auto first=tree->findItems("127.0.0.1",Qt::MatchExactly).first();
  auto second=tree->findItems("127.0.0.2",Qt::MatchExactly).first();
  auto third=tree->findItems("127.0.0.3",Qt::MatchExactly).first();
  QVERIFY(!first->isHidden());QVERIFY(!second->isHidden());QVERIFY(third->isHidden());
  tree->setCurrentItem(first);
  auto search=window.findChild<QLineEdit*>("search");search->setText("127.0.0.3");
  QVERIFY(!tree->currentItem());QVERIFY(!window.findChild<QLabel*>("details")->text().contains("127.0.0.1"));
  QVERIFY(first->isHidden());QVERIFY(second->isHidden());QVERIFY(third->isHidden());
  hide->setChecked(false);QVERIFY(!third->isHidden());QVERIFY(first->isHidden());
  hide->setChecked(true);search->setText("HTTP");QVERIFY(!first->isHidden());QVERIFY(third->isHidden());
 }
 void advancedFitsSmallWindow() {
  MainWindow window;window.resize(1040,720);window.show();
  window.findChild<QPushButton*>("advancedToggle")->click();QTest::qWait(20);
  QVERIFY(window.height()<=720);
  auto scroll=window.findChild<QScrollArea*>("configScroll");QVERIFY(scroll);QVERIFY(scroll->widget()->height()>0);
 }
 void cancellationAndRestart() {
  MainWindow window;
  auto input=window.findChild<QLineEdit*>("targets"); input->setText("127.0.0.1");
  window.findChild<QLineEdit*>("ports")->setText("1-5000");
  auto start=window.findChild<QPushButton*>("start");
  auto stop=window.findChild<QPushButton*>("stop");
  start->click(); QVERIFY(!input->isEnabled()); stop->click();
  QVERIFY(start->isEnabled()); QVERIFY(input->isEnabled());
  QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost));
  window.findChild<QLineEdit*>("ports")->setText(QString::number(server.serverPort()));
  start->click(); QTRY_VERIFY_WITH_TIMEOUT(start->isEnabled(),5000);
  QCOMPARE(window.findChild<QTreeWidget*>("results")->topLevelItem(0)->text(1),QString("在线"));
 }
 void favoritesPersist() {
  { MainWindow window;window.findChild<QLineEdit*>("targets")->setText("10.20.30.40");window.findChild<QPushButton*>("saveFavorite")->click(); }
  MainWindow restored; auto favorites=restored.findChild<QComboBox*>("favorites");
  QCOMPARE(favorites->count(),2); QVERIFY(favorites->itemText(1).startsWith("10.20.30.40 | "));
 }
 void finishedResultsRespectFilter() {
  MainWindow window; auto tree=window.findChild<QTreeWidget*>("results");
  window.findChild<QLineEdit*>("search")->setText("does-not-match");
  window.findChild<QLineEdit*>("targets")->setText("127.0.0.1");
  window.findChild<QLineEdit*>("ports")->setText("1");
  auto start=window.findChild<QPushButton*>("start");start->click();
  QTRY_VERIFY_WITH_TIMEOUT(start->isEnabled(),5000);
  QVERIFY(tree->topLevelItem(0)->isHidden());
 }
 void destroyWhileScanning() {
  auto window=new MainWindow;
  window->findChild<QLineEdit*>("targets")->setText("127.0.0.1");
  window->findChild<QLineEdit*>("ports")->setText("1-20000");
  window->findChild<QPushButton*>("start")->click();
  QTest::qWait(1);delete window;QTest::qWait(20);
 }
 void exportFailure() { MainWindow window; QString error; QVERIFY(!window.exportCsv("/no-such-lanscanner-directory/output.csv",&error)); QVERIFY(!error.isEmpty()); }
 void scanAndExport() {
  QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost));
  MainWindow window;
  window.findChild<QLineEdit*>("targets")->setText("127.0.0.1");
  window.findChild<QComboBox*>("mode")->setCurrentIndex(1);
  window.findChild<QLineEdit*>("ports")->setText(QString::number(server.serverPort()));
  auto start=window.findChild<QPushButton*>("start"); start->click();
  QTRY_VERIFY_WITH_TIMEOUT(start->isEnabled(),5000);
  auto tree=window.findChild<QTreeWidget*>("results");
  QCOMPARE(tree->topLevelItemCount(),1);
  QCOMPARE(tree->topLevelItem(0)->text(0),QString("127.0.0.1"));
  QCOMPARE(tree->topLevelItem(0)->childCount(),1);
  QCOMPARE(tree->topLevelItem(0)->child(0)->text(1),QString("开放"));
  if(qEnvironmentVariableIsSet("NETSCANNER_SCREENSHOT")) { tree->expandAll();tree->setCurrentItem(tree->topLevelItem(0)->child(0));window.show(); QTest::qWait(100); window.grab().save(qEnvironmentVariable("NETSCANNER_SCREENSHOT"));window.findChild<QPushButton*>("advancedToggle")->click();QTest::qWait(50);auto advancedPath=qEnvironmentVariable("NETSCANNER_SCREENSHOT");advancedPath.replace(".png","-advanced.png");window.grab().save(advancedPath); }
  QTemporaryDir dir; QVERIFY(dir.isValid());
  QString error; QVERIFY(window.exportCsv(dir.filePath("result.csv"),&error));
  QFile f(dir.filePath("result.csv")); QVERIFY(f.open(QIODevice::ReadOnly));
  auto bytes=f.readAll(); QVERIFY(bytes.contains("127.0.0.1")); QVERIFY(bytes.contains(QByteArray::number(server.serverPort())));
  window.findChild<QLineEdit*>("search")->setText("does-not-match");
  QVERIFY(tree->topLevelItem(0)->isHidden());
 }
 void quoteCsv() { QCOMPARE(MainWindow::csvField("=1+1"),QString("\"'=1+1\"")); QCOMPARE(MainWindow::csvField("a,\"b\"\nc"),QString("\"a,\"\"b\"\"\nc\"")); }
};
QTEST_MAIN(UiTests)
#include "test_ui.moc"
