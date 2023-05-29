//
// Created by nudelerde on 24.05.23.
//

#pragma once

#include "opengl.h"
#include <string>
#include <unordered_map>

namespace cr::ui {
struct font {
    struct glyph {
        unsigned int atlasX;
        unsigned int atlasY;
        unsigned int atlasWidth;
        unsigned int atlasHeight;

        unsigned int advance;
        unsigned int width;
        unsigned int height;
        int bearingX;
        int bearingY;
    };

    std::unordered_map<char, glyph> glyphs;
    std::shared_ptr<texture> atlas;
    unsigned int atlasWidth;
    unsigned int atlasHeight;

    font() = default;
    font(std::unordered_map<char, glyph> glyphs,
         std::shared_ptr<texture> atlas,
         unsigned int atlasWidth,
         unsigned int atlasHeight) ://
                                     glyphs(std::move(glyphs)),
                                     atlas(std::move(atlas)),
                                     atlasWidth(atlasWidth),
                                     atlasHeight(atlasHeight) {}
};

std::shared_ptr<font> loadFont(const std::string& path, unsigned int size);
}// namespace cr::ui
