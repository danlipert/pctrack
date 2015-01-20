#include "defs.hpp"

#include "libfreenect.h"
#include "libfreenect_sync.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

const int WIDTH = 640;
const int HEIGHT = 480;

struct Point {
    // Location in space, in meters.
    float v[3];
    // Color, RGB0.
    unsigned color;
};

unsigned read_color(const unsigned char *p) {
    union {
        unsigned char uc[4];
        unsigned ui;
    } c;
    c.uc[0] = p[0];
    c.uc[1] = p[1];
    c.uc[2] = p[2];
    c.uc[3] = 0;
    return c.ui;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        die("Usage: pckinect FILE FRAME_COUNT");
    }

    int frame_count = std::stoi(argv[2]);
    if (frame_count < 1) {
        die("Frame count is negative.");
    }

    std::fprintf(stderr, "Writing data to %s.", argv[1]);
    FILE *fp = std::fopen(argv[1], "wb");
    if (!fp) {
        die("Could not open file.");
    }

    std::vector<Point> points;
    for (int i = 0; i < frame_count; i++) {
        const unsigned short *depth;
        const unsigned char *color;

        // Read data from Kinect
        {
            void *p;
            uint32_t ts;
            int r;
            r = freenect_sync_get_depth(
                &p, &ts, 0, FREENECT_DEPTH_11BIT);
            if (r < 0) {
                die("Could not get depth data.");
            }
            depth = static_cast<const unsigned short *>(p);
            r = freenect_sync_get_video(
                &p, &ts, 0, FREENECT_VIDEO_RGB);
            if (r < 0) {
                die("Could not get color data.");
            }
            color = static_cast<const unsigned char *>(p);
        }

        // Convert data to points.
        points.clear();
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                int d = depth[y*HEIGHT+x];
                unsigned c = read_color(color + (y * HEIGHT + x) * 3);
                if (d >= 2047) {
                    continue;
                }
                float z = 0.1236f * std::tan(
                    static_cast<float>(d) * (1.0f / 2842.5f) + 1.1863f);
                float minDistance = -10.0f;
                float scaleFactor = 0.0021f;
                points.push_back(Point{
                    { static_cast<float>(x - WIDTH / 2) *
                      (z + minDistance) * scaleFactor,
                      static_cast<float>(y - HEIGHT / 2) *
                      (z + minDistance) * scaleFactor,
                      z },
                    c
                });
            }
        }

        {
            unsigned n = points.size();
            std::size_t r;
            r = std::fwrite(&n, sizeof(n), 1, fp);
            if (r != 1) {
                die("Could not write data.");
            }
            r = std::fwrite(points.data(), sizeof(Point), n, fp);
            if (r != n) {
                die("Could not write data.");
            }
        }
    }
}
