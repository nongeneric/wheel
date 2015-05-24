#include "Text.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <boost/locale.hpp>
#include <assert.h>
#include <map>
#include <stdio.h>

bool freeTypeInit = false;
FT_Library library;

struct CacheKey {
    unsigned pxHeight;
    FT_UInt glyphIndex;
    FT_Face face;
    bool operator<(const CacheKey& other) const {
        return pxHeight < other.pxHeight ||
               glyphIndex < other.glyphIndex ||
               face < other.face;
    }
};

struct CacheEntry {
    FT_Int bitmapTop;
    FT_Int bitmapLeft;
    FT_Pos advanceX;
    Bitmap bitmap;
};

std::u32string tou32str(std::string utf8) {
    return boost::locale::conv::utf_to_utf<char32_t>(utf8);
}

void dump(Bitmap bitmap) {
    FILE* f = fopen("/d/out.txt", "w");
    assert(f);
    auto pos = (char*)bitmap.data();
    for (auto row = 0u; row < bitmap.height(); ++row) {
        for (auto x = 0u; x < bitmap.width(); ++x) {
            fprintf(f, "%02x", (unsigned char)pos[x]);
        }
        pos += bitmap.pitch();
        fprintf(f, "\n");
    }
    fclose(f);
}

class Text::impl {
    FT_Face face;
    std::map<const CacheKey, CacheEntry> _cache;
    Bitmap _fbitmap;
    CacheEntry getCachedGlyph(unsigned pxHeight, FT_UInt glyphIndex, FT_Face face) {
        CacheKey key { pxHeight, glyphIndex, face };
        auto it = _cache.find(key);
        if (it != _cache.end())
            return it->second;
        FT_GlyphSlot slot = face->glyph;
        auto error = FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT);
        assert(!error);

        error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        assert(!error);

        Bitmap glyph_bitmap = slot->bitmap.width > 0 && slot->bitmap.rows > 0
                ? Bitmap(
                      slot->bitmap.buffer,
                      slot->bitmap.width,
                      slot->bitmap.rows,
                      slot->bitmap.pitch,
                      8, true)
                : Bitmap(0, 0, 8);
        dump(glyph_bitmap);
        CacheEntry entry { slot->bitmap_top, slot->bitmap_left, slot->advance.x, glyph_bitmap };
        _cache[key] = entry;
        return entry;
    }
public:
    impl() : _fbitmap(1500, 300, 8) {
    }

    Bitmap renderText(std::string utf8str, unsigned pxHeight) {
        auto str = tou32str(utf8str);
        if (!freeTypeInit) {
            auto error = FT_Init_FreeType(&library);
            assert(!error);
            freeTypeInit = true;
            error = FT_New_Face(library,
                        "LiberationSans-Regular.ttf",
                        0,
                        &face);
            assert(!error);
        }

        auto error = FT_Set_Char_Size(
                face,
                0,
                pxHeight*64,
                96,
                96
            );
        assert(!error);

        _fbitmap.fill(0);

        int pen_x = 5;
        int pen_y = pxHeight / 3;
        int prev = 0;
        for (size_t i = 0; i < str.size(); ++i) {
            FT_UInt glyph_index = FT_Get_Char_Index(face, str[i]);

            if (prev) {
                FT_Vector delta;
                FT_Get_Kerning(face, prev, glyph_index, FT_KERNING_DEFAULT, &delta);
                pen_x += delta.x / 64;
            }

            CacheEntry entry = getCachedGlyph(pxHeight, glyph_index, face);
            _fbitmap.blendPaste(entry.bitmap, pen_x + entry.bitmapLeft, pen_y + entry.bitmapTop - entry.bitmap.height());

            pen_x += entry.advanceX / 64;
            prev = glyph_index;
        }
        //dump(_fbitmap);
        auto fcropped = _fbitmap.copy(0, pxHeight * 1.3f, pen_x, 0);
//        bool res = FreeImage_Save(FIF_PNG, fcropped.get(), "/tmp/test.png");
//        (void)res;
        return fcropped;
        //return _fbitmap;
    }
};

Text::Text() : _impl(new impl())
{ }

Bitmap Text::renderText(std::string str, unsigned pxHeight) {
    return _impl->renderText(str, pxHeight);
}

Text::~Text() { }

