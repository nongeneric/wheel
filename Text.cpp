#include "Text.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <boost/locale.hpp>
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

std::u32string tou32str(std::string utf8) {
    return boost::locale::conv::utf_to_utf<char32_t>(utf8);
}

// FreeImage/Source/FreeImageToolkit/CopyPaste.cpp: Combine8
BOOL BlendPaste8(FIBITMAP *dst_dib, FIBITMAP *src_dib, unsigned x, unsigned y) {
    // check the bit depth of src and dst images
    if((FreeImage_GetBPP(dst_dib) != 8) || (FreeImage_GetBPP(src_dib) != 8)) {
        return FALSE;
    }

    // check the size of src image
    if((x + FreeImage_GetWidth(src_dib) > FreeImage_GetWidth(dst_dib)) || (y + FreeImage_GetHeight(src_dib) > FreeImage_GetHeight(dst_dib))) {
        return FALSE;
    }

    BYTE *dst_bits = FreeImage_GetBits(dst_dib) + ((FreeImage_GetHeight(dst_dib) - FreeImage_GetHeight(src_dib) - y) * FreeImage_GetPitch(dst_dib)) + (x);
    BYTE *src_bits = FreeImage_GetBits(src_dib);

    // alpha blend images
    for(unsigned rows = 0; rows < FreeImage_GetHeight(src_dib); rows++) {
        for (unsigned cols = 0; cols < FreeImage_GetLine(src_dib); cols++) {
            dst_bits[cols] += src_bits[cols];
        }

        dst_bits += FreeImage_GetPitch(dst_dib);
        src_bits += FreeImage_GetPitch(src_dib);
    }

    return TRUE;
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

    BitmapPtr renderText(std::string utf8str, unsigned pxHeight) {
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

        RGBQUAD color = { 0,0,0,0 };
        FreeImage_FillBackground(_fbitmap.get(), &color);

        int pen_x = 5;
        int prev = 0;
        for (size_t i = 0; i < str.size(); ++i) {
            FT_UInt glyph_index = FT_Get_Char_Index(face, str[i]);

            if (prev) {
                FT_Vector delta;
                FT_Get_Kerning(face, prev, glyph_index, FT_KERNING_DEFAULT, &delta);
                pen_x += delta.x / 64;
            }

            CacheEntry entry = getCachedGlyph(pxHeight, glyph_index, face);
            BlendPaste8(
                _fbitmap.get(),
                entry.bitmap.get(),
                pen_x + entry.bitmapLeft,
                pxHeight - entry.bitmapTop
            );

            pen_x += entry.advanceX / 64;
            prev = glyph_index;
        }
        BitmapPtr fcropped = make_bitmap_ptr( FreeImage_Copy(_fbitmap.get(), 0, pxHeight * 1.3f, pen_x, 0) );
//        bool res = FreeImage_Save(FIF_PNG, fcropped.get(), "/home/tr/Desktop/test.png");
//        (void)res;
        return fcropped;
    }
};

Text::Text() : _impl(new impl())
{ }

BitmapPtr Text::renderText(std::string str, unsigned pxHeight) {
    return _impl->renderText(str, pxHeight);
}

Text::~Text() { }

