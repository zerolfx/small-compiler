#ifndef ZPC_HELPER_HPP
#define ZPC_HELPER_HPP

#include <utility>

#include "parser.hpp"


Parser<std::string> lit(const std::string& s) {
  return [=](Scanner& in)->ParseResult<std::string> {
    if (in.starts_with(s)) {
      in.remove_prefix(s.length());
      return s;
    } else {
      return {};
    }
  };
}

template<typename R = char>
Parser<R> ch(const std::function<bool(char)>& f) {
  return [=](Scanner& in)->ParseResult<R> {
    if (in.empty() || !f(in[0])) return {};
    char c = in[0];
    in.remove_prefix(1);
    return c;
  };
}

auto ch_range(char l, char r) {
  return ch([=](char c){ return l <= c && c <= r; });
}


template<typename T, typename R = std::string>
Parser<R> raw(const Parser<T>& p) {
  return [=](Scanner& in)->ParseResult<R> {
    auto in_bak = in;
    if (!p(in)) return {};
    return in_bak.substr(0, in_bak.length() - in.length());
  };
}

template<typename T, typename R = std::vector<T>>
Parser<R> many1(const Parser<T>& p) {
  return [=](Scanner& in)->ParseResult<R> {
    auto v = many(p)(in).value();
    if (v.empty()) return {};
    return v;
  };
}

template<typename T, typename S, typename R = std::vector<T>>
Parser<R> sep_by(const Parser<T>& p, const Parser<S>& s) {
  return seq(p, many(seq(s, p))) %= [](T t, std::vector<std::tuple<S, T>> ts) {
    R r; r.emplace_back(t);
    for (auto& x: ts) r.emplace_back(std::get<1>(x));
    return r;
  };
}

template<typename T>
Parser<T> eof(const Parser<T>& p) {
  return [=](Scanner& in)->ParseResult<T> {
    auto v = p(in);
    if (in.empty()) return v;
    return {};
  };
}


#endif //ZPC_HELPER_HPP
