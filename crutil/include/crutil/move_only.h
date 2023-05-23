//
// Created by nudelerde on 22.05.23.
//

namespace cr::util {
    struct move_only {
        move_only() = default;

        move_only(const move_only &) = delete;

        move_only(move_only &&) noexcept = default;

        move_only &operator=(const move_only &) = delete;

        move_only &operator=(move_only &&) noexcept = default;

        ~move_only() = default;
    };
}