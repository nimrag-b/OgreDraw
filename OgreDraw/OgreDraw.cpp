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

static colour drawColour = {0xFF,0,0,0xFF};

static int windowWidth = 460;
static int windowHeight = 680;

static float xo = -1000;
static float yo = -1000;
static float s = 0.5f;

static int mPosX;
static int mPosY;

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

        px[it.first] = it.second.prev;
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
                colour cn = { brush.colour.r,brush.colour.g,brush.colour.b,opacity };
                if (changes.changes.count(idx) != 0) {

                    if (cn.a > changes.changes.at(idx).add.a) {
                        changes.changes.at(idx).add = cn;
                        px[idx] = changes.changes.at(idx).prev.blend(cn);
                    }
                }
                else {
                    changes.changes.insert({ (uint32_t)idx, {px[idx], cn} });
                    px[idx] = px[idx].blend(cn);
                }

            }

        }
    }

    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }
    synced = false;
}



bool ctrl = false;
bool space = false;
bool quit = false;
bool mdown = false;

int xlast;
int ylast;
Canvas canvas{ 2000,2000 };
Brush* brush;

#define CHANGE_MAX 50
Stroke changeStack[CHANGE_MAX];
int changeCount = 0;
int changeTop = 0;


int transform(int p, int w, int o) {
    return (p - ((w / 2) + (o * s))) / s;
}


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

        case SDLK_Z: {
            if (ctrl && changeCount > 0) {

                if (changeTop == 0) {
                    changeTop = CHANGE_MAX - 1;
                }
                else {
                    --changeTop;
                }


                --changeCount;

                changeStack[changeTop].layer->Undo(changeStack[changeTop]);

                changeStack[changeTop].changes.clear();
                changeStack[changeTop].layer = nullptr;

            }
        } break;
        case SDLK_UP:
            canvas.Select(SDL_clamp(canvas.SelectedIndex() + 1, 0, canvas.LayerCount() - 1));
            break;
        case SDLK_DOWN:
            canvas.Select(SDL_clamp((int)canvas.SelectedIndex() - 1, 0, canvas.LayerCount() - 1));
            break;
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

            xlast = transform(e->button.x, windowWidth, xo);
            ylast = transform(e->button.y, windowHeight, yo);

            mdown = true;
            if (!space) {
                canvas.Selected()->DrawPoint(*brush, xlast, ylast);
            }

        }
    } break;
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        if (e->button.button == SDL_BUTTON_LEFT) {
            mdown = false;
            changeStack[changeTop].changes = std::move(canvas.Selected()->changes.changes);
            changeStack[changeTop].layer = canvas.Selected()->changes.layer;
            //should be empty because of move semantics, but make sure anyway
            canvas.Selected()->changes.changes.clear();

            changeTop = (changeTop + 1) % CHANGE_MAX;

            if (changeCount < CHANGE_MAX) {
                changeCount++;
            }

        }
    } break;
    case SDL_EVENT_MOUSE_MOTION: {
        mPosX = e->motion.x;
        mPosY = e->motion.y;
        if (mdown) {
            if (space) {
                xo += e->motion.xrel / s;
                yo += e->motion.yrel / s;
            }
            else {
                int x = transform(e->motion.x, windowWidth, xo);
                int y = transform(e->motion.y, windowHeight, yo);
                Drawing::DrawLine(*brush, canvas.Selected(), xlast, ylast, x, y);
                xlast = x;
                ylast = y;
            }

        }


    } break;

    case SDL_EVENT_MOUSE_WHEEL: {
        s += e->wheel.y * 0.02f;
        s = s < 0.1f ? 0.1f : s > 40 ? 40 : s;

    } break;

    case SDL_EVENT_WINDOW_RESIZED: {
        windowWidth = e->window.data1;
        windowHeight = e->window.data2;
    } break;

    }

    return 1;
}

int main()
{
    std::cout << "Hello World!\n";

    SDL_CreateWindowAndRenderer("OgreDraw", windowWidth, windowHeight, SDL_WINDOW_RESIZABLE, &window, &renderer);



    Layer* baselayer = canvas.InsertLayer(renderer);
    baselayer->Clear(0xFF, 0xFF, 0xFF, 0xFF);

    Layer* layer2 = canvas.InsertLayer(renderer);
    //layer2->Clear(0xFF, 0x00, 0xFF, 0xFF);

    brush = new Brush;
    brush->colour = drawColour;
    brush->shape = new  VectorShape{ VectorCircle(0,0,0.5f) };
    brush->width = 20;



    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            sdl_proc_event(&e);

        }

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);

        SDL_RenderClear(renderer);
        for (int i = 0; i < canvas.LayerCount(); ++i)
        {
            canvas.Layers()[i]->Render(window, renderer, xo, yo, s);
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0x90);

        SDL_FRect r{ 0,0,300,200 };

        SDL_RenderFillRect(renderer, &r);

        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

        char textbuf[512];

        snprintf(textbuf, 512, "%d %d (%.2f %.2f) %.2f", mPosX, mPosY, xo, yo, s);
        SDL_RenderDebugText(renderer, 10, 10, textbuf);

        snprintf(textbuf, 512, "layer - %zu/%zu", canvas.SelectedIndex() + 1, canvas.LayerCount());
        SDL_RenderDebugText(renderer, 10,30, textbuf);

        snprintf(textbuf, 512, "brush - '%s' %.2fpx", "round", brush->width);
        SDL_RenderDebugText(renderer, 10, 40, textbuf);


        // Update screen
        SDL_RenderPresent(renderer);
    }
}
