//
// Created by nudelerde on 05.06.23.
//

#pragma once

namespace cr::compiletime {

#ifndef NDEBUG
constexpr bool debug = true;
#else
constexpr bool debug = false;
#endif

}// namespace cr::compiletime