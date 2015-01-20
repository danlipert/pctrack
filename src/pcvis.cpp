#include "defs.hpp"
#include "sggl/3_0.h"

#include "SDL.h"
#include <cstdlib>
#include <cstdio>

namespace {

const int WIDTH = 1280;
const int HEIGHT = 720;
SDL_Window *g_window;
SDL_GLContext g_context;

__attribute__((noreturn))
void die_sdl(const char *what) {
    die("%s: %s", SDL_GetError());
}

void sdl_init() {
    unsigned flags;

    flags = SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS;
    if (SDL_Init(flags)) {
        die_sdl("Could not initialize LibSDL");
    }

    flags = (SDL_WINDOW_OPENGL |
             SDL_WINDOW_ALLOW_HIGHDPI |
             SDL_WINDOW_RESIZABLE);
    g_window = SDL_CreateWindow(
        "PCTrack",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WIDTH,
        HEIGHT,
        flags);
    if (!g_window) {
        die_sdl("Could not open window");
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    g_context = SDL_GL_CreateContext(g_window);
    if (!g_context) {
        die_sdl("Could not create OpenGL context");
    }
}

bool sdl_handle_events() {
    SDL_PumpEvents();
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            return false;
        default:
            break;
        }
    }
    return true;
}

}

int main(int argc, char *argv[]) {
    sdl_init();
    if (sggl_init()) {
        die("Could not load OpenGL functions");
    }
    while (sdl_handle_events()) {
        int width, height;
        SDL_GL_GetDrawableSize(g_window, &width, &height);

        SDL_GL_SwapWindow(g_window);
    }
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}
