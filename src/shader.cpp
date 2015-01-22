#include "defs.hpp"
#include "sggl/3_2.h"

#include <cassert>
#include <cstring>
#include <limits>

using namespace gl_2_0;

////////////////////////////////////////////////////////////

namespace {
std::string g_shader_search_path("src/glsl/");
};

Shader::~Shader() {
    if (value) {
        glDeleteShader(value);
    }
}

const struct {
    char ext[5];
    GLenum type;
} SHADER_EXTENSION[] = {
    { "comp", 0x91B9 /* GL_COMPUTE_SHADER */ },
    { "vert", 0x8B31 /* GL_VERTEX_SHADER */ },
    { "tesc", 0x8E88 /* GL_TESS_CONTROL_SHADER */ },
    { "tese", 0x8E87 /* GL_TESS_EVALUATION_SHADER */ },
    { "geom", 0x8DD9 /* GL_GEOMETRY_SHADER */ },
    { "frag", 0x8B30 /* GL_FRAGMENT_SHADER */ }
};

std::string shader_file_extension(GLenum type)
{
    for (const auto &ext : SHADER_EXTENSION) {
        if (ext.type == type) {
            return std::string(ext.ext);
        }
    }
    die("Unknown shader type: 0x%04x", type);
    return std::string();
}

Shader Shader::load(const std::string &path, GLenum type) {
    Shader shader;
    shader.value = glCreateShader(type);
    if (!shader) {
        die("Could not create shader.");
    }
    std::string fullpath(g_shader_search_path);
    if (!fullpath.empty() && fullpath[fullpath.size() - 1] != '/') {
        fullpath += '/';
    }
    fullpath += path;
    fullpath += '.';
    fullpath += shader_file_extension(type);
    std::vector<char> data = load_file(fullpath);
    assert(data.size() <= std::numeric_limits<int>::max());
    const char *src[1] = { data.data() };
    GLint srclen[1] = { static_cast<int>(data.size()) };
    glShaderSource(shader.value, 1, src, srclen);
    glCompileShader(shader.value);
    GLint flag;
    glGetShaderiv(shader.value, GL_COMPILE_STATUS, &flag);
    if (!flag) {
        std::fprintf(stderr, "Error: %s: compilation failed.\n",
                     fullpath.c_str());
    }
    GLint loglen;
    glGetShaderiv(shader.value, GL_INFO_LOG_LENGTH, &loglen);
    if (loglen > 1) {
        std::vector<char> log(loglen + 1);
        log[0] = '\0';
        glGetShaderInfoLog(shader.value, loglen, nullptr, log.data());
        log[loglen] = '\0';
        std::fputs(log.data(), stderr);
        std::fputc('\n', stderr);
    }
    if (!flag) {
        return Shader();
    }
    shader.name = std::move(fullpath);
    return std::move(shader);
}

void Shader::set_search_path(const std::string &path) {
    g_shader_search_path = path;
}

//////////////////////////////////////////////////////////////////////

Program::~Program() {
    if (value) {
        glDeleteProgram(value);
    }
}

Program Program::load(const std::vector<Shader> &shaders) {
    Program prog;
    prog.value = glCreateProgram();
    if (!prog) {
        die("Could not create program.");
    }
    for (const auto &shader : shaders) {
        glAttachShader(prog.value, shader.value);
    }
    bool success = prog.link();
    if (!success) {
        return Program();
    }
    for (const auto &shader : shaders) {
        if (!prog.name.empty()) {
            prog.name += ',';
        }
        prog.name += shader.name;
    }
    return std::move(prog);
}

bool Program::link() {
    glLinkProgram(value);
    GLint flag;
    glGetProgramiv(value, GL_LINK_STATUS, &flag);
    if (!flag) {
        std::fprintf(stderr, "Error: %s: linking failed.\n", name.c_str());
    }
    GLint loglen;
    glGetProgramiv(value, GL_INFO_LOG_LENGTH, &loglen);
    if (loglen > 1) {
        std::vector<char> log(loglen + 1);
        log[0] = '\0';
        glGetProgramInfoLog(value, loglen, nullptr, log.data());
        log[loglen] = '\0';
        std::fputs(log.data(), stderr);
        std::fputc('\n', stderr);
    }
    return flag != 0;
}

//////////////////////////////////////////////////////////////////////

GLint &get_field(const ShaderField &field, void *object) {
    return *reinterpret_cast<GLint *>(
        static_cast<char *>(object) + field.offset);
}

/// Load the indexes of shader uniforms into an object.
void get_uniforms(const Program &program, const ShaderField *uniforms,
                  void *object) {
    for (int i = 0; uniforms[i].name; i++) {
        GLint value = glGetUniformLocation(program.value, uniforms[i].name);
        if (value < 0) {
            std::fprintf(stderr, "Warning: %s: Uniform does not exist: %s",
                         program.name.c_str(), uniforms[i].name);
        }
        get_field(uniforms[i], object) = value;
    }
    GLint ucount = 0;
    glGetProgramiv(program.value, GL_ACTIVE_UNIFORMS, &ucount);
    for (int j = 0; j < ucount; j++) {
        char name[32];
        name[0] = '\0';
        GLint size;
        GLenum type;
        glGetActiveUniform(program.value, j, sizeof(name), nullptr,
                           &size, &type, name);
        for (int i = 0; ; i++) {
            if (!uniforms[i].name) {
                std::fprintf(stderr, "Warning: %s: Unbound uniform: %s",
                             program.name.c_str(), name);
                break;
            }
            if (!std::strcmp(name, uniforms[i].name)) {
                break;
            }
        }
    }
}

/// Load the indexes of shader attributes into an object.
void get_attributes(const Program &program, const ShaderField *attributes,
                    void *object) {
    for (int i = 0; attributes[i].name; i++) {
        GLint *ptr = reinterpret_cast<GLint *>(
            static_cast<char *>(object) + attributes[i].offset);
        *ptr = glGetAttribLocation(program.value, attributes[i].name);
    }
}

bool load_program(const std::string &vertex_shader,
                  const std::string &fragment_shader,
                  const std::string &geometry_shader,
                  Program &prog,
                  const ShaderField *fields,
                  void *object) {
    using namespace gl_3_2;
    std::vector<Shader> shaders;
    shaders.push_back(Shader::load(vertex_shader, GL_VERTEX_SHADER));
    if (!geometry_shader.empty()) {
        shaders.push_back(Shader::load(geometry_shader, GL_GEOMETRY_SHADER));
    }
    shaders.push_back(Shader::load(fragment_shader, GL_FRAGMENT_SHADER));
    for (const auto &s : shaders) {
        if (!s) {
            return false;
        }
    }

    Program p = Program::load(shaders);
    shaders.clear();
    if (!p) {
        return false;
    }

    const ShaderField *attributes = fields, *uniforms = fields;
    while (uniforms->name) {
        uniforms++;
    }
    uniforms++;
    get_uniforms(p, uniforms, object);
    get_attributes(p, attributes, object);
    prog = std::move(p);
    return true;
}
