#include "Text.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <assert.h>
#include <map>

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
    BitmapPtr bitmap;
};

std::shared_ptr<FIBITMAP> make_bitmap_ptr(FIBITMAP* raw) {
    return std::shared_ptr<FIBITMAP>(raw, FreeImage_Unload);
}

class Text::impl {
    FT_Face face;
    std::map<const CacheKey, CacheEntry> _cache;
    BitmapPtr _fbitmap;
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

        BitmapPtr glyph_bitmap;
        if (slot->bitmap.width > 0 && slot->bitmap.rows > 0) {
            glyph_bitmap = make_bitmap_ptr( FreeImage_ConvertFromRawBits(
                    static_cast<BYTE*>(slot->bitmap.buffer),
                    slot->bitmap.width,
                    slot->bitmap.rows,
                    slot->bitmap.pitch,
                    8, 0, 0, 0, true));
            assert(glyph_bitmap.get());
        } else {
            glyph_bitmap = make_bitmap_ptr(FreeImage_Allocate(0, 0, 8));
        }
        CacheEntry entry { slot->bitmap_top, slot->bitmap_left, slot->advance.x, glyph_bitmap };
        _cache[key] = entry;
        return entry;
    }
public:
    impl() {
        _fbitmap = make_bitmap_ptr( FreeImage_Allocate(1000, 300, 8) );
        assert(_fbitmap.get());
    }

    BitmapPtr renderText(std::string str, unsigned pxHeight) {
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

        unsigned color = 0;
        FreeImage_FillBackground(_fbitmap.get(), &color);

        int pen_x = 0;
        int prev = 0;
        for (size_t i = 0; i < str.size(); ++i) {
            FT_UInt glyph_index = FT_Get_Char_Index(face, str[i]);

            if (prev) {
                FT_Vector delta;
                FT_Get_Kerning(face, prev, glyph_index, FT_KERNING_DEFAULT, &delta);
                pen_x += delta.x / 64;
            }

            CacheEntry entry = getCachedGlyph(pxHeight, glyph_index, face);
            FreeImage_Paste(
                _fbitmap.get(),
                entry.bitmap.get(),
                pen_x + entry.bitmapLeft,
                pxHeight - entry.bitmapTop,
                255
            );

            pen_x += entry.advanceX / 64;
            prev = glyph_index;
        }
        BitmapPtr fcropped = make_bitmap_ptr( FreeImage_Copy(_fbitmap.get(), 0, pxHeight * 1.1f, pen_x, 0) );
        BitmapPtr fbitmap32 = make_bitmap_ptr( FreeImage_ConvertTo32Bits(fcropped.get()) );
        for (size_t y = 0; y < FreeImage_GetHeight(fbitmap32.get()); ++y) {
            for (size_t x = 0; x < FreeImage_GetWidth(fbitmap32.get()); ++x) {
                RGBQUAD color;
                FreeImage_GetPixelColor(fbitmap32.get(), x, y, &color);
                color.rgbReserved = color.rgbBlue;
                color.rgbBlue = 255;
                color.rgbGreen = 255;
                color.rgbRed = 255;
                FreeImage_SetPixelColor(fbitmap32.get(), x, y, &color);
            }
        }
//        bool res = FreeImage_Save(FIF_PNG, fbitmap32.get(), "C:/Users/tr/Desktop/test.png");
//        (void)res;
        return fbitmap32;
    }
};

Text::Text() : _impl(new impl())
{ }

BitmapPtr Text::renderText(std::string str, unsigned pxHeight) {
    return _impl->renderText(str, pxHeight);
}

Text::~Text() { }

