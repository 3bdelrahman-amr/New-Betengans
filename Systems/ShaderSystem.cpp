#include "ShaderSystem.h"
//#include<stb/stb_include.h>











// stb_include.h - v0.02 - parse and process #include directives - public domain
//
// To build this, in one source file that includes this file do
//      #define STB_INCLUDE_IMPLEMENTATION
//
// This program parses a string and replaces lines of the form
//         #include "foo"
// with the contents of a file named "foo". It also embeds the
// appropriate #line directives. Note that all include files must
// reside in the location specified in the path passed to the API;
// it does not check multiple directories.
//
// If the string contains a line of the form
//         #inject
// then it will be replaced with the contents of the string 'inject' passed to the API.
//
// Options:
//
//      Define STB_INCLUDE_LINE_GLSL to get GLSL-style #line directives
//      which use numbers instead of filenames.
//
//      Define STB_INCLUDE_LINE_NONE to disable output of #line directives.
//
// Standard libraries:
//
//      stdio.h     FILE, fopen, fclose, fseek, ftell
//      stdlib.h    malloc, realloc, free
//      string.h    strcpy, strncmp, memcpy
//
// Credits:
//
// Written by Sean Barrett.
//
// Fixes:
//  Michal Klos

#ifndef STB_INCLUDE_STB_INCLUDE_H
#define STB_INCLUDE_STB_INCLUDE_H

// Do include-processing on the string 'str'. To free the return value, pass it to free()
char* stb_include_string(char* str, char* inject, char* path_to_includes, char* filename_for_line_directive, char error[256]);

// Concatenate the strings 'strs' and do include-processing on the result. To free the return value, pass it to free()
char* stb_include_strings(char** strs, int count, char* inject, char* path_to_includes, char* filename_for_line_directive, char error[256]);

// Load the file 'filename' and do include-processing on the string therein. note that
// 'filename' is opened directly; 'path_to_includes' is not used. To free the return value, pass it to free()
char* stb_include_file(char* filename, char* inject, char* path_to_includes, char error[256]);

#endif




#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* stb_include_load_file(char* filename, size_t* plen)
{
    char* text;
    size_t len;
    FILE* f = fopen(filename, "rb");
    if (f == 0) return 0;
    fseek(f, 0, SEEK_END);
    len = (size_t)ftell(f);
    if (plen) *plen = len;
    text = (char*)malloc(len + 1);
    if (text == 0) return 0;
    fseek(f, 0, SEEK_SET);
    fread(text, 1, len, f);
    fclose(f);
    text[len] = 0;
    return text;
}

typedef struct
{
    int offset;
    int end;
    char* filename;
    int next_line_after;
} include_info;

static include_info* stb_include_append_include(include_info* array, int len, int offset, int end, char* filename, int next_line)
{
    include_info* z = (include_info*)realloc(array, sizeof(*z) * (len + 1));
    z[len].offset = offset;
    z[len].end = end;
    z[len].filename = filename;
    z[len].next_line_after = next_line;
    return z;
}

static void stb_include_free_includes(include_info* array, int len)
{
    int i;
    for (i = 0; i < len; ++i)
        free(array[i].filename);
    free(array);
}

static int stb_include_isspace(int ch)
{
    return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
}

