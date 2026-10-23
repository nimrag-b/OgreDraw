#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <unordered_map>

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

        if (o.a == 255) {
            v.r = o.r;
            v.g = o.g;
            v.b = o.b;
            v.a = o.a;
        }
        else {
            float f = (float)o.a / 255.0f;
            v.r = std::min(int(r + o.r * f), 255);
            v.g = std::min(int(g + o.g * f), 255);
            v.b = std::min(int(b + o.b * f), 255);
            v.a = std::min(int(a + o.a), 255);
        }

        return v;

    }
};


struct VectorPolygon {
    struct Point {
        float x;
        float y;
    };
    Point* points; //last point connects to first
    int length;
};

struct VectorCircle {
    VectorCircle(float X, float Y, float R) {
        x = X;
        y = Y;
        r = R;
    }
    float x;
    float y;
    float r;
};

enum class VectorShapeType {
    Polygon,
    Circle
};
struct VectorShape {

    VectorShape(VectorCircle circle) {
        type = VectorShapeType::Circle;
        this->circle = circle;
    }
    VectorShape(VectorPolygon polygon) {
        type = VectorShapeType::Polygon;
        this->polygon = polygon;
    }

    VectorShapeType type;
    union {
        VectorPolygon polygon;
        VectorCircle circle;
    };
    bool PointInside_Circle(float x, float y) {
        float dx = abs(x - circle.x);
        if (dx > circle.r) return false;
        float dy = abs(y - circle.y);
        if (dy > circle.r) return false;
        if (dx + dy <= circle.r) return true;
        return (dx * dx + dy * dy <= circle.r * circle.r);
    }

    bool PointInside_Polygon(float x, float y) {
        int count = 0;

        // Iterate through each edge of the polygon
        for (int i = 0; i < polygon.length; i++) {
            VectorPolygon::Point p1 = polygon.points[i];
            // Ensure the last point connects to the first point
            VectorPolygon::Point p2 = polygon.points[(i + 1) % polygon.length];

            // Check if the point's y-coordinate is within the
            // edge's y-range and if the point is to the left of
            // the edge
            if ((y > std::min(p1.y, p2.y))
                && (y <= std::max(p1.y, p2.y))
                && (x <= std::max(p1.x, p2.x))) {
                // Calculate the x-coordinate of the
                // intersection of the edge with a horizontal
                // line through the point
                double xIntersect = (y - p1.y)
                    * (p2.x - p1.x)
                    / (p2.y - p1.y)
                    + p1.x;
                // If the edge is vertical or the point's
                // x-coordinate is less than or equal to the
                // intersection x-coordinate, increment count
                if (p1.x == p2.x || x <= xIntersect) {
                    count++;
                }
            }
        }
        // If the number of intersections is odd, the point is
        // inside the polygon
        return count % 2 == 1;
    }

    bool PointInside(float x, float y) {
        switch (type)
        {
        case VectorShapeType::Polygon:
            return PointInside_Polygon(x, y);
        case VectorShapeType::Circle:
            return PointInside_Circle(x, y);
        default:
            return false;
        }
    }
};

struct Brush {
    float width;
    colour colour;
    VectorShape* shape;
};


struct Rect {
    float x, y, w, h;
};
struct IRect {
    IRect() {}
    IRect(int X, int Y, int W, int H) : x(X), y(Y), w(W), h(H) {}

    int x = 0, y = 0, w = 0, h = 0;
};




//supports canvasses up to 65000 px
struct Stroke {

    struct colourPair {
        colour prev;
        colour next;
    };

    std::unordered_map<uint32_t, colour> changes;
};

struct Layer {
    SDL_Surface* surface = NULL;
    SDL_Texture* gpuTexture = NULL;
    IRect area;

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

    void Render(SDL_Renderer* renderer, float xo, float yo, float s) {
        if (!synced) {
            Commit();
        }

        SDL_FRect r{ area.x + (xo * s),area.y + (yo * s),area.w * s, area.h * s};
        SDL_RenderTexture(renderer, gpuTexture, NULL, &r);
    }

    void DrawPoint(Brush& brush, int x, int y);
};

struct Canvas {
    std::vector<Layer*> layers;
};

