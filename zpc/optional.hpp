#ifndef ZPC_OPTIONAL_HPP
#define ZPC_OPTIONAL_HPP

#include <optional>

template<typename T>
class optional : public std::optional<T> {
public:
  optional() : std::optional<T>() {}
  template<typename Q>
  optional(Q&& v) : std::optional<T>(std::forward<Q>(v)) {}

  using std::optional<T>::has_value;
  using std::optional<T>::value;
  using std::optional<T>::value_or;
  using std::optional<T>::operator bool;

  template<typename F, typename Q = std::invoke_result_t<F, T>>
  auto map(F&& f) const { // f: T => ?
    if (!has_value()) return optional<Q>();
    return optional<Q>(f(value()));
  }

  template<typename F, typename Q = std::invoke_result_t<F, T>>
  auto flat_map(F&& f) const { // f : T => optional<?>
    if (!has_value()) return Q();
    return f(value());
  }

  auto or_else(const optional<T>& alternative) const {
    if (!has_value()) return alternative;
    return *this;
  }
};

#endif //ZPC_OPTIONAL_HPP
