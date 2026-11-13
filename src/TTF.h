#pragma once

#include <proto-include.h>
#include <SDL2/SDL.h>

#include "Color.h"

extern FT_Library ftLib;

bool InitFont(FT_Face* face, const char* fontPath, int fontSize);

// Render a line of text as an alpha-blended texture. The caller owns the texture.
SDL_Texture* CreateText(SDL_Renderer* renderer, const char* txt, FT_Face fontFace, Color fgColor);

// Width in pixels a line of text will take with this font
int MeasureText(const char* txt, FT_Face fontFace);
