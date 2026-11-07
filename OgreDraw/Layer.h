#pragma once
#include <SDL3/SDL.h>
#include <unordered_map>

#include "ogretypes.h"
#include "Brush.h"


struct Layer;

//supports canvasses up to 65000 px
struct Stroke {

    Stroke() : layer(nullptr){}

    Stroke(Layer* layer) : layer(layer){}

    struct colourPair {
        ogre::colour prev;
        ogre::colour add;
    };
    Layer* layer;
    std::unordered_map<uint32_t, colourPair> changes;
};

struct Layer {

    Layer() : changes(this){}

    SDL_Surface* surface = NULL;
    SDL_Texture* gpuTexture = NULL;
    ogre::IRect area;

    Stroke changes;

    int* insidegrid = NULL;
    int lastWidth = -1;

    bool synced = false;

    void Undo(Stroke& stroke);


    void Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        SDL_ClearSurface(surface, r, g, b, a);
        synced = false;
    }

    void Commit();

    void Render(SDL_Window* window, SDL_Renderer* renderer, float xo, float yo, float s) {
        if (!synced) {
            Commit();
        }
        int ww, wh;
        SDL_GetWindowSize(window, &ww, &wh);

        SDL_FRect r{ area.x + (ww/2) + (xo * s),area.y + (wh/2) + (yo * s),area.w * s, area.h * s};
        SDL_RenderTexture(renderer, gpuTexture, NULL, &r);
    }

    void DrawPoint(Brush& brush, int x, int y);
};