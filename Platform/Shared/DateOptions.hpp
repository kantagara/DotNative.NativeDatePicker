#pragma once
#include <DotNativePlugins.hpp>
#include <tuple>
#include <cstdio>

namespace dotnative_dates {
struct Date {
  int y, m, d;
  friend bool operator<(Date a, Date b) { return std::tie(a.y,a.m,a.d)<std::tie(b.y,b.m,b.d); }
  friend bool operator>(Date a, Date b) { return b<a; }
};
inline int Days(int year,int month) {
  static const int counts[]={31,28,31,30,31,30,31,31,30,31,30,31};
  return counts[month-1]+(month==2 && year%4==0 && (year%100!=0 || year%400==0));
}
inline Date Parse(const std::string &text) {
  int y, m, d; char extra;
  if (text.size() != 10 || std::sscanf(text.c_str(), "%d-%d-%d%c", &y, &m, &d, &extra) != 3)
    throw std::invalid_argument("Invalid calendar date");
  Date date{y,m,d};
  if (y < 1 || y > 9999 || m < 1 || m > 12 || d < 1 || d > Days(y,m)) throw std::invalid_argument("Invalid calendar date");
  return date;
}
inline std::string Format(Date date) {
  char text[11]; std::snprintf(text, sizeof(text), "%04d-%02u-%02u", date.y, unsigned(date.m), unsigned(date.d)); return text;
}
struct Options {
  Date minimum, maximum, initial;
  std::string title, confirm, cancel;
  explicit Options(const dotnative::Value &value) {
    const auto &map = value.As<dotnative::Value::Map>();
    auto text = [&](const char *key) { return map.at(key).As<std::string>(); };
    minimum=Parse(text("minimumDate")); maximum=Parse(text("maximumDate")); initial=Parse(text("initialDate"));
    title=text("title"); confirm=text("confirmText"); cancel=text("cancelText");
    if (initial < minimum || initial > maximum) throw std::invalid_argument("Initial date is outside the bounds");
  }
};
}
