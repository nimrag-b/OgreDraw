#include "OgreDef.h"

Canvas::~Canvas()
{
    for (auto& it : mLayers)
    {
        delete it;
    }
}

Layer* Canvas::InsertLayer(SDL_Renderer* renderer, size_t index)
{
    Layer* layer = new Layer;
    layer->surface = SDL_CreateSurface(mWidth, mHeight, SDL_PIXELFORMAT_RGBA32);
    layer->gpuTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, mWidth, mHeight);
    SDL_SetTextureScaleMode(layer->gpuTexture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(layer->gpuTexture, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
    layer->area = { 0,0,(int)mWidth,(int)mHeight };

    if (index >= mLayers.size()) {
        mLayers.push_back(layer);
        mSelectedIndex = mLayers.size() - 1;
    }
    else {
        mLayers.insert(mLayers.begin() + index, layer);

        mSelectedIndex = index;
    }



    return layer;
}