// find location of all #include and #inject
static int stb_include_find_includes(char* text, include_info** plist)
{
    int line_count = 1;
    int inc_count = 0;
    char* s = text, * start;
    include_info* list = NULL;
    while (*s) {
        // parse is always at start of line when we reach here
        start = s;
        while (*s == ' ' || *s == '\t')
            ++s;
        if (*s == '#') {
            ++s;
            while (*s == ' ' || *s == '\t')
                ++s;
            if (0 == strncmp(s, "include", 7) && stb_include_isspace(s[7])) {
                s += 7;
                while (*s == ' ' || *s == '\t')
                    ++s;
                if (*s == '"') {
                    char* t = ++s;
                    while (*t != '"' && *t != '\n' && *t != '\r' && *t != 0)
                        ++t;
                    if (*t == '"') {
                        char* filename = (char*)malloc(t - s + 1);
                        memcpy(filename, s, t - s);
                        filename[t - s] = 0;
                        s = t;
                        while (*s != '\r' && *s != '\n' && *s != 0)
                            ++s;
                        // s points to the newline, so s-start is everything except the newline
                        list = stb_include_append_include(list, inc_count++, start - text, s - text, filename, line_count + 1);
                    }
                }
            }
            else if (0 == strncmp(s, "inject", 6) && (stb_include_isspace(s[6]) || s[6] == 0)) {
                while (*s != '\r' && *s != '\n' && *s != 0)
                    ++s;
                list = stb_include_append_include(list, inc_count++, start - text, s - text, NULL, line_count + 1);
            }
        }
        while (*s != '\r' && *s != '\n' && *s != 0)
            ++s;
        if (*s == '\r' || *s == '\n') {
            s = s + (s[0] + s[1] == '\r' + '\n' ? 2 : 1);
        }
        ++line_count;
    }
    *plist = list;
    return inc_count;
}

// avoid dependency on sprintf()
static void stb_include_itoa(char str[9], int n)
{
    int i;
    for (i = 0; i < 8; ++i)
        str[i] = ' ';
    str[i] = 0;

    for (i = 1; i < 8; ++i) {
        str[7 - i] = '0' + (n % 10);
        n /= 10;
        if (n == 0)
            break;
    }
}

static char* stb_include_append(char* str, size_t* curlen, char* addstr, size_t addlen)
{
    str = (char*)realloc(str, *curlen + addlen);
    memcpy(str + *curlen, addstr, addlen);
    *curlen += addlen;
    return str;
}

char* stb_include_string(char* str, char* inject, char* path_to_includes, char* filename, char error[256])
{
    char temp[4096];
    include_info* inc_list;
    int i, num = stb_include_find_includes(str, &inc_list);
    size_t source_len = strlen(str);
    char* text = 0;
    size_t textlen = 0, last = 0;
    for (i = 0; i < num; ++i) {
        text = stb_include_append(text, &textlen, str + last, inc_list[i].offset - last);
        // write out line directive for the include
#ifndef STB_INCLUDE_LINE_NONE
#ifdef STB_INCLUDE_LINE_GLSL
        if (textlen != 0)  // GLSL #version must appear first, so don't put a #line at the top
#endif
        {
            strcpy(temp, "#line ");
            stb_include_itoa(temp + 6, 1);
            strcat(temp, " ");
#ifdef STB_INCLUDE_LINE_GLSL
            stb_include_itoa(temp + 15, i + 1);
#else
            strcat(temp, "\"");
            if (inc_list[i].filename == 0)
                strcmp(temp, "INJECT");
            else
                strcat(temp, inc_list[i].filename);
            strcat(temp, "\"");
#endif
            strcat(temp, "\n");
            text = stb_include_append(text, &textlen, temp, strlen(temp));
        }
#endif
        if (inc_list[i].filename == 0) {
            if (inject != 0)
                text = stb_include_append(text, &textlen, inject, strlen(inject));
        }
        else {
            char* inc;
            strcpy(temp, path_to_includes);
            strcat(temp, "/");
            strcat(temp, inc_list[i].filename);
            inc = stb_include_file(temp, inject, path_to_includes, error);
            if (inc == NULL) {
                stb_include_free_includes(inc_list, num);
                return NULL;
            }
            text = stb_include_append(text, &textlen, inc, strlen(inc));
            free(inc);
        }
        // write out line directive
#ifndef STB_INCLUDE_LINE_NONE
        strcpy(temp, "\n#line ");
        stb_include_itoa(temp + 6, inc_list[i].next_line_after);
        strcat(temp, " ");
#ifdef STB_INCLUDE_LINE_GLSL
        stb_include_itoa(temp + 15, 0);
#else
        strcat(temp, filename != 0 ? filename : "source-file");
#endif
        text = stb_include_append(text, &textlen, temp, strlen(temp));
        // no newlines, because we kept the #include newlines, which will get appended next
#endif
        last = inc_list[i].end;
    }
    text = stb_include_append(text, &textlen, str + last, source_len - last + 1); // append '\0'
    stb_include_free_includes(inc_list, num);
    return text;
}

