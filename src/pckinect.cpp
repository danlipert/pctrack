#include "defs.hpp"

#include <unistd.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "libfreenect.h"
#include "libfreenect_sync.h"

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

const unsigned short *get_depth() {
    void *p;
    uint32_t ts;
    int r;
    r = freenect_sync_get_depth(
        &p, &ts, 0, FREENECT_DEPTH_REGISTERED);
    if (r < 0) {
        die("Could not get depth data.");
    }
    return static_cast<const unsigned short *>(p);
}

const unsigned char *get_color() {
    void *p;
    uint32_t ts;
    int r;
    r = freenect_sync_get_video(
        &p, &ts, 0, FREENECT_VIDEO_RGB);
    if (r < 0) {
        die("Could not get color data.");
    }
    return static_cast<const unsigned char *>(p);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        die("Usage: pckinect FILE FRAME_COUNT KEY_DISTANCE_MM");
    }

    int frame_count = std::stoi(argv[2]);
    if (frame_count < 1) {
        die("Frame count is negative.");
    }
    int key_distance = std::stoi(argv[3]);
    if (key_distance <= 0 || key_distance > 1000) {
        die("Key distance must be positive and no more than 1000.");
    }

    std::fputs("Creating depth key.\n", stderr);
    std::vector<unsigned short> depth_key(WIDTH * HEIGHT);
    {
        const unsigned short *depth;
        while (true) {
            depth = get_depth();

            // Fill holes by erosion.  Erosion is always at least 1.
            int holes = 0;
            for (int i = 0; i < WIDTH * HEIGHT; i++) {
                holes += depth[i] == 0;
            }
            double frac = (double) holes * (1.0 / (WIDTH * HEIGHT));
            std::fprintf(stderr, "    Filling %d pixels (%.1f%%).\n",
                         holes, frac * 100.0);
            if (frac > 0.25) {
                std::fprintf(stderr, "    Trying another frame...\n");
            } else {
                break;
            }
        }

        int unfilled = 0;
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                unsigned short d = 0xffff;
                for (int r = 1; r < 64; r++) {
                    int y0 = std::max(y - r, 0);
                    int y1 = std::min(y + r + 1, HEIGHT);
                    int x0 = std::max(x - r, 0);
                    int x1 = std::min(x + r + 1, WIDTH);
                    for (int yy = y0; yy < y1; yy++) {
                        for (int xx = x0; xx < x1; xx++) {
                            unsigned short dd = depth[yy * WIDTH + xx];
                            if (!dd) {
                                continue;
                            } else if (dd < d) {
                                d = dd;
                            }
                        }
                    }
                    if (d != 0xffff) {
                        break;
                    }
                }
                if (d == 0xffff) {
                    d = 0;
                    unfilled++;
                }
                depth_key[y * WIDTH + x] = d;
            }
        }
        std::fprintf(stderr, "   Could not fill %d pixels.\n", unfilled);
    }

    std::fputs("Sleeping 2 seconds.\n", stderr);
    sleep(2);

    std::fprintf(stderr, "Writing data to %s.\n", argv[1]);
    FILE *fp = std::fopen(argv[1], "wb");
    if (!fp) {
        die("Could not open file.");
    }

    std::vector<Point> points;
    for (int i = 0; i < frame_count; i++) {
        const unsigned short *depth = get_depth();
        const unsigned char *color = get_color();

        // Convert data to points.
        points.clear();
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                int d = depth[y*WIDTH+x];
                int dk = depth_key[y*WIDTH+x];
                unsigned c = read_color(color + (y * WIDTH + x) * 3);
                if (!d || d >= dk - key_distance) {
                    continue;
                }
                float z = 0.001f * d;
                float scaleFactor = 0.0021f;
                points.push_back(Point{
                    { static_cast<float>(WIDTH  / 2 - x) * (z * scaleFactor),
                      static_cast<float>(HEIGHT / 2 - y) * (z * scaleFactor),
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
