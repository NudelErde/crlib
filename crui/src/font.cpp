//
// Created by nudelerde on 24.05.23.
//

#include "font.h"
#include <ft2build.h>
#include FT_FREETYPE_H

static FT_Library& get_ftlib() {
    static FT_Library ftlib;
    static bool init = false;
    if (!init) {
        FT_Init_FreeType(&ftlib);
        init = true;
    }
    return ftlib;
}

namespace cr::ui {
std::shared_ptr<font> loadFont(const std::string& path, unsigned int size) {
    FT_Face face;
    FT_New_Face(get_ftlib(), path.c_str(), 0, &face);
    FT_Set_Char_Size(face, 0, size * 64, 300, 300);

    unsigned int maxCharWidth = 0;
    unsigned int maxCharHeight = 0;

    std::unordered_map<char, font::glyph> glyphs;
    glyphs.reserve(128);
    for (size_t i = 0; i < 128; ++i) {
        // load only width and height of bitmap
        FT_Load_Char(face, i, FT_LOAD_RENDER);
        maxCharWidth = std::max(maxCharWidth, face->glyph->bitmap.width);
        maxCharHeight = std::max(maxCharHeight, face->glyph->bitmap.rows);
    }

    unsigned int atlasWidth = maxCharWidth * 16;
    unsigned int atlasHeight = maxCharHeight * 8;

    std::unique_ptr<unsigned char[]> atlasData;
    atlasData.reset(new unsigned char[atlasWidth * atlasHeight]);

    for (size_t i = 0; i < 128; ++i) {
        FT_Load_Char(face, i, FT_LOAD_RENDER);
        auto& glyph = glyphs[char(i)];
        glyph.atlasX = (int(i) % 16) * maxCharWidth;
        glyph.atlasY = (int(i) / 16) * maxCharHeight;
        glyph.atlasWidth = face->glyph->bitmap.width;
        glyph.atlasHeight = face->glyph->bitmap.rows;
        glyph.advance = face->glyph->advance.x / 64;
        glyph.width = face->glyph->bitmap.width;
        glyph.height = face->glyph->bitmap.rows;
        glyph.bearingX = face->glyph->bitmap_left;
        glyph.bearingY = face->glyph->bitmap_top;
        for (size_t y = 0; y < face->glyph->bitmap.rows; ++y) {
            for (size_t x = 0; x < face->glyph->bitmap.width; ++x) {
                atlasData[(glyph.atlasY + y) * atlasWidth + glyph.atlasX + x] =
                        face->glyph->bitmap.buffer[y * face->glyph->bitmap.width + x];
            }
        }
    }

    auto atlas = std::make_shared<texture>();
    atlas->set_data(atlasData.get(), atlasWidth, atlasHeight, 1);

    FT_Done_Face(face);

    return std::make_shared<font>(std::move(glyphs), atlas, atlasWidth, atlasHeight);
}
}// namespace cr::ui