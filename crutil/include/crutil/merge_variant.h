//
// Created by nudelerde on 03.06.23.
//

#pragma once

#include <variant>

namespace cr::util {

namespace impl {
template<typename T, typename... Args>
struct concatenator;

template<typename... Args0, typename... Args1>
struct concatenator<std::variant<Args0...>, std::variant<Args1...>> {
    using type = std::variant<Args0..., Args1...>;
};

template<typename... V>
struct multiconcat;

template<typename V1>
struct multiconcat<V1> {
    using type = V1;
};

template<typename V, typename... Vs>
struct multiconcat<V, Vs...> {
    using type = typename concatenator<V, typename multiconcat<Vs...>::type>::type;
};


template<typename V1, typename V2>
using concat = typename concatenator<V1, V2>::type;

template<typename... V>
using multicat = typename multiconcat<V...>::type;

}// namespace impl

template<typename... V>
using merge_variant = impl::multicat<V...>;

}// namespace cr::util