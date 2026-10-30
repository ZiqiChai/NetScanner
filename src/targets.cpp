#include "targets.h"
#include <QHostAddress>
#include <QRegularExpression>
#include <QSet>
namespace {
[[noreturn]] void invalidTarget() { throw std::invalid_argument("IP 输入无效，或超过 4096 个主机上限"); }
[[noreturn]] void invalidPort() { throw std::invalid_argument("端口输入无效；请使用 1–65535 的十进制端口或升序范围"); }
// Require canonical decimal spelling. QHostAddress also accepts historical
// octal/hexadecimal forms, which must never silently change the scan target.
bool decimal(const QString &text) {
 static const QRegularExpression expression("^(0|[1-9][0-9]*)$");
 return expression.match(text).hasMatch();
}
quint32 address(const QString &text) {
 const auto octets=text.split('.');
 if(octets.size()!=4) invalidTarget();
 quint32 value=0;
 for(const auto &octet:octets) {
  bool ok=false; const uint number=octet.toUInt(&ok,10);
  if(!decimal(octet) || !ok || number>255) invalidTarget();
  value=(value<<8)|number;
 }
 return value;
}
QStringList tokens(const QString &text) {
 // Repeated separators and surrounding whitespace are intentionally allowed.
 return text.trimmed().split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts);
}
}
QStringList parseTargets(const QString &text) {
 QStringList out; QSet<quint32> seen;
 const auto entries=tokens(text); if(entries.isEmpty()) invalidTarget();
 for(const auto &token:entries) {
  quint64 first, last;
  if(token.contains('/')) {
   auto p=token.split('/'); if(p.size()!=2 || !decimal(p[1])) invalidTarget();
   bool ok=false; int prefix=p[1].toInt(&ok); if(!ok || prefix<0 || prefix>32) invalidTarget();
   quint32 ip=address(p[0]); quint64 count=quint64(1)<<(32-prefix);
   first=quint64(ip)&(~(count-1)&0xffffffffULL); last=first+count-1;
   if(prefix<31) { ++first; --last; }
  } else if(token.contains('-')) {
   auto p=token.split('-'); if(p.size()!=2) invalidTarget();
   first=address(p[0]); last=address(p[1]);
  } else { first=last=address(token); }
  if(last<first || last-first+1>4096) invalidTarget();
  for(quint64 ip=first;ip<=last;++ip) {
   if(!seen.contains(quint32(ip))) { seen.insert(quint32(ip)); out<<QHostAddress(quint32(ip)).toString(); }
   if(out.size()>4096) invalidTarget();
  }
 }
 return out;
}
QList<quint16> parsePorts(const QString &text) {
 QSet<quint16> seen; QList<quint16> out;
 const auto entries=tokens(text); if(entries.isEmpty()) invalidPort();
 for(const auto &token:entries) {
  auto p=token.split('-'); if(p.size()>2 || !decimal(p[0])) invalidPort();
  bool ok=false; int first=p[0].toInt(&ok); if(!ok || first<1 || first>65535) invalidPort();
  int last=first;
  if(p.size()==2) {
   if(!decimal(p[1])) invalidPort();
   last=p[1].toInt(&ok); if(!ok || last<first || last>65535) invalidPort();
  }
  for(int port=first;port<=last;++port) if(!seen.contains(port)) { seen.insert(port); out<<quint16(port); }
 }
 return out;
}
