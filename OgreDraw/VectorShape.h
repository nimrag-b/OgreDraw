#pragma once
#include <cmath>
#include <algorithm>


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