char* stb_include_strings(char** strs, int count, char* inject, char* path_to_includes, char* filename, char error[256])
{
    char* text;
    char* result;
    int i;
    size_t length = 0;
    for (i = 0; i < count; ++i)
        length += strlen(strs[i]);
    text = (char*)malloc(length + 1);
    length = 0;
    for (i = 0; i < count; ++i) {
        strcpy(text + length, strs[i]);
        length += strlen(strs[i]);
    }
    result = stb_include_string(text, inject, path_to_includes, filename, error);
    free(text);
    return result;
}

char* stb_include_file(char* filename, char* inject, char* path_to_includes, char error[256])
{
    size_t len;
    char* result;
    char* text = stb_include_load_file(filename, &len);
    if (text == NULL) {
        strcpy(error, "Error: couldn't load '");
        strcat(error, filename);
        strcat(error, "'");
        return 0;
    }
    result = stb_include_string(text, inject, path_to_includes, filename, error);
    free(text);
    return result;
}

#if 0 // @TODO, GL_ARB_shader_language_include-style system that doesn't touch filesystem
char* stb_include_preloaded(char* str, char* inject, char* includes[][2], char error[256])
{

}
#endif



















//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//#include <glad/gl.h>
void ShaderSystem:: create(Entity e, Manager* mangr) {
   // assert(Entities.find(e) != Entities.end());
    //auto shader = mangr->GetComponent<ShaderProg>(e);
   // mangr->AddComponent(e, ShaderProg());
    auto MeshRend = mangr->GetComponent<MeshRendererr>(e);
    MeshRend->shader.program = glCreateProgram();
    mng = mangr;
    //mangr->AddComponent(e, MeshRend);

}
//void ShaderSystem::destroy() {
//    for (auto entt : Entities) {
//        mng
//    }
//
//}
//GLuint getProgramId() { return program; }
//ShaderSystem(Manager*m): System(m) { //program = 0;
//}
//ShaderSystem::~ShaderSystem() { destroy(); }
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//Cast Class to an OpenGL Object name
// operator GLuint() const { return program; } // NOLINT: Allow implicit casting for convenience

 //Read shader from file, send it to GPU, compile it then attach it to shader
