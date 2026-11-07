#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include "Layer.h"








class Canvas {
public:

    Canvas(size_t w, size_t h) : mWidth(w), mHeight(h){}


    ~Canvas();

    Layer* InsertLayer(SDL_Renderer* renderer, size_t index = SIZE_MAX);

    Layer* Selected() const {
        if (mSelectedIndex > mLayers.size()) return nullptr;
        return mLayers.at(mSelectedIndex);
    }

    void Select(size_t index) {
        mSelectedIndex = index;
    }

    size_t SelectedIndex() const noexcept{
        return mSelectedIndex;
    }

    size_t LayerCount() const {
        return mLayers.size();
    }

    const std::vector<Layer*> Layers() const {
        return mLayers;
    }
private:

    std::vector<Layer*> mLayers;
    size_t mSelectedIndex = 0;
    size_t mWidth = 0;
    size_t mHeight = 0;
};

