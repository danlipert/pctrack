#include <cstdarg>
#include <string>
#include <utility>
#include <vector>

#include "sggl/common.h"

__attribute__((noreturn))
void diev(const char *msg, va_list ap);

__attribute__((noreturn))
void die(const char *msg, ...);

std::vector<char> load_file(const std::string &path);

//////////////////////////////////////////////////////////////////////
// Wrapper for OpenGL shader objects

struct Shader {
    std::string name;
    GLuint value;

    Shader();
    Shader(const Shader &) = delete;
    Shader(Shader &&);
    ~Shader();
    Shader &operator=(const Shader &) = delete;
    Shader &operator=(Shader &&);
    explicit operator bool() const;

    /// Load the shader from the given path.  The path should be
    /// relative to the directory containing shaders, and it should
    /// NOT include the extension, which is determined automatically
    /// from the shader's type (e.g. GL_VERTEX_SHADER files have a
    /// .vert extension).  This will return a zero shader if loading
    /// or compilation fails.  Errors will be logged to the console.
    static Shader load(const std::string &path, GLenum type);
};

inline Shader::Shader() : value(0) {}

inline Shader::Shader(Shader &&other) : value(other.value) {
    other.value = 0;
}

inline Shader &Shader::operator=(Shader &&other) {
    std::swap(value, other.value);
    return *this;
}

inline Shader::operator bool() const {
    return value != 0;
}

//////////////////////////////////////////////////////////////////////
// Wrapper for OpenGL program objects

struct Program {
    std::string name;
    GLuint value;

    Program();
    Program(const Program &) = delete;
    Program(Program &&);
    ~Program();
    Program &operator=(const Program &) = delete;
    Program &operator=(Program &&);
    explicit operator bool() const;

    /// Load a program from the given shaders.  This will link the
    /// program, and print diagnostic messages to the console.  If
    /// this fails, it will return a zero program.
    static Program load(const std::vector<Shader> &shaders);

    /// Link the program, and print any diagnostic messages.  It is
    /// not necessary to call this on a program loaded with load().
    bool link();
};

inline Program::Program() : value(0) {}

inline Program::Program(Program &&other) : value(other.value) {
    other.value = 0;
}

inline Program &Program::operator=(Program &&other) {
    std::swap(value, other.value);
    return *this;
}

inline Program::operator bool() const {
    return value != 0;
}

//////////////////////////////////////////////////////////////////////
// Wrapper for OpenGL program objects + uniform locations

/// A field in an object which stores program attributes and uniform indexes.
struct ShaderField {
    /// The name of the uniform or attribute.
    const char *name;

    /// The offset of the corresponding GLint field in the structure.
    std::size_t offset;
};

bool load_program(const std::string &vertex_shader,
                  const std::string &fragment_shader,
                  const std::string &geometry_shader,
                  Program &prog,
                  const ShaderField *fields,
                  void *object);

template<class T>
class ProgramObj {
private:
    Program m_prog;
    T m_fields;

public:
    ProgramObj() = default;
    ProgramObj(const ProgramObj &) = delete;
    ProgramObj(ProgramObj &&) = default;
    ~ProgramObj() = default;
    ProgramObj &operator=(const ProgramObj &) = delete;
    ProgramObj &operator=(ProgramObj &&) = default;

    /// Load the given shader program.
    bool load(const std::string &vertex_shader,
              const std::string &fragment_shader,
              const std::string &geometry_shader = std::string());
    /// Get program attribute and uniform indexes.
    const T &operator*() const { return m_fields; }
    /// Get program attribute and uniform indexes.
    const T *operator->() const { return &m_fields; }
    /// Get the program attribute and uniform locations.
    GLuint prog() const { return m_prog.value; }
    /// Test whether the program is loaded.
    bool is_loaded() const { return m_prog.value != 0; }
};

template<class T>
bool ProgramObj<T>::load(const std::string &vertex_shader,
                         const std::string &fragment_shader,
                         const std::string &geometry_shader) {
    return load_program(vertex_shader, fragment_shader, geometry_shader,
                        m_prog, T::FIELDS, &m_fields);
}
