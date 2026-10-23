// OgreDraw.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <SDL3/SDL.h>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include "OgreDef.h"
#include "Linedraw.h"

#define COL_KEY(x, y) (((uint16_t)(x)) | ((uint16_t)(y) << 16))


static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

static colour drawColour = {0,0,0,0x2F};


static float xo = 50;
static float yo = 50;
static float s = 0.5f;

typedef std::vector<std::pair<int, int>> AA_Nodes;

AA_Nodes AA_none = {
    {0,0}
};
AA_Nodes AA_MSAA = {
    {0,0},
    {1,0},
    {-1,0},
    {0,1},
    {0,-1},
};

AA_Nodes AA_MSAA8 = {
    {0,0},
    {1,0},
    {1,1},
    {0,1},
    {-1,1},
    {-1,0},
    {-1,-1},
    {0, -1},
    {1, -1}
};

AA_Nodes AA_Large = {
    {-3,-3},{-2,-3},{-1,-3},{0,-3},{1,-3},{2,-3},{3,-3},
    {-3,-2},{-2,-2},{-1,-2},{0,-2},{1,-2},{2,-2},{3,-2},
    {-3,-1},{-2,-1},{-1,-1},{0,-1},{1,-1},{2,-1},{3,-1},
    {-3, 0},{-2, 0},{-1, 0},{0, 0},{1, 0},{2, 0},{3, 0},
    {-3, 1},{-2, 1},{-1, 1},{0, 1},{1, 1},{2, 1},{3, 1},
    {-3, 2},{-2, 2},{-1, 2},{0, 2},{1, 2},{2, 2},{3, 2},
    {-3, 3},{-2, 3},{-1, 3},{0, 3},{1, 3},{2, 3},{3, 3},
};

AA_Nodes& AAMode = AA_MSAA8;

float clickx = -1;
float clicky = -1;

bool TextBox(int x, int y, int w, int h, char* str, int len) {
    
    if (clickx >= x && clickx < (x + w) && clicky >= y && clicky < (y + h)) {
        SDL_Rect r = { x,y,w,h }; 

    }
    return false;
}


void Layer::Undo(Stroke& stroke) {

    if (SDL_MUSTLOCK(surface)) {
        SDL_LockSurface(surface);
    }

    for (auto& it : stroke.changes) {
        colour* px = (colour*)(surface->pixels);

        px[it.first] = it.second;
    }

    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }

    synced = false;
}

void Layer::Commit() {
    uint8_t* pixels;
    int pitch;
    SDL_LockTexture(gpuTexture, NULL, (void**)&pixels, &pitch);

    if (SDL_MUSTLOCK(surface)) {
        SDL_LockSurface(surface);
    }

    if (surface->pitch == pitch) {
        for (size_t i = 0; i < surface->h; i++)
        {
            memcpy(pixels + (pitch * i), ((char*)surface->pixels) + (pitch * i), pitch);
        }
    }
    else {
        SDL_assert(0); //dont handle for now
    }

    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }

    SDL_UnlockTexture(gpuTexture);

    synced = true;
}

void Layer::DrawPoint(Brush& brush, int x, int y) {
    if (x < area.x || y < area.y || x >= area.x + area.w || y >= area.y + area.h) {
        return;
    }

    if (SDL_MUSTLOCK(surface)) {
        SDL_LockSurface(surface);
    }

    //rgba colour
    colour* px = (colour*)(surface->pixels);


    int halfwidth = brush.width / 2;
    int iwidth = (int)ceilf(brush.width);
    if (insidegrid == NULL) {
        insidegrid = new int[iwidth * iwidth] {0};
        lastWidth = iwidth;
    }
    else if (lastWidth != iwidth) {
        delete[] insidegrid;
        insidegrid = new int[iwidth * iwidth] {0};
        lastWidth = iwidth;
    }
    else {
        memset(insidegrid, 0, iwidth * iwidth * sizeof(int));
    }


    for (int j = -halfwidth + 1; j < halfwidth; j++)
    {
        for (int i = -halfwidth + 1; i < halfwidth; i++)
        {
            if ((x + i) < area.x || (y + j) < area.y || (x + i) >= area.x + area.w || (y + j) >= area.y + area.h) {
                continue;
            }

            int factor = 0;

            for (auto& it : AAMode)
            {
                int a = (i + it.first + halfwidth) + int((j + it.second + halfwidth) * iwidth);
                if (a < 0 || a >= (iwidth * iwidth)) {
                    continue;
                }
                if (insidegrid[a] == 0) {
                    insidegrid[a] = brush.shape->PointInside(((float)(i + it.first) / (float)brush.width), ((float)(j + it.second) / (float)brush.width)) ? 1: -1;
                }
                if (insidegrid[a] == 1) {
                    factor++;
                }
            }

            if (factor != 0) {

                float f = (float)factor / (float)(AAMode.size());

                f = f * f * f;

                int dx = (x + i);
                int dy = (y + j);

                //int v = COL_KEY(dx,dy);

                int idx = dx + (dy * (surface->pitch / sizeof(colour)));


                uint8_t opacity = (uint8_t)(brush.colour.a * f);

                if (changes.changes.count(idx) != 0) {
                    colour cn = changes.changes.at(idx).blend({ brush.colour.r,brush.colour.g,brush.colour.b,opacity });
                    if (cn.a > px[idx].a) {
                        px[idx] = cn;
                    }
                }
                else {

                    changes.changes.insert({ idx, px[idx] });
                    px[idx] = px[idx].blend({ brush.colour.r,brush.colour.g,brush.colour.b,opacity });
                }

            }

        }
    }

    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }
    synced = false;
}


