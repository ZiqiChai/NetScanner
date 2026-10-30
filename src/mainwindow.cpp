#include "mainwindow.h"
#include "scanner.h"
#include "targets.h"
#include <QtWidgets>
#include <QNetworkInterface>
#include <QSaveFile>
#include <QTextStream>
#include <QHostInfo>
#include <QDesktopServices>
#include <QUrl>
#include <algorithm>

namespace {
class NumericTreeItem : public QTreeWidgetItem {
public:
 using QTreeWidgetItem::QTreeWidgetItem;
 bool operator<(const QTreeWidgetItem &other)const override{
  const int column=treeWidget()?treeWidget()->sortColumn():0;
  if(column==0){
   if(parent())return text(0).toUInt()<other.text(0).toUInt();
   return QHostAddress(text(0)).toIPv4Address()<QHostAddress(other.text(0)).toIPv4Address();
  }
  return QTreeWidgetItem::operator<(other);
 }
};
QString serviceScheme(quint16 port){
 switch(port){case 80:case 8080:case 8000:case 8888:return "http";case 443:case 8443:return "https";case 21:return "ftp";default:return {};}
}
QString service(quint16 port) {
 static const QHash<quint16,QString> names={{21,"FTP"},{22,"SSH"},{23,"Telnet"},{25,"SMTP"},{53,"DNS"},{80,"HTTP"},{110,"POP3"},{139,"NetBIOS"},{143,"IMAP"},{443,"HTTPS"},{445,"SMB"},{3389,"RDP"},{5900,"VNC"},{8080,"HTTP"},{8000,"HTTP"},{8888,"HTTP"},{8443,"HTTPS"}};
 return names.value(port,QStringLiteral("未知"));
}
}
MainWindow::MainWindow(QWidget *parent):QMainWindow(parent),scanner_(new Scanner(this)) {
 setWindowTitle("NetScanner"); resize(1280,820);setMinimumSize(1040,720);
 setStyleSheet(R"(
 QMainWindow, QWidget#workspace { background: #f3f6fb; color: #20304a; }
 QWidget { font-size: 13px; color: #20304a; }
 QToolTip { color: #20304a; background: #ffffff; border: 1px solid #c7d3e4; padding: 5px; }
 QScrollArea { background: transparent; border: none; }
 QFrame#brand { background: #142b4f; border-radius: 12px; }
 QLabel#brandTitle { color: #ffffff; font-size: 25px; font-weight: 700; }
 QLabel#brandSubtitle { color: #b9cce8; font-size: 13px; }
 QLabel#brandBadge { color: #c4d9fb; background: #223e64; border-radius: 12px; padding: 7px 12px; }
 QFrame#card { background: #ffffff; border: 1px solid #e1e7ef; border-radius: 12px; }
 QLabel#sectionTitle { color: #1b2c47; font-size: 17px; font-weight: 700; }
 QLabel#fieldLabel { color: #52637c; font-size: 12px; font-weight: 600; }
 QLabel#muted { color: #75849a; font-size: 12px; }
 QLineEdit, QComboBox, QSpinBox { color: #253954; background: #ffffff; border: 1px solid #d7dfeb; border-radius: 6px; padding: 7px 9px; min-height: 20px; selection-background-color: #dceaff; selection-color: #153b70; }
 QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border: 1px solid #377ee8; }
 QLineEdit:disabled, QComboBox:disabled, QSpinBox:disabled { background: #f5f7fa; color: #93a0b2; }
 QComboBox::drop-down { border: none; width: 24px; }
 QComboBox QAbstractItemView { background: white; color: #20304a; selection-background-color: #e8f0ff; selection-color: #164b91; }
 QPushButton { background: #ffffff; border: 1px solid #d7dfeb; border-radius: 6px; color: #40546f; padding: 8px 12px; min-height: 20px; }
 QPushButton:hover { background: #f1f6ff; border-color: #a8c4ee; }
 QPushButton:pressed { background: #e4edfc; }
 QPushButton:disabled { color: #a6b1c2; background: #f5f7fa; border-color: #e5eaf1; }
 QPushButton#start { background: #286ce5; border-color: #286ce5; color: white; font-weight: 700; }
 QPushButton#start:hover { background: #1d5fd5; }
 QPushButton#start:disabled { background: #9ebbea; border-color: #9ebbea; color: #edf4ff; }
 QPushButton[stepButton="true"] { padding: 7px 0; font-size: 17px; font-weight: 600; background: #edf3fd; color: #3064a7; }
 QPushButton[stepButton="true"]:disabled { background: #f5f7fa; color: #a6b1c2; }
 QPushButton#advancedToggle { text-align: left; border: none; background: #f4f7fc; color: #4f6584; }
 QTreeWidget { border: 1px solid #e5eaf2; border-radius: 7px; background: white; alternate-background-color: #f9fbfe; color: #32465f; outline: none; }
 QTreeWidget::item { padding: 7px 4px; border-bottom: 1px solid #f0f3f8; }
 QTreeWidget::item:selected { background: #e8f0fe; color: #174a92; }
 QTreeWidget::branch:selected { background: #e8f0fe; }
 QHeaderView::section { background: #f3f6fb; color: #63748c; border: none; border-bottom: 1px solid #e2e8f1; padding: 10px 7px; font-size: 12px; font-weight: 600; }
 QCheckBox { color: #536882; spacing: 7px; }
 QCheckBox::indicator { width: 16px; height: 16px; }
 QProgressBar { background: #e8eef8; border: none; border-radius: 3px; max-height: 6px; min-height: 6px; }
 QProgressBar::chunk { background: #3c80ee; border-radius: 3px; }
 QLabel#details { background: #f6f8fc; border-radius: 7px; color: #677a94; padding: 10px; }
 QLabel#summary { color: #506783; font-size: 12px; }
 QLabel#emptyState { color: #7b8da6; font-size: 14px; background: transparent; }
 QScrollBar:vertical { background: #f5f7fb; width: 10px; margin: 0; }
 QScrollBar::handle:vertical { background: #c8d3e2; border-radius: 4px; min-height: 24px; }
 QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
 )");
 auto central=new QWidget(this);central->setObjectName("workspace");setCentralWidget(central);
 auto layout=new QVBoxLayout(central);layout->setContentsMargins(22,20,22,18);layout->setSpacing(18);
 auto brand=new QFrame;brand->setObjectName("brand");auto brandLayout=new QHBoxLayout(brand);brandLayout->setContentsMargins(24,18,24,18);
 auto brandText=new QVBoxLayout;brandText->setSpacing(5);
 auto title=new QLabel("NetScanner");title->setObjectName("brandTitle");brandText->addWidget(title);
 auto subtitle=new QLabel("局域网 IP扫描 / Port扫描");subtitle->setObjectName("brandSubtitle");brandText->addWidget(subtitle);
 brandLayout->addLayout(brandText);brandLayout->addStretch();auto badge=new QLabel("NETWORK WORKSPACE");badge->setObjectName("brandBadge");brandLayout->addWidget(badge);layout->addWidget(brand);
 auto body=new QHBoxLayout;body->setSpacing(18);layout->addLayout(body,1);
 auto configCard=new QFrame;configCard->setObjectName("card");configCard->setMinimumWidth(278);auto configScroll=new QScrollArea;configScroll->setObjectName("configScroll");configScroll->setFixedWidth(300);configScroll->setWidgetResizable(true);configScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);configScroll->setWidget(configCard);body->addWidget(configScroll);
 auto config=new QVBoxLayout(configCard);config->setContentsMargins(18,20,18,18);config->setSpacing(10);
 auto section=[](const QString &text){auto label=new QLabel(text);label->setObjectName("sectionTitle");return label;};
 auto field=[config](const QString &text,QWidget *widget){auto label=new QLabel(text);label->setObjectName("fieldLabel");config->addWidget(label);config->addWidget(widget);};
 config->addWidget(section("扫描配置"));auto configHint=new QLabel("选择范围，然后开始发现设备");configHint->setObjectName("muted");config->addWidget(configHint);config->addSpacing(6);
 mode_=new QComboBox;mode_->setObjectName("mode");mode_->addItems({"IP扫描","端口扫描"});field("扫描模式",mode_);
 adapters_=new QComboBox;adapters_->addItem("手动输入目标网段",QString());adapters_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);adapters_->setMinimumContentsLength(12);
 for(const auto &iface:QNetworkInterface::allInterfaces())for(const auto &entry:iface.addressEntries()){
  if(entry.ip().protocol()!=QAbstractSocket::IPv4Protocol || entry.ip().isLoopback() || entry.prefixLength()<0)continue;
  quint32 net=entry.ip().toIPv4Address() & entry.netmask().toIPv4Address();QString subnet=QHostAddress(net).toString()+"/"+QString::number(entry.prefixLength());
  adapters_->addItem(iface.humanReadableName()+" — "+subnet,subnet);
 }
 field("本机网段",adapters_);
 targets_=new QLineEdit("127.0.0.1");targets_->setObjectName("targets");targets_->setPlaceholderText("192.168.1.0/24");targets_->setToolTip("支持 IPv4、CIDR、完整 IP 范围；多个目标用逗号分隔");field("目标 IP / 范围",targets_);
 ports_=new QLineEdit("22,80,443,445,3389,8080");ports_->setObjectName("ports");ports_->setPlaceholderText("22,80,443 或 1-1024");field("TCP 端口",ports_);
 favorites_=new QComboBox;favorites_->setObjectName("favorites");favorites_->addItem("暂无收藏");favorites_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);favorites_->setMinimumContentsLength(12);
 QSettings settings;for(const auto &entry:settings.value("favorites").toStringList())favorites_->addItem(entry);favorites_->setEnabled(favorites_->count()>1);if(favorites_->count()>1)favorites_->setItemText(0,"选择已收藏配置");field("收藏配置",favorites_);
 auto favoriteButton=new QPushButton("＋ 收藏当前配置");favoriteButton->setObjectName("saveFavorite");config->addWidget(favoriteButton);
 auto advancedToggle=new QPushButton("▸ 高级参数");advancedToggle->setObjectName("advancedToggle");advancedToggle->setCheckable(true);config->addWidget(advancedToggle);
 auto advanced=new QWidget;advanced->setObjectName("advancedOptions");auto advancedLayout=new QFormLayout(advanced);advancedLayout->setContentsMargins(0,2,0,2);
 timeout_=new QSpinBox;timeout_->setRange(100,10000);timeout_->setValue(600);timeout_->setSuffix(" ms");concurrency_=new QSpinBox;concurrency_->setRange(1,512);concurrency_->setValue(128);
 auto stepper=[this](QSpinBox *spin,const QString &name,const QString &label){
  auto container=new QWidget;auto row=new QHBoxLayout(container);row->setContentsMargins(0,0,0,0);row->setSpacing(4);
  spin->setObjectName(name);spin->setButtonSymbols(QAbstractSpinBox::NoButtons);spin->setAlignment(Qt::AlignCenter);spin->setMinimumWidth(0);spin->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
  auto minus=new QPushButton("−");auto plus=new QPushButton("+");
  minus->setObjectName(name+"Minus");plus->setObjectName(name+"Plus");minus->setAccessibleName("减少"+label);plus->setAccessibleName("增加"+label);
  for(auto button:{minus,plus}){button->setFixedWidth(30);button->setProperty("stepButton",true);button->setAutoRepeat(true);stepControls_.append(button);}
  row->addWidget(minus);row->addWidget(spin,1);row->addWidget(plus);
  connect(minus,&QPushButton::clicked,spin,&QSpinBox::stepDown);connect(plus,&QPushButton::clicked,spin,&QSpinBox::stepUp);
  return container;
 };
 timeout_->setSingleStep(100);
 advancedLayout->addRow("连接超时",stepper(timeout_,"timeout","连接超时"));advancedLayout->addRow("并发连接",stepper(concurrency_,"concurrency","并发连接"));advanced->setVisible(false);config->addWidget(advanced);
 connect(advancedToggle,&QPushButton::toggled,this,[advanced,advancedToggle](bool checked){advanced->setVisible(checked);advancedToggle->setText(checked?"▾ 高级参数":"▸ 高级参数");});
 config->addStretch(1);
 auto actions=new QHBoxLayout;actions->setSpacing(8);start_=new QPushButton("开始扫描");start_->setObjectName("start");stop_=new QPushButton("停止");stop_->setObjectName("stop");stop_->setEnabled(false);actions->addWidget(start_,1);actions->addWidget(stop_);config->addLayout(actions);
 auto resultsCard=new QFrame;resultsCard->setObjectName("card");body->addWidget(resultsCard,1);auto resultsLayout=new QVBoxLayout(resultsCard);resultsLayout->setContentsMargins(20,20,20,16);resultsLayout->setSpacing(12);
 auto resultHeader=new QHBoxLayout;resultHeader->addWidget(section("扫描结果"));resultHeader->addStretch();auto exportButton=new QPushButton("导出 CSV");exportButton->setToolTip("导出全部扫描记录，包括隐藏的主机");resultHeader->addWidget(exportButton);resultsLayout->addLayout(resultHeader);
 search_=new QLineEdit;search_->setObjectName("search");search_->setClearButtonEnabled(true);search_->setPlaceholderText("搜索 IP、主机名、端口或服务…");resultsLayout->addWidget(search_);
 auto filters=new QHBoxLayout;hideUnresponsive_=new QCheckBox("隐藏无响应主机");hideUnresponsive_->setObjectName("hideUnresponsive");hideUnresponsive_->setChecked(true);hideUnresponsive_->setToolTip("仅显示收到 TCP 开放或关闭响应的主机；取消勾选可查看全部目标");filters->addWidget(hideUnresponsive_);filters->addStretch();
 auto expand=new QPushButton("展开");auto collapse=new QPushButton("收起");filters->addWidget(expand);filters->addWidget(collapse);resultsLayout->addLayout(filters);
 tree_=new QTreeWidget;tree_->setObjectName("results");tree_->setSelectionBehavior(QAbstractItemView::SelectRows);tree_->setSelectionMode(QAbstractItemView::SingleSelection);
 auto resultPalette=tree_->palette();for(auto group:{QPalette::Active,QPalette::Inactive,QPalette::Disabled}){resultPalette.setColor(group,QPalette::Highlight,QColor("#e8f0fe"));resultPalette.setColor(group,QPalette::HighlightedText,QColor("#174a92"));}tree_->setPalette(resultPalette);
 tree_->setHeaderLabels({"IP / 端口","状态","服务（推测）","响应耗时","已完成","主机名","MAC"});tree_->setAlternatingRowColors(true);tree_->setUniformRowHeights(true);tree_->setSortingEnabled(true);tree_->setRootIsDecorated(true);tree_->setIndentation(16);tree_->header()->setSectionResizeMode(QHeaderView::Interactive);tree_->header()->setStretchLastSection(true);tree_->header()->moveSection(2,1);tree_->sortItems(0,Qt::AscendingOrder);
 const int widths[]={142,150,110,100,95,130,145};for(int i=0;i<7;++i)tree_->setColumnWidth(i,widths[i]);
 auto tableArea=new QWidget;auto stack=new QStackedLayout(tableArea);stack->setStackingMode(QStackedLayout::StackAll);stack->setContentsMargins(0,0,0,0);stack->addWidget(tree_);
 emptyState_=new QLabel("准备发现你的局域网\n\n在左侧设置目标，点击“开始扫描”");emptyState_->setObjectName("emptyState");emptyState_->setAlignment(Qt::AlignCenter);emptyState_->setAttribute(Qt::WA_TransparentForMouseEvents);stack->addWidget(emptyState_);stack->setCurrentWidget(emptyState_);resultsLayout->addWidget(tableArea,1);
 connect(expand,&QPushButton::clicked,tree_,&QTreeWidget::expandAll);connect(collapse,&QPushButton::clicked,tree_,&QTreeWidget::collapseAll);
 details_=new QLabel("设备详情  ·  选择一条结果查看详细信息");details_->setObjectName("details");details_->setTextFormat(Qt::PlainText);details_->setWordWrap(true);details_->setTextInteractionFlags(Qt::TextSelectableByMouse);resultsLayout->addWidget(details_);
 summary_=new QLabel("就绪 · 0 台主机有响应");summary_->setObjectName("summary");resultsLayout->addWidget(summary_);
 progress_=new QProgressBar;progress_->setRange(0,1);progress_->setValue(0);progress_->setTextVisible(false);resultsLayout->addWidget(progress_);
 auto note=new QLabel("发现依据：TCP 连接响应。未响应不代表离线；服务名称按端口推测。隐藏仅影响显示，CSV 保留全部结果。");note->setObjectName("muted");note->setWordWrap(true);layout->addWidget(note);
 connect(hideUnresponsive_,&QCheckBox::toggled,this,&MainWindow::filterResults);
 connect(start_,&QPushButton::clicked,this,&MainWindow::beginScan);
 connect(stop_,&QPushButton::clicked,scanner_,&Scanner::stop);
 auto filterTimer=new QTimer(this);filterTimer->setInterval(200);filterTimer->setSingleShot(true);filterTimer->setObjectName("filterTimer");
 connect(filterTimer,&QTimer::timeout,this,&MainWindow::filterResults);
 connect(search_,&QLineEdit::textChanged,this,&MainWindow::filterResults);
 connect(scanner_,&Scanner::result,this,&MainWindow::receiveResult);
 connect(scanner_,&Scanner::progress,this,[this](int done,int total){progress_->setRange(0,total);progress_->setValue(done);summary_->setText(QString("已完成 %1 / %2 次探测 · %3 台主机有响应").arg(done).arg(total).arg(online_));});
 connect(scanner_,&Scanner::finished,this,[this](bool cancelled){setRunning(false);summary_->setText((cancelled?"已停止":"扫描完成")+QString(" · %1 台主机有响应").arg(online_));if(cancelled) for(auto item:hosts_) if(item->text(1)=="探测中") item->setText(1,"已取消 / 未确认");findChild<QTimer*>("filterTimer")->stop();filterResults();});
 connect(adapters_,qOverload<int>(&QComboBox::activated),this,[this](int index){if(index>0) targets_->setText(adapters_->itemData(index).toString());});
 connect(favoriteButton,&QPushButton::clicked,this,&MainWindow::saveFavorite);
 connect(favorites_,qOverload<int>(&QComboBox::activated),this,[this](int index){if(index<1)return;auto p=favorites_->itemText(index).split(" | ");if(p.size()==3){targets_->setText(p[0]);ports_->setText(p[1]);mode_->setCurrentIndex(p[2].toInt());}});
 connect(exportButton,&QPushButton::clicked,this,[this]{auto path=QFileDialog::getSaveFileName(this,"导出扫描结果", "scan.csv","CSV (*.csv)");if(path.isEmpty())return;QString error;if(!exportCsv(path,&error))QMessageBox::warning(this,"导出失败",error);});
 connect(tree_,&QTreeWidget::itemDoubleClicked,this,[this](QTreeWidgetItem *item,int){
  if(!item || !item->parent() || item->data(0,Qt::UserRole).toString()!="open")return;
  bool ok=false;const int port=item->text(0).toInt(&ok);if(!ok || port<1 || port>65535)return;
  const auto scheme=serviceScheme(quint16(port));if(scheme.isEmpty())return;
  const QHostAddress host(item->parent()->text(0));if(host.protocol()!=QAbstractSocket::IPv4Protocol)return;
  QUrl url;url.setScheme(scheme);url.setHost(host.toString());url.setPort(port);url.setPath("/");
  if(!QDesktopServices::openUrl(url))summary_->setText("无法打开服务，请检查系统默认应用设置（"+scheme+"）");
 });
 connect(tree_,&QTreeWidget::currentItemChanged,this,[this](QTreeWidgetItem *item){if(!item)return;
  auto host=item->parent()?item->parent():item;
  details_->setText(QString("设备：%1    主机名：%2    MAC：%3\n状态：%4    端口 / 服务：%5    响应耗时：%6")
   .arg(host->text(0),host->text(5).isEmpty()?"—":host->text(5),host->text(6).isEmpty()?"—":host->text(6),item->text(1),item->parent()?item->text(0)+" / "+item->text(2):"—",item->text(3)));});
}
MainWindow::~MainWindow(){cancelLookups();scanner_->disconnect(this);scanner_->stop();}
void MainWindow::setRunning(bool running){tree_->setSortingEnabled(!running);start_->setEnabled(!running);stop_->setEnabled(running);for(QWidget *w:QList<QWidget*>{targets_,ports_,mode_,adapters_,timeout_,concurrency_})w->setEnabled(!running);favorites_->setEnabled(!running&&favorites_->count()>1);for(auto widget:stepControls_)widget->setEnabled(!running);}
void MainWindow::beginScan(){
 try {
  auto hosts=parseTargets(targets_->text());auto ports=parsePorts(ports_->text());
  std::sort(hosts.begin(),hosts.end(),[](const QString &a,const QString &b){return QHostAddress(a).toIPv4Address()<QHostAddress(b).toIPv4Address();});
  if(qint64(hosts.size())*ports.size()>200000)throw std::invalid_argument("单次最多 20 万次端口探测，请缩小扫描范围");
  cancelLookups();lookups_.clear();tree_->clear();hosts_.clear();responses_.clear();completed_.clear();online_=0;portsPerHost_=ports.size();discovery_=mode_->currentIndex()==0;
  tree_->sortItems(0,Qt::AscendingOrder);tree_->setSortingEnabled(false);
  for(const auto &host:hosts){auto item=new NumericTreeItem(tree_,{host,"探测中","","—","0 / "+QString::number(portsPerHost_)});hosts_.insert(host,item);item->setHidden(hideUnresponsive_->isChecked());}
  filterResults();details_->setText("设备详情  ·  选择一条结果查看详细信息");setRunning(true);scanner_->start(hosts,ports,timeout_->value(),concurrency_->value());
 }catch(const std::exception &e){QMessageBox::warning(this,"扫描参数错误",QString::fromUtf8(e.what()));}
}
void MainWindow::receiveResult(const QString &host,quint16 port,const QString &state,double latency){
 auto item=hosts_.value(host);if(!item)return;
 bool response=state=="open" || state=="closed";
 if(response && responses_[host]++==0){++online_;item->setText(1,"在线");item->setToolTip(1,"已收到 TCP 连接成功或端口关闭响应");item->setForeground(1,QColor("#238636"));}
 int completed=++completed_[host];item->setText(4,QString("%1 / %2").arg(completed).arg(portsPerHost_));
 if(completed==portsPerHost_ && !responses_.value(host))item->setText(1,"未响应 / 未确认");
 QString label=state=="open"?"开放":state=="closed"?"关闭":state=="timeout"?"超时":"连接错误";
 if(!discovery_ || state=="open"){
  auto child=new NumericTreeItem(QStringList{QString::number(port),label,service(port),QString::number(latency,'f',1)+" ms",""});
  child->setData(0,Qt::UserRole,state);
  const auto scheme=serviceScheme(port);
  const QString tip="服务按端口惯例推测，未验证实际协议。"+QString(state=="open"&&!scheme.isEmpty()?" 双击使用系统默认程序打开 "+scheme.toUpper()+" 服务。":"");
  child->setToolTip(0,tip);child->setToolTip(2,tip);
  int low=0,high=item->childCount();while(low<high){int middle=(low+high)/2;if(item->child(middle)->text(0).toUInt()<port)low=middle+1;else high=middle;}item->insertChild(low,child);
 }
 if(response && !lookups_.contains(host)) {
  lookups_.insert(host);pendingLookups_.enqueue(host);pumpLookups();
  QFile arp("/proc/net/arp");if(arp.open(QIODevice::ReadOnly|QIODevice::Text)){while(!arp.atEnd()){auto fields=QString::fromLatin1(arp.readLine()).simplified().split(' ');if(fields.size()>=6 && fields[0]==host && fields[3]!="00:00:00:00:00:00"){bool ok=false;auto flags=fields[2].toUInt(&ok,16);if(ok && (flags&2))item->setText(6,fields[3]);}}}
 }
 if(response)item->setText(3,QString::number(latency,'f',1)+" ms");
 if(search_->text().trimmed().isEmpty())item->setHidden(hideUnresponsive_->isChecked()&&!responses_.value(host));
 if(!item->isHidden())emptyState_->hide();
 auto timer=findChild<QTimer*>("filterTimer");if(!timer->isActive())timer->start();
}

void MainWindow::cancelLookups(){
 ++generation_;pendingLookups_.clear();
 for(int id:activeLookups_)QHostInfo::abortHostLookup(id);
 activeLookups_.clear();
}
void MainWindow::pumpLookups(){
 while(activeLookups_.size()<8 && !pendingLookups_.isEmpty()){
  const auto host=pendingLookups_.dequeue();const auto generation=generation_;
  int id=QHostInfo::lookupHost(host,this,[this,host,generation](const QHostInfo &info){
   if(generation!=generation_)return;
   activeLookups_.remove(host);
   auto item=hosts_.value(host);
   if(item && info.error()==QHostInfo::NoError)item->setText(5,info.hostName());
   auto timer=findChild<QTimer*>("filterTimer");if(!timer->isActive())timer->start();
   pumpLookups();
  });
  activeLookups_.insert(host,id);
 }
}
void MainWindow::filterHost(QTreeWidgetItem *parent){
 const auto query=search_->text().trimmed();
 bool match=false;QString text;
 for(int i=0;i<7;++i)text+=parent->text(i)+" ";
 const bool parentMatch=text.contains(query,Qt::CaseInsensitive);
 for(int i=0;i<parent->childCount();++i){auto child=parent->child(i);QString childText;for(int j=0;j<7;++j)childText+=child->text(j)+" ";bool visible=parentMatch||childText.contains(query,Qt::CaseInsensitive);child->setHidden(!visible);match|=visible;}
 const bool responsive=!hideUnresponsive_->isChecked()||responses_.value(parent->text(0))>0;
 parent->setHidden(!responsive||(!parentMatch&&!match));
 auto selected=tree_->currentItem();
 if(selected && (selected==parent || selected->parent()==parent) && (parent->isHidden() || selected->isHidden())){
  tree_->setCurrentItem(nullptr);details_->setText("设备详情  ·  选择一条结果查看详细信息");
 }
}
void MainWindow::updateEmptyState(){
 bool visible=false;for(auto item:hosts_)if(!item->isHidden()){visible=true;break;}
 emptyState_->setVisible(!visible);
 if(hosts_.isEmpty())emptyState_->setText("准备发现你的局域网\n\n在左侧设置目标，点击“开始扫描”");
 else if(!search_->text().trimmed().isEmpty())emptyState_->setText("没有匹配的主机\n\n尝试其他关键词，或取消隐藏无响应主机");
 else emptyState_->setText("暂未发现有响应的主机\n\n扫描中会自动显示响应设备；取消隐藏可查看全部目标");
}
void MainWindow::filterResults(){for(auto item:hosts_)filterHost(item);updateEmptyState();}
void MainWindow::saveFavorite(){QString entry=targets_->text()+" | "+ports_->text()+" | "+QString::number(mode_->currentIndex());if(favorites_->findText(entry)<0)favorites_->addItem(entry);QStringList entries;for(int i=1;i<favorites_->count();++i)entries<<favorites_->itemText(i);QSettings().setValue("favorites",entries);favorites_->setItemText(0,"选择已收藏配置");favorites_->setEnabled(start_->isEnabled());}
QString MainWindow::csvField(QString value){if(!value.isEmpty()&&QString("=+-@\t\r").contains(value.front()))value.prepend('\'');value.replace('"',"\"\"");return '"'+value+'"';}
bool MainWindow::exportCsv(const QString &path,QString *error)const{
 QSaveFile file(path);if(!file.open(QIODevice::WriteOnly|QIODevice::Text)){if(error)*error=file.errorString();return false;}
 QTextStream out(&file);out<<"IP,Port,Status,Service,Latency,Progress,Hostname,MAC\n";
 for(int i=0;i<tree_->topLevelItemCount();++i){auto host=tree_->topLevelItem(i);auto write=[&](QTreeWidgetItem *item,bool child){QStringList fields={host->text(0),child?item->text(0):QString(),item->text(1),item->text(2),item->text(3),item->text(4),host->text(5),host->text(6)};for(auto &field:fields)field=csvField(field);out<<fields.join(',')<<'\n';};write(host,false);for(int j=0;j<host->childCount();++j)write(host->child(j),true);}
 out.flush();if(out.status()!=QTextStream::Ok || !file.commit()){if(error)*error=file.errorString();return false;}return true;
}
