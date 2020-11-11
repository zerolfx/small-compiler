#ifndef ZPC_UNIQUE_VARIANT_HPP
#define ZPC_UNIQUE_VARIANT_HPP

#include <variant>

template<typename T, typename... List>
constexpr bool is_in_list = (std::is_same_v<T, List> || ...);

template<typename...>
struct variant_concat;

template<typename T, typename... Ts>
struct variant_concat<T, std::variant<Ts...>> {
  using type = std::variant<T, Ts...>;
};

template<typename T, typename... Ts>
struct unique_variant {
  using type = std::conditional_t<
      is_in_list<T, Ts...>,
      typename unique_variant<Ts...>::type,
      typename variant_concat<T, typename unique_variant<Ts...>::type>::type
  >;
};

template<typename T>
struct unique_variant<T> {
  using type = std::variant<T>;
};

template<typename... Ts>
using unique_variant_t = typename unique_variant<Ts...>::type;

#endif //ZPC_UNIQUE_VARIANT_HPP
