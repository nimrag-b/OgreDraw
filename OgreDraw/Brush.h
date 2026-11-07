#pragma once

#include "VectorShape.h"
#include "ogretypes.h"


struct Brush {
    float width;
    ogre::colour colour;
    VectorShape* shape;
};
