#include "TTF.h"

#include <string.h>

FT_Library ftLib;

bool InitFont(FT_Face* face, const char* fontPath, int fontSize)
{
    if (FT_New_Face(ftLib, fontPath, 0, face) != 0)
        return false;
    if (FT_Set_Pixel_Sizes(*face, 0, fontSize) != 0)
        return false;
    return true;
}

int MeasureText(const char* txt, FT_Face fontFace)
{
    int width = 0;
    for (const char* c = txt; *c; ++c)
    {
        if (FT_Load_Char(fontFace, (unsigned char)*c, FT_LOAD_DEFAULT) != 0)
            continue;
        width += fontFace->glyph->advance.x >> 6;
    }
    return width;
}

SDL_Texture* CreateText(SDL_Renderer* renderer, const char* txt, FT_Face fontFace, Color fgColor)
{
    // Line metrics from the font size, in whole pixels
    const int ascent = fontFace->size->metrics.ascender >> 6;
    const int descent = -(fontFace->size->metrics.descender >> 6);
    const int height = ascent + descent;
    int width = MeasureText(txt, fontFace);
    if (width < 1)
        width = 1;
    if (height < 1)
        return NULL;

    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!surface)
        return NULL;
    SDL_FillRect(surface, NULL, 0x00000000);

    const uint32_t rgb = ((uint32_t)fgColor.r << 16) | ((uint32_t)fgColor.g << 8) | (uint32_t)fgColor.b;
    int penX = 0;

    for (const char* c = txt; *c; ++c)
    {
        if (FT_Load_Char(fontFace, (unsigned char)*c, FT_LOAD_RENDER) != 0)
            continue;

        FT_GlyphSlot glyph = fontFace->glyph;
        const FT_Bitmap& bmp = glyph->bitmap;

        for (unsigned int row = 0; row < bmp.rows; ++row)
        {
            const int py = ascent - glyph->bitmap_top + (int)row;
            if (py < 0 || py >= height)
                continue;
            uint32_t* line = (uint32_t*)((uint8_t*)surface->pixels + py * surface->pitch);

            for (unsigned int col = 0; col < bmp.width; ++col)
            {
                const int px = penX + glyph->bitmap_left + (int)col;
                if (px < 0 || px >= width)
                    continue;

                const uint8_t coverage = bmp.buffer[row * bmp.pitch + col];
                if (coverage == 0)
                    continue;

                // Keep the strongest coverage where glyphs overlap
                const uint32_t existing = line[px] >> 24;
                if (coverage > existing)
                    line[px] = ((uint32_t)coverage << 24) | rgb;
            }
        }

        penX += glyph->advance.x >> 6;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture)
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(surface);
    return texture;
}
