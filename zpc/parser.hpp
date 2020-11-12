#ifndef ZPC_PARSER_HPP
#define ZPC_PARSER_HPP

#include <string_view>
#include <cassert>
#include <functional>
#include <string>
#include <iostream>

#include "unique_variant.hpp"
#include "optional.hpp"

template<typename T>
using ParseResult = optional<T>;


template<typename... Ts>
class TD;

//template<typename T>
//using Parser = std::function<ParseResult<T>(Scanner&)>;

class Scanner: std::string_view {
public:
  explicit Scanner(auto&& s): std::string_view(s) {}
  using std::string_view::remove_prefix;
  using std::string_view::starts_with;
  using std::string_view::size;
  using std::string_view::substr;
  using std::string_view::length;
  using std::string_view::empty;
  using std::string_view::operator[];
  friend std::ostream& operator << (std::ostream& os, const Scanner& s);

  bool skip = true;
};

std::ostream& operator << (std::ostream& os, const Scanner& s) {
  return os << s.data();
}

template<typename T>
class Parser;

template<typename T>
class Parser {
public:
  template<typename F>
  Parser (F&& f)  requires (!std::is_same_v<std::decay_t<F>, Parser<T>>) : p(std::forward<F>(f))  {}

  Parser() {}

  ParseResult<T> operator () (Scanner& in) const;

  Parser<T>&& atom() && {
    skip_ = false;
    return static_cast<Parser<T>&&>(*this);
  }
private:
  std::function<ParseResult<T>(Scanner&)> p;
  bool skip_ = true;
};


template<typename F, typename T, typename R = std::remove_reference_t<std::invoke_result_t<F, T>>>
Parser<R> operator % (const Parser<T>& p, F&& f) {
  return [=](Scanner& in)->ParseResult<R> {
    return p(in).map([&](auto&& v) { return f(v); });
  };
}

template<typename F, typename... Ts, typename R = std::remove_reference_t<std::invoke_result_t<F, Ts...>>>
Parser<R> operator %= (const Parser<std::tuple<Ts...>>& p, F&& f) {
  return [=](Scanner& in)->ParseResult<R> {
    return p(in).map([&](auto&& v) { return std::apply(f, v); });
  };
}

template<typename F, typename T>
Parser<T> operator /= (const Parser<T>& p, F&& f) {
  return [=](Scanner& in)->ParseResult<T> {
    return p(in).flat_map([&](auto&& v)->optional<T> { if (f(v)) return v; else return {}; });
  };
}

#define RESOLVE_OVERLOAD(...) \
	[](auto&&...args)->decltype(auto){return __VA_ARGS__(std::forward<decltype(args)>(args)...);}


Parser<std::tuple<>> seq() {
  return [](Scanner&)->ParseResult<std::tuple<>> {
    return std::make_tuple();
  };
}

template<typename T, typename... Ts, typename R = std::tuple<T, Ts...>>
Parser<R> seq(const Parser<T>& p, const Parser<Ts>& ...ps) {
  return [=](Scanner& in)->ParseResult<R> {
    return p(in).flat_map([&](auto v) {
      return seq(ps...)(in).map([&](auto vs) {
        return std::tuple_cat(std::make_tuple(v), vs);
      });
    });
  };
}

template<typename T>
Parser<T> attempt(const Parser<T>& p) {
  return [=](Scanner& in)->ParseResult<T> {
    auto in_bak = in;
    auto v = p(in);
    if (!v) in = in_bak;
    return v;
  };
}

namespace detail {
  template<typename R>
  Parser<R> alt() {
    return [](Scanner&)->ParseResult<R> {
      return {};
    };
  }

  template<typename R, typename T, typename... Ts>
  Parser<R> alt(const Parser<T>& p, const Parser<Ts>& ...ps) {
    return [=](Scanner& in)->ParseResult<R> {
      auto res = attempt(p)(in).map([](auto v){ return R{v}; });
      if (!res) res = alt<R>(ps...)(in);
      return res;
    };
  }

  template<typename... Ts>
  auto flat(const std::variant<Ts...>& v) {
    if constexpr (sizeof...(Ts) == 1) {
      return std::get<0>(v);
    } else {
      return v;
    }
  }
}

template<typename... Ts>
auto alt(const Parser<Ts>& ...ps) {
  return detail::alt<unique_variant_t<Ts...>>(ps...) % RESOLVE_OVERLOAD(detail::flat);
}

template<typename T, typename R = std::vector<T>>
Parser<R> many(const Parser<T>& p) {
  return [=](Scanner& in)->ParseResult<R> {
    R r;
    ParseResult<T> v;
    while ((v = attempt(p)(in))) {
      r.emplace_back(v.value());
    }
    return r;
  };
}

template<typename T>
Parser<T> lazy(const Parser<T>& p) {
  return [p_addr = &p](Scanner& in)->ParseResult<T> {
    return (*p_addr)(in);
  };
}

template<typename T, typename R = optional<T>>
Parser<R> opt(const Parser<T>& p) {
  return [=](Scanner& in)->ParseResult<R> {
    return p(in);
  };
}


// ========================= helper =========================


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

struct Nothing {} nothing;

template<typename T>
Parser<Nothing> and_predicate(const Parser<T>& p) {
  return [=](Scanner& in)->ParseResult<Nothing> {
    auto in_bak = in;
    auto v = p(in);
    in = in_bak;
    if (v) return nothing; else return {};
  };
}

template<typename T>
Parser<Nothing> not_predicate(const Parser<T>& p) {
  return [=](Scanner& in)->ParseResult<Nothing> {
    auto in_bak = in;
    auto v = p(in);
    in = in_bak;
    if (!v) return nothing; else return {};
  };
}


// ========================= end =========================

template<typename T>
ParseResult<T> Parser<T>::operator () (Scanner& in) const {
  bool flip = !skip_ && in.skip;
  if (flip) in.skip = !in.skip;
  if (in.skip) {
    auto any_char = ch([](char){ return true; });
    while (alt(
      ch(isspace),
      seq(
        lit("/*"),
        many(seq(not_predicate(lit("*/")), any_char)),
        lit("*/")
      ),
      seq(
        lit("//"), many(seq(not_predicate(lit("\n")), any_char))
      )
    ).atom()(in));
  }

  auto r = p(in);
  if (flip) in.skip = !in.skip;
  return r;
}


#endif //ZPC_PARSER_HPP
