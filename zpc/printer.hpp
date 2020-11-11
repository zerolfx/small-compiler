#ifndef ZPC_PRINTER_HPP
#define ZPC_PRINTER_HPP

#include <variant>
#include <vector>
#include <tuple>
#include <iostream>
#include <memory>

#include "optional.hpp"

//int indent = 0;
//struct Indent {} ind;
//
//std::ostream& operator << (std::ostream& os, Indent) {
//  return os << std::string(indent, ' ');
//}

namespace detail {
  template<std::size_t... Indices>
  std::ostream& print_tuple(std::ostream& os, auto&& v, std::index_sequence<Indices...>) {
    ((os << (Indices == 0 ? "" : ", ") << std::get<Indices>(v)), ...);
    return os;
  }
}

template<typename... Ts>
std::ostream& operator << (std::ostream& os, const std::tuple<Ts...>& t) {
  os << "(";
  detail::print_tuple(os, t, std::index_sequence_for<Ts...>{});
  return os << ")";
}

template<typename T>
std::ostream& operator << (std::ostream& os, const std::vector<T>& v) {
  os << "[";
  for (size_t i = 0; i < v.size(); ++i) os << (i ? ", " : "") << v[i];
  return os << "]";
}

template<typename T, typename... Ts>
std::ostream& operator << (std::ostream& os, const std::variant<T, Ts...>& v) {
  os << "V{";
  std::visit([&os](auto&& x){ os << x; }, v);
  return os << "}";
}

template<typename T>
std::ostream& operator << (std::ostream& os, const optional<T>& v) {
  if (!v.has_value()) return os << "None";
  return os << "?:" << v.value();
}


//std::ostream& operator << (std::ostream& os, const std::string& v) {
//  return os << '"' << v.c_str() << '"';
//}

template<typename T>
std::ostream& operator << (std::ostream& os, const std::shared_ptr<T>& p) {
  return os << *p;
}

#endif //ZPC_PRINTER_HPP
