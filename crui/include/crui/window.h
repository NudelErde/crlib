//
// Created by nudelerde on 19.05.23.
//

#pragma once
#include <memory>
#include "crmath/matrix.h"

struct GLFWwindow;

namespace cr::ui {
    struct Window {
        GLFWwindow* window = nullptr;

        void update();
        [[nodiscard]] cr::math::cvector<int, 2> size() const;
        void setTitle(const char* title);

        [[nodiscard]] bool exists() const;
    private:
        bool isOpen = true;
        friend std::unique_ptr<Window> createWindow();
    };

    std::unique_ptr<Window> createWindow();
}
