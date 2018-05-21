/**
  * Copyright (C) 2017 flerovium^-^ (Frederic Meyer)
  *
  * This file is part of NanoboyAdvance.
  *
  * NanoboyAdvance is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * NanoboyAdvance is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with NanoboyAdvance. If not, see <http://www.gnu.org/licenses/>.
  */

#include <cstring>

#include "screen.hpp"
#include <QtWidgets>
#include <QGLFunctions>

#include "util/ini.hpp"
#include "util/file.hpp"

static const char* s_vertex_shader =
    "varying vec2 uv;\n"
    "void main(void) {\n"
    "    gl_Position = gl_Vertex;\n"
    "    uv = gl_MultiTexCoord0;\n"
    "}";

using namespace Util;

Screen::Screen(QtConfig* config, int width, int height, QWidget* parent) : QGLWidget(parent), width(width), height(height), config(config) {
    framebuffer = new u32[width * height];
    clear();
}

Screen::~Screen() {
    glDeleteTextures(1, &texture);

    delete framebuffer;
}

void Screen::updateTexture() {
    // Update texture pixels
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_BGRA,
        GL_UNSIGNED_BYTE,
        framebuffer
    );

    // Redraw screen
    updateGL();
}

void Screen::compileShader(std::string shader_source, std::unordered_map<std::string, float>& uniforms) {
    QGLFunctions ctx(QGLContext::currentContext());

    auto vid = ctx.glCreateShader(GL_VERTEX_SHADER);
    auto fid = ctx.glCreateShader(GL_FRAGMENT_SHADER);

    const char* vert_src[] = { s_vertex_shader };
    const char* frag_src[] = { shader_source.c_str() };

    ctx.glShaderSource(vid, 1, vert_src, nullptr);
    ctx.glShaderSource(fid, 1, frag_src, nullptr);
    ctx.glCompileShader(vid);
    ctx.glCompileShader(fid);

    auto pid = ctx.glCreateProgram();

    ctx.glAttachShader(pid, vid);
    ctx.glAttachShader(pid, fid);
    ctx.glLinkProgram(pid);

    ctx.glUseProgram(pid);

    for (auto uniform : uniforms) {
        auto location = ctx.glGetUniformLocation(pid, uniform.first.c_str());

        ctx.glUniform1f(location, uniform.second);
    }

    // TODO: check compilation status
}

auto Screen::sizeHint() const -> QSize {
    return QSize {480, 320};
}

void Screen::initializeGL() {
    qglClearColor(Qt::black);

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    std::string shader_path     = config->video.shader;
    std::string shader_path_fs  = shader_path + ".fs";
    std::string shader_path_ini = shader_path + ".ini";

    if (!shader_path.empty() && File::exists(shader_path_fs)) {
        std::unordered_map<std::string, float> uniforms;

        if (File::exists(shader_path_ini)) {
            Util::INI shader_ini { shader_path_ini, true };

            try {
                auto section = shader_ini.getSection("Uniform");

                for (auto& pair : *section) {
                    try {
                        uniforms[pair.first] = std::stof(pair.second->value);
                    } catch (std::exception& e) { }
                }
            } catch (INISectionNotFoundError& e) { }
        }
        compileShader(File::read_as_string(shader_path_fs), uniforms);
    }

    clear();
}

void Screen::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_QUADS);

    glTexCoord2f(0, 1.0f);
    glVertex2f(-1.0f, -1.0f);

    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(1.0f, -1.0f);

    glTexCoord2f(1.0f, 0);
    glVertex2f(1.0f, 1.0f);

    glTexCoord2f(0, 0);
    glVertex2f(-1.0f, 1.0f);

    glEnd();
}

void Screen::resizeGL(int width, int height) {
    int fixedWidth  = width;
    int sidePadding = 0;

    if (aspect_ratio) {
        fixedWidth  = height + height / 2;
        sidePadding = (width - fixedWidth) / 2;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glViewport(sidePadding, 0, fixedWidth, height);
}

void Screen::clear() {
    std::memset(framebuffer, 0, sizeof(u32) * width * height);
    updateTexture();
}