bool ShaderSystem:: attach(const std::string& filename, GLenum type, Entity e) const // NOLINT: attach does alter the object state so [[nodiscard]] is unneeded
{ // first, we use C++17 filesystem library to get the directory (parent) path of the file.
// the parent path will be sent to stb_include to search for files referenced by any "#include" preprocessor command.
    auto file_path = std::filesystem::path(filename);
    auto file_path_string = file_path.string();
    auto parent_path_string = file_path.parent_path().string();
    auto path_to_includes = &(parent_path_string[0]);
    char error[256];

    // Read the file as a string and resolve any "#include"s recursively
    auto source = stb_include_file(&(file_path_string[0]), nullptr, path_to_includes, error);

    // Check if any loading errors happened
    if (source == nullptr) {
        std::cerr << "ERROR: " << error << std::endl;
        return false;
    }

    GLuint shaderID = glCreateShader(type); //Create shader of the given type

    // Function parameter:
    // shader (GLuint): shader object name.
    // count (GLsizei): number of strings passed in the third parameter. We only have one string here.
    // string (const GLchar**): an array of source code strings.
    // lengths (const GLint*): an array of string lengths for each string in the third parameter. if null is passed,
    //           then the function will deduce the lengths automatically by searching for '\0'.
    glShaderSource(shaderID, 1, &source, nullptr); //Send shader source code
    glCompileShader(shaderID); //Compile the shader code
    free(source);

    //Check and log for any error in the compilation process
    GLint status;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &status);
    if (!status) {
        GLint length;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &length);
        char* logStr = new char[length];
        glGetShaderInfoLog(shaderID, length, nullptr, logStr);
        std::cerr << "ERROR IN" << filename << std::endl;
        std::cerr << logStr << std::endl;
        delete[] logStr;
        glDeleteShader(shaderID);
        return false;
    }
    auto MeshRend = mng->GetComponent<MeshRendererr>(e);


    
    glAttachShader(MeshRend->shader.program, shaderID); //Attach shader to program
    glDeleteShader(shaderID); //Delete shader (the shader is already attached to the program so its object is no longer needed)
    return true;
}
///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//Link Program (Do this after all shaders are attached)
bool ShaderSystem::link(Entity e) const // NOLINT: link does alter the object state so [[nodiscard]] is unneeded
{
    //Link
    auto MeshRend = mng->GetComponent<MeshRendererr>(e);

    glLinkProgram(MeshRend->shader.program);

    //Check and log for any error in the linking process
    GLint status;
    glGetProgramiv(MeshRend->shader.program, GL_LINK_STATUS, &status);
    if (!status) {
        GLint length;
        glGetProgramiv(MeshRend->shader.program, GL_INFO_LOG_LENGTH, &length);
        char* logStr = new char[length];
        glGetProgramInfoLog(MeshRend->shader.program, length, nullptr, logStr);
        std::cerr << "LINKING ERROR" << std::endl;
        std::cerr << logStr << std::endl;
        delete[] logStr;
        return false;
    }
    return true;


}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//Get the location of a uniform variable in the shader
GLuint ShaderSystem::getUniformLocation(const string& name, Entity e) {


    //assert(Entities.find(e) != Entities.end());

    auto MeshRend = mng->GetComponent<MeshRendererr>(e);
    auto it = MeshRend->shader.uniform_location_cache.find(name);

    if (it != MeshRend->shader.uniform_location_cache.end()) {
        return it->second; // We found the uniform in our cache, so no need to call OpenGL.
    }
    GLuint location = glGetUniformLocation(MeshRend->shader.program, name.c_str()); // The uniform was not found, so we retrieve its location
    MeshRend->shader.uniform_location_cache[name] = location; // and cache the location for later queries
    return location;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

//A group of setter for uniform variables
//NOTE: It is inefficient to call glGetUniformLocation every frame
//So it is usually a better option to either cache the location
//or explicitly define the uniform location in the shader
void ShaderSystem::set(const std::string& uniform, GLfloat value, Entity e) {
    //assert(Entities.find(e) != Entities.end());
    glUniform1f(getUniformLocation(uniform, e), value);
}
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////


void ShaderSystem::set(const std::string& uniform, GLint value, Entity e) {
    //assert(Entities.find(e) != Entities.end());
    glUniform1i(getUniformLocation(uniform, e), value);
}
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void ShaderSystem:: set(const std::string& uniform, GLboolean value, Entity e) {
    //assert(Entities.find(e) != Entities.end());
    glUniform1i(getUniformLocation(uniform, e), value);
}
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void ShaderSystem:: set(const std::string& uniform, glm::vec2 value, Entity e) {
    //assert(Entities.find(e) != Entities.end());
    glUniform2f(getUniformLocation(uniform, e), value.x, value.y);
}
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void ShaderSystem:: set(const std::string& uniform, glm::vec3 value, Entity e) {
    //assert(Entities.find(e) != Entities.end());
    glUniform3f(getUniformLocation(uniform, e), value.x, value.y, value.z);
}
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void ShaderSystem:: set(const std::string& uniform, glm::vec4 value, Entity e) {
    //assert(Entities.find(e) != Entities.end());
    glUniform4f(getUniformLocation(uniform, e), value.x, value.y, value.z, value.w);
}
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
void ShaderSystem:: set(const std::string& uniform, glm::mat4 value, Entity e, GLboolean transpose ) {
    //assert(Entities.find(e) != Entities.end());
    glUniformMatrix4fv(getUniformLocation(uniform, e), 1, transpose, glm::value_ptr(value));
}



//ShaderSystem::ShaderSystem(ShaderSystem const&) = delete;
//ShaderSystem::ShaderSystem& operator=(ShaderSystem const&) = delete;