Layer* CreateLayer(int w, int h) {
    Layer* layer = new Layer;
    layer->surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
    layer->gpuTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
    SDL_SetTextureScaleMode(layer->gpuTexture, SDL_SCALEMODE_NEAREST);
    layer->area = { 0,0,w,h };

    return layer;
}


bool ctrl = false;
bool space = false;
bool quit = false;
bool mdown = false;

int xlast;
int ylast;

Layer* layer;
Brush* brush;

#define CHANGE_MAX 50
Stroke changeStack[CHANGE_MAX];
int changeCount = 0;
int changeTop = 0;


int sdl_proc_event(SDL_Event* e) {


    switch (e->type)
    {
    case SDL_EVENT_QUIT:
        quit = true;
        break;
    case SDL_EVENT_KEY_DOWN: {
        switch (e->key.key)
        {
        case SDLK_LCTRL:
            ctrl = true;
            break;

        case SDLK_SPACE:
            space = true;
            break;
        }

        case SDLK_Z: {
            if (ctrl && changeCount > 0) {

                if (changeTop == 0) {
                    changeTop = CHANGE_MAX - 1;
                }
                else {
                    --changeTop;
                }


                --changeCount;

                layer->Undo(changeStack[changeTop]);

                changeStack[changeTop].changes.clear();

            }
        }

    } break;


    case SDL_EVENT_KEY_UP: {
        switch (e->key.key)
        {
        case SDLK_LCTRL:
            ctrl = false;
            break;

        case SDLK_SPACE:
            space = false;
            break;
        }

    } break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        if (e->button.button == SDL_BUTTON_LEFT) {

            clickx = e->button.x;
            clicky = e->button.y;

            xlast = (e->button.x / s) - (xo);
            ylast = (e->button.y / s) - (yo);

            mdown = true;
            if (!space) {
                layer->DrawPoint(*brush, xlast, ylast);
            }

        }
    } break;
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        if (e->button.button == SDL_BUTTON_LEFT) {
            mdown = false;
            changeStack[changeTop] = layer->changes;
            layer->changes.changes.clear();

            changeTop = (changeTop + 1) % CHANGE_MAX;

            if (changeCount < CHANGE_MAX) {
                changeCount++;
            }

        }
    } break;
    case SDL_EVENT_MOUSE_MOTION: {
        if (mdown) {
            if (space) {
                xo += e->motion.xrel / s;
                yo += e->motion.yrel / s;
            }
            else {
                int x = (e->motion.x / s) - (xo);
                int y = (e->motion.y / s) - (yo);
                Drawing::DrawLine(*brush, layer, xlast, ylast, x, y);
                xlast = x;
                ylast = y;
            }

        }


    } break;

    case SDL_EVENT_MOUSE_WHEEL: {
        s += e->wheel.y * 0.02f;
        s = s < 0.1f ? 0.1f : s > 40 ? 40 : s;
    } break;


    }

    return 1;
}

int main()
{
    std::cout << "Hello World!\n";

    SDL_CreateWindowAndRenderer("OgreDraw", 860, 480, SDL_WINDOW_RESIZABLE, &window, &renderer);

    Canvas canvas;

    layer = CreateLayer(2000, 2000);
    canvas.layers.push_back(layer);
    layer->Clear(0xFF, 0xFF, 0xFF, 0xFF);

    layer = CreateLayer(2000, 2000);
    canvas.layers.push_back(layer);


    brush = new Brush;
    brush->colour = drawColour;
    brush->shape = new  VectorShape{ VectorCircle(0,0,0.5f) };
    brush->width = 20;




    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            sdl_proc_event(&e);

        }

        SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0xFF, 0xFF);

        SDL_RenderClear(renderer);
        for (size_t i = 0; i < canvas.layers.size(); i++)
        {
            canvas.layers[i]->Render(renderer, xo, yo, s);
        }


        // Update screen
        SDL_RenderPresent(renderer);
    }

    delete layer;
}
