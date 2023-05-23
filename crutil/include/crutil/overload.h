//
// Created by nudelerde on 22.05.23.
//

#pragma once

namespace cr::util {

    template<class... Ts>
    struct overloaded : Ts ... {
        using Ts::operator()...;
    };

    template<class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;


}
