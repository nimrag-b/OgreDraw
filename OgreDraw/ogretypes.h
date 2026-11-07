#pragma once
#include <cstdint>

namespace ogre{

    template <typename T>
    T& min(const T& a, const T& b){
        return a < b ? a : b;
    }
    template <typename T>
    T& max(const T& a, const T& b){
        return a > b ? a : b;
    }

    template <typename T>
    T& clamp(const T& v, const T& min, const T& max){
        return (v > max) ? (max) : ((v < min) ? (min) : (v));
    }

    
    struct Rect {
        float x, y, w, h;
    };
    struct IRect {
        IRect() {}
        IRect(int X, int Y, int W, int H) : x(X), y(Y), w(W), h(H) {}

        int x = 0, y = 0, w = 0, h = 0;
    };



    struct colour {
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        uint8_t a = 0;


        colour operator +=(const colour& o) {
            return colour{
                uint8_t((r + o.r) & 0xFF),
                uint8_t((g + o.g) & 0xFF),
                uint8_t((b + o.b) & 0xFF) ,
                uint8_t((a + o.a) & 0xFF) };
        }

        inline colour blend(const colour& o) {
            colour v = *this;

            unsigned int alpha = o.a + 1;
            unsigned int inv_alpha = 256 - o.a;

            v.r = (unsigned char)((alpha * o.r + inv_alpha * r) >> 8);
            v.g = (unsigned char)((alpha * o.g + inv_alpha * g) >> 8);
            v.b = (unsigned char)((alpha * o.b + inv_alpha * b) >> 8);
            v.a = types::min(int(o.a + a), 255);

            return v;

        }
    };
}
