#include <QApplication>
#include <QIcon>
#include <QSettings>
#include "mainwindow.h"
int main(int argc,char **argv) {
 QApplication app(argc,argv);
 app.setOrganizationName("NetScanner");
 app.setApplicationName("NetScanner");
 // Preserve existing NetScanner settings, including an explicitly empty favorites list.
 QSettings settings;
 if(!settings.contains("favorites")) {
  QSettings legacySettings("LanScanner", "LAN Scanner");
  if(legacySettings.contains("favorites")) {
   settings.setValue("favorites", legacySettings.value("favorites"));
   settings.sync();
  }
 }
 app.setApplicationVersion("0.1.0");
 app.setDesktopFileName("io.github.ZiqiChai.NetScanner");
 app.setWindowIcon(QIcon::fromTheme("io.github.ZiqiChai.NetScanner"));
 MainWindow window; window.show();
 return app.exec();
}
