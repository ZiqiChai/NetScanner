#pragma once
#include <QMainWindow>
#include <QHash>
#include <QSet>
#include <QQueue>
#include <QStringList>
class Scanner;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QSpinBox;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QProgressBar;
class QLabel;
class MainWindow : public QMainWindow {
 Q_OBJECT
public:
 explicit MainWindow(QWidget *parent=nullptr);
 ~MainWindow() override;
 bool exportCsv(const QString &path, QString *error=nullptr) const;
 static QString csvField(QString value);
private:
 void beginScan();
 void receiveResult(const QString &,quint16,const QString &,double);
 void filterResults();
 void filterHost(QTreeWidgetItem *item);
 void updateEmptyState();
 void setRunning(bool);
 void saveFavorite();
 void pumpLookups();
 void cancelLookups();
 Scanner *scanner_;
 QLineEdit *targets_,*ports_,*search_;
 QComboBox *mode_,*adapters_,*favorites_;
 QSpinBox *timeout_,*concurrency_;
 QPushButton *start_,*stop_;
 QList<QWidget*> stepControls_;
 QTreeWidget *tree_;
 QProgressBar *progress_;
 QLabel *details_,*summary_,*emptyState_;
 QCheckBox *hideUnresponsive_;
 QHash<QString,QTreeWidgetItem*> hosts_;
 QHash<QString,int> responses_,completed_;
 int portsPerHost_=0;
 quint64 generation_=0;
 QSet<QString> lookups_;
 QQueue<QString> pendingLookups_;
 QHash<QString,int> activeLookups_;
 int online_=0;
 bool discovery_=true;
};
