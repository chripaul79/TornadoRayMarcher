#ifndef SHADER_HPP
#define SHADER_HPP
#pragma once

// System headers
#include <glad/glad.h>

// Standard headers
#include <cassert>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>


namespace Tornado
{
    class Shader
    {
    private:

        // Private member variables
        GLuint mProgram;
        GLint  mStatus;
        GLint  mLength;

        static std::string preprocessIncludes(const std::string& source,
                                              const std::string& baseDir,
                                              int depth = 0)
        {
            if (depth > 16) return source;
            std::string result;
            std::istringstream stream(source);
            std::string line;
            while (std::getline(stream, line)) {
                std::string trimmed = line;
                auto first = trimmed.find_first_not_of(" \t");
                if (first != std::string::npos)
                    trimmed = trimmed.substr(first);

                if (trimmed.rfind("#include", 0) == 0) {
                    auto q1 = trimmed.find('"');
                    auto q2 = trimmed.find('"', q1 + 1);
                    if (q1 != std::string::npos && q2 != std::string::npos) {
                        std::string incFile = trimmed.substr(q1 + 1, q2 - q1 - 1);
                        std::string fullPath = baseDir + incFile;
                        std::ifstream incFd(fullPath);
                        if (incFd.is_open()) {
                            auto incSrc = std::string(
                                std::istreambuf_iterator<char>(incFd),
                                std::istreambuf_iterator<char>());
                            result += preprocessIncludes(incSrc, baseDir, depth + 1);
                        } else {
                            fprintf(stderr, "Shader #include error: could not open \"%s\"\n",
                                    fullPath.c_str());
                            result += "// ERROR: could not open include: " + fullPath + "\n";
                        }
                        continue;
                    }
                }
                result += line + "\n";
            }
            return result;
        }

    public:
        Shader() {
            mProgram = glCreateProgram();
        }

        // Public member functions
        void   activate()   { glUseProgram(mProgram); }
        void   deactivate() { glUseProgram(0); }
        GLuint get()        { return mProgram; }
        void   destroy()    { glDeleteProgram(mProgram); }

        /* Attach a shader to the current shader program */
        void attach(std::string const &filename)
        {
            // Load GLSL Shader from source
            std::ifstream fd(filename.c_str());
            if (fd.fail())
            {
                fprintf(stderr,
                    "Something went wrong when attaching the Shader file at \"%s\".\n"
                    "The file may not exist or is currently inaccessible.\n",
                    filename.c_str());
                return;
            }
            auto src = std::string(std::istreambuf_iterator<char>(fd),
                                  (std::istreambuf_iterator<char>()));

            // Resolve #include directives before compilation.
            auto lastSlash = filename.find_last_of("/\\");
            std::string baseDir = (lastSlash != std::string::npos)
                                    ? filename.substr(0, lastSlash + 1)
                                    : "";
            src = preprocessIncludes(src, baseDir);

            // Create shader object
            const char * source = src.c_str();
            auto shader = create(filename);
            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);

            std::cout << "Loading shader: " << filename << std::endl;

            // Display errors
            glGetShaderiv(shader, GL_COMPILE_STATUS, &mStatus);
            if (!mStatus)
            {
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &mLength);
                std::unique_ptr<char[]> buffer(new char[mLength]);
                glGetShaderInfoLog(shader, mLength, nullptr, buffer.get());
                fprintf(stderr, "%s\n%s", filename.c_str(), buffer.get());
            }

            assert(mStatus);

            // Attach shader and free allocated memory
            glAttachShader(mProgram, shader);
            glDeleteShader(shader);
        }


        /* Links all attached shaders together into a shader program */
        void link()
        {
            // Link all attached shaders
            glLinkProgram(mProgram);

            // Display errors
            glGetProgramiv(mProgram, GL_LINK_STATUS, &mStatus);
            if (!mStatus)
            {
                glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &mLength);
                std::unique_ptr<char[]> buffer(new char[mLength]);
                glGetProgramInfoLog(mProgram, mLength, nullptr, buffer.get());
                fprintf(stderr, "%s\n", buffer.get());
            }

            assert(mStatus);
        }


        /* Convenience function that attaches and links a vertex and a
           fragment shader in a shader program */
        void makeBasicShader(std::string const &vertexFilename,
                             std::string const &fragmentFilename)
        {
            attach(vertexFilename);
            attach(fragmentFilename);
            link();
        }

        /* Convenience function to get a uniforms ID from a string
           containing its name */
        GLint getUniformFromName(std::string const &uniformName) {
            return glGetUniformLocation(this->get(), uniformName.c_str());
        }


        /* Used for debugging shader programs (expensive to run) */
        bool isValid()
        {
            // Validate linked shader program
            glValidateProgram(mProgram);

            // Display errors
            glGetProgramiv(mProgram, GL_VALIDATE_STATUS, &mStatus);
            if (!mStatus)
            {
                glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &mLength);
                std::unique_ptr<char[]> buffer(new char[mLength]);
                glGetProgramInfoLog(mProgram, mLength, nullptr, buffer.get());
                fprintf(stderr, "%s\n", buffer.get());
                return false;
            }
            return true;
        }


        /* Helper function for creating shaders */
        GLuint create(std::string const &filename)
        {
            // Extract file extension and create the correct shader type
            auto idx = filename.rfind(".");
            auto ext = filename.substr(idx + 1);
                 if (ext == "comp") return glCreateShader(GL_COMPUTE_SHADER);
            else if (ext == "frag") return glCreateShader(GL_FRAGMENT_SHADER);
            else if (ext == "geom") return glCreateShader(GL_GEOMETRY_SHADER);
            else if (ext == "tcs")  return glCreateShader(GL_TESS_CONTROL_SHADER);
            else if (ext == "tes")  return glCreateShader(GL_TESS_EVALUATION_SHADER);
            else if (ext == "vert") return glCreateShader(GL_VERTEX_SHADER);
            else                    return false;
        }

    private:
        // Disable copying and assignment
        Shader(Shader const &) = delete;
        Shader & operator =(Shader const &) = delete;

    };
}

#endif
