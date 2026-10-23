#include "Linedraw.h"

#include <cmath>


namespace Drawing {

    void line_low(Brush& brush, Layer* layer, int x1, int y1, int x2, int y2) {
        int dx = x2 - x1;
        int dy = y2 - y1;
        int yi = 1;
        if (dy < 0) {
            yi = -1;
            dy = -dy;
        }
        int D = (2 * dy) - dx;
        int y = y1;

        for (int x = x1; x < x2; ++x)
        {
            layer->DrawPoint(brush, x, y);

            if (D > 0) {
                y += yi;
                D = D + (2 * (dy - dx));
            }
            else {
                D = D + 2 * dy;
            }
        }

    }

    void line_high(Brush& brush, Layer* layer, int x1, int y1, int x2, int y2) {
        int dx = x2 - x1;
        int dy = y2 - y1;
        int xi = 1;
        if (dx < 0) {
            xi = -1;
            dx = -dx;
        }
        int D = (2 * dx) - dy;
        int x = x1;

        for (int y = y1; y < y2; ++y)
        {

            layer->DrawPoint(brush, x, y);
            if (D > 0) {
                x += xi;
                D = D + (2 * (dx - dy));
            }
            else {
                D = D + 2 * dx;
            }
        }

    }

    void DrawLine(Brush& brush, Layer* layer, int x1, int y1, int x2, int y2) {

        //printf("[%d, %d] -> [%d, %d]\n",x1,y1,x2,y2);
        if (abs(y2 - y1) < abs(x2 - x1)) {
            if (x1 > x2) {
                line_low(brush, layer, x2, y2, x1, y1);
            }
            else {
                line_low(brush, layer, x1, y1, x2, y2);
            }
        }
        else {
            if (y1 > y2) {
                line_high(brush, layer, x2, y2, x1, y1);
            }
            else {
                line_high(brush, layer, x1, y1, x2, y2);
            }
        }
    }
}
