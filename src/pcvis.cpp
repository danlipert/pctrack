#include "defs.hpp"
#include "sggl/3_3.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <fstream>

#include "SDL.h"
#define GLM_FORCE_RADIANS 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

namespace {

const int WIDTH = 1280;
const int HEIGHT = 720;
SDL_Window *g_window;
SDL_GLContext g_context;

__attribute__((noreturn))
void die_sdl(const char *what) {
    die("%s: %s", what, SDL_GetError());
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

struct Points {
    GLint a_pos, a_color;

    GLint u_mvp;

    static const ShaderField FIELDS[];
};

#define F(x) offsetof(Points, x)
const ShaderField Points::FIELDS[] = {
    { "a_pos", F(a_pos) },
    { "a_color", F(a_color) },
    { 0, 0 },

    { "MVP", F(u_mvp) },
    { 0, 0 }
};
#undef F


int main(int argc, char *argv[]) {
    using namespace gl_3_3;
    if (argc < 2) {
        die("Usage: pcvis FILE");
    }

    sdl_init();
    if (sggl_init()) {
        die("Could not load OpenGL functions");
    }

    {
        std::ifstream fp;
        fp.open(argv[1]);
        if (!fp.good()) {
            die("Could not open file: %s", argv[1]);
        }

        GLuint buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, 640 * 480 * 16, nullptr,
                     GL_STREAM_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        ProgramObj<Points> prog_points;
        GLuint arr;
        if (!prog_points.load("points", "points")) {
            die("Could not load shader program.");
        }
        glGenVertexArrays(1, &arr);

        {
            const auto &prog = *prog_points;
            glBindVertexArray(arr);
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            if (prog.a_pos >= 0) {
                glEnableVertexAttribArray(prog.a_pos);
                glVertexAttribPointer(
                    prog.a_pos, 3, GL_FLOAT, GL_FALSE, 16,
                    reinterpret_cast<const void *>(0));
            }
            if (prog.a_color >= 0) {
                glEnableVertexAttribArray(prog.a_color);
                glVertexAttribPointer(
                    prog.a_color, 3, GL_UNSIGNED_BYTE, GL_TRUE, 16,
                    reinterpret_cast<const void *>(12));
            }
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }

        int point_count = 0;
        while (sdl_handle_events()) {
            int width, height;
            SDL_GL_GetDrawableSize(g_window, &width, &height);

            if (point_count == 0) {
                int count = 0;
                fp.read(reinterpret_cast<char *>(&count), sizeof(count));
                if (fp.good() && count > 0) {
                    assert(count <= 640 * 480);
                    glBindBuffer(GL_ARRAY_BUFFER, buffer);
                    if (true) {
                        void *ptr = glMapBuffer(
                            GL_ARRAY_BUFFER, GL_WRITE_ONLY);
                        assert(ptr != nullptr);
                        fp.read(static_cast<char *>(ptr), count * 16);
                        if (!fp.good()) {
                            die("Could not read data.");
                        }
                        glUnmapBuffer(GL_ARRAY_BUFFER);
                    } else {
                        std::vector<char> data(count * 16);
                        fp.read(data.data(), count * 16);
                        glBufferSubData(GL_ARRAY_BUFFER,
                                        0, count * 16, data.data());
                    }
                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                    point_count = count;
                }
            }

            glViewport(0, 0, width, height);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            {
                const auto &prog = prog_points;
                glUseProgram(prog.prog());
                glBindVertexArray(arr);
                // glEnable(GL_DEPTH_TEST);

                float aspect = (float) width / (float) height;
                float fovy = std::atan(1.0f); // 45 degrees
                glm::mat4 mvp =
                    glm::perspective(fovy, aspect, 0.1f, 30.0f) *
                    glm::translate(glm::mat4(1.0f),
                                   glm::vec3(0.0f, 0.0f, -10.0f)) *
                    glm::rotate(glm::mat4(1.0f),
                                (float) SDL_GetTicks() * 0.001f,
                                glm::vec3(0.0f, 1.0f, 0.0f));
                glUniformMatrix4fv(
                    prog->u_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

                glDrawArrays(GL_POINTS, 0, point_count);
            }

            {
                GLenum err;
                while ((err = glGetError())) {
                    std::fprintf(stderr, "OpenGL error: 0x%04x\n", err);
                }
            }

            SDL_GL_SwapWindow(g_window);
        }
    }

    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}
