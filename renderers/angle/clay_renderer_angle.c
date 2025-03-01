// This file contains the core rendering logic of the ANGLE/GLES renderer

#ifndef CLAY_ANGLE_OPENGL_HEADER
#include <angle_gl.h>
#else
#include CLAY_ANGLE_OPENG_HEADER
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../clay.h"

GLuint rectProgram;
GLuint rectBuffer;
GLuint rectVAO;
GLuint textProgram;
GLuint textBuffer;
GLuint textVAO;

typedef struct {
    vec4 position, color, sizeCenter, radius;
} RectVert;

typedef struct {
    void *data;
    unsigned long length, capacity, unitSize;
} Angle_Vector;

void GetFramebufferSize(int *width, int *height);

Angle_Vector angleMakeVector(unsigned long unitSize) {
    return (Angle_Vector){.data = NULL, .length = 0, .capacity = 0, .unitSize = unitSize};
}

void rectPut(Angle_Vector *vector, void *data, unsigned long length) {
    if (vector->data == NULL) {
        vector->capacity = length < 4 ? 4 : length;
        vector->length = length;
        vector->data = malloc(vector->capacity * vector->unitSize);
        memcpy(vector->data, data, length * sizeof(RectVert));

    } else if (vector->length + length > vector->capacity) {

        while (vector->capacity < vector->length + length) {
            vector->capacity *= 2;
        }

        void *new = realloc(vector->data, vector->capacity * vector->unitSize);
        if (!new) { // Just clear the data if it fails and write again
            free(vector->data);
            vector->length = length;
            vector->capacity = length;
            vector->data = malloc(vector->unitSize * length);
            memcpy(vector->data, data, length * vector->unitSize);
        } else {
            vector->length = vector->length + length;
        }
    } else {
        memcpy(vector->data + (vector->length * vector->unitSize), data, length * vector->unitSize);
        vector->length = vector->length + length;
    }
}

void angleVectFree(Angle_Vector vector) {
    free(vector.data);
    vector.data = NULL;
    vector.capacity = 0;
    vector.length = 0;
}

const char *rectVertShader = "attribute vec4 a_position;\n"
                             "attribute vec4 a_color;\n"
                             "attribute vec4 a_sizeCenter;\n"
                             "attribute vec4 a_radius;\n"
                             "varying vec4 v_color;\n"
                             "varying vec4 v_sizeCenter;\n"
                             "varying vec4 v_radius;\n"
                             "void main() {\n"
                             "    v_sizeCenter = a_sizeCenter;\n"
                             "    v_color = a_color;\n"
                             "    v_radius = a_radius;\n"
                             "    gl_Position = vec4(a_position.x,a_position.y,0.0,1.0);\n"
                             "}\n";

const char *imgVertShader = "attribute vec2 a_position;\n"
                            "attribute vec2 a_texPos;\n"
                            "varying vec2 v_texPos;\n"
                            "void main() {\n"
                            "    v_texPos = a_texPos;\n"
                            "    gl_Position = vec4(a_position.x,a_position.y,0.0,1.0);\n"
                            "}\n";

const char *imgFragShader =
    "precision highp float;\n"
    "varying vec2 v_texPos;\n"
    "uniform sampler2D img;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(img, v_texPos);\n"
    "}\n";

const char *rectFragShader =
    "// Adapted from https://www.shadertoy.com/view/fsdyzB by inobelar (coz I can't shader for the life of me)\n"
    "precision highp float;\n"
    "varying vec4 v_sizeCenter;\n"
    "varying vec4 v_color;\n"
    "varying vec4 v_radius;\n"
    "float roundedBoxSDF(vec2 CenterPosition, vec2 Size, vec4 Radius)\n"
    "{\n"
    "    Radius.xy = (CenterPosition.x > 0.0) ? Radius.xy : Radius.zw;\n"
    "    Radius.x  = (CenterPosition.y > 0.0) ? Radius.x  : Radius.y;\n"
    "    vec2 q = abs(CenterPosition)-Size+Radius.x;\n"
    "    return min(max(q.x,q.y),0.0) + length(max(q,0.0)) - Radius.x;\n"
    "}\n"
    "void main() {\n"
    "    vec2 halfSize = (v_sizeCenter.xy / 2.0); // Rectangle extents (half of the size)\n"
    "    float edgeSoftness   = 2.0; // How soft the edges should be (in pixels).\n"
    "    float distance = roundedBoxSDF(gl_FragCoord.xy - v_sizeCenter.zw, v_sizeCenter.zw, v_radius);\n"
    "    float smoothedAlpha = 1.0-smoothstep(0.0, edgeSoftness, distance);\n"
    "    gl_FragColor = mix(vec4(0.0), v_color, smoothedAlpha);\n"
    "    //gl_FragColor = v_color;\n"
    "}\n";

    typedef struct {
        GLuint vert;
        GLuint frag;
    } Shaders;

Shaders Clay_Angle_CreateShaderProgram(GLuint* program, GLuint* vao, const char* vertShader, const char* fragShader) {
    *program = glCreateProgram();
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);

    GLint vertSize = strlen(vertShader);
    glShaderSource(vert, 1, &vertShader, &vertSize);

    GLint fragSize = strlen(fragShader);
    glShaderSource(frag, 1, &fragShader, &fragSize);

    glAttachShader(*program, vert);
    glAttachShader(*program, frag);

    return (Shaders){vert, frag};
}

void Clay_Angle_CompileShader(GLuint program, GLuint* buffer, GLuint* vao, GLuint vert, GLuint frag) {
    glCompileShader(vert);
    glCompileShader(frag);

    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char buf[256];
        GLsizei length;
        glGetProgramInfoLog(program, 256, &length, buf);
        buf[length] = '\0';
        fprintf(stderr, "%s", buf);
    }

    glValidateProgram(program);

    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (!success) {
        char buf[256];
        GLsizei length;
        glGetProgramInfoLog(program, 256, &length, buf);
        buf[length] = '\0';
        fprintf(stderr, "%s", buf);
    }

    glUseProgram(program);

    glGenVertexArrays(1, vao);
    glGenBuffers(1, buffer);
    glBindBuffer(GL_ARRAY_BUFFER, *buffer);
    glBindVertexArray(*vao);
}

void Clay_Angle_GL_Init(int winWidth, int winHeight) {

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shaders rectShaders = Clay_Angle_CreateShaderProgram(&rectProgram, &rectVAO, rectVertShader, rectFragShader);

    glBindAttribLocation(rectProgram, 0, "a_position");
    glBindAttribLocation(rectProgram, 1, "a_color");
    glBindAttribLocation(rectProgram, 2, "a_sizeCenter");
    glBindAttribLocation(rectProgram, 3, "a_radius");

    Clay_Angle_CompileShader(rectProgram, &rectBuffer, &rectVAO, rectShaders.vert, rectShaders.frag);
    for (int i = 0; i < 4; i++) {
        glVertexAttribPointer(i, 4, GL_FLOAT, false, sizeof(RectVert), (void *)(i * sizeof(float) * 4));
        glEnableVertexAttribArray(i);
    }

    Shaders textShaders = Clay_Angle_CreateShaderProgram(&textProgram, &textVAO, imgVertShader, imgFragShader);

    glBindAttribLocation(textProgram, 0, "a_position");
    glBindAttribLocation(textProgram, 1, "a_texPos");

    Clay_Angle_CompileShader(textProgram, &textBuffer, &textVAO, textShaders.vert, textShaders.frag);

    glVertexAttribPointer(0, 2, GL_FLOAT, false, 6 * sizeof(float), (void *)(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, 6 * sizeof(float), (void *)(2));
    glEnableVertexAttribArray(1);
}

GlyphVerts Clay_Angle_DrawText(Clay_String *text, Clay_TextElementConfig *config);

void Clay_Angle_Render(Clay_RenderCommandArray renderCommands) {
    int width, height;
    GetFramebufferSize(&width, &height);
    float wWidth = (float)width;
    float wHeight = (float)height;
    Clay_SetLayoutDimensions((Clay_Dimensions){.width=wWidth, .height=wHeight});

    for (int j = 0; j < renderCommands.length; j++) {

        Clay_RenderCommand *command = Clay_RenderCommandArray_Get(&renderCommands, j);
        Clay_BoundingBox boundingBox = command->boundingBox;

        switch (command->commandType) {
        case CLAY_RENDER_COMMAND_TYPE_TEXT: {
            Clay_String text = command->text;
            GlyphVerts verts = Clay_Angle_DrawText(&command->text, command->config.textElementConfig);

            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
            Clay_RectangleElementConfig *config = command->config.rectangleElementConfig;
            RectVert rect[6];
            rect[0].position = (vec4){.x = boundingBox.x,
                                      .y = boundingBox.y,
                                      .z = boundingBox.x + boundingBox.width,
                                      .w = boundingBox.height + boundingBox.y};
            rect[1].position = (vec4){.x = boundingBox.x + boundingBox.width,
                                      .y = boundingBox.y,
                                      .z = boundingBox.x + boundingBox.width,
                                      .w = boundingBox.height + boundingBox.y};
            rect[2].position = (vec4){.x = boundingBox.x + boundingBox.width,
                                      .y = boundingBox.y + boundingBox.height,
                                      .z = boundingBox.x + boundingBox.width,
                                      .w = boundingBox.height + boundingBox.y};
            rect[3].position = (vec4){.x = boundingBox.x,
                                      .y = boundingBox.y + boundingBox.height,
                                      .z = boundingBox.x + boundingBox.width,
                                      .w = boundingBox.height + boundingBox.y};
            rect[4].position = (vec4){.x = boundingBox.x,
                                      .y = boundingBox.y,
                                      .z = boundingBox.x + boundingBox.width,
                                      .w = boundingBox.height + boundingBox.y};
            rect[5].position = (vec4){.x = boundingBox.x + boundingBox.width,
                                      .y = boundingBox.y + boundingBox.height,
                                      .z = boundingBox.x + boundingBox.width,
                                      .w = boundingBox.height + boundingBox.y};
            for (int i = 0; i < 6; i++) {
                rect[i].color = (vec4){.x = config->color.r, .y = config->color.g, .z = config->color.b, .w = config->color.a};
                rect[i].radius = (vec4){.x = (float)config->cornerRadius.topRight,
                                        .y = (float)config->cornerRadius.bottomRight,
                                        .z = (float)config->cornerRadius.topLeft,
                                        .w = (float)config->cornerRadius.bottomLeft};
                // Transform to GL coord space
                rect[i].position.x = (rect[i].position.x / wWidth) * 2.0f - 1.0f;
                rect[i].position.y = 1.0f - ((rect[i].position.y / wHeight) * 2.0f);
                rect[i].position.z = (rect[i].position.z / wWidth) * 2.0f - 1.0f;
                rect[i].position.w = 1.0f - ((rect[i].position.w / wHeight) * 2.0f);
                rect[i].sizeCenter = (vec4){.x = boundingBox.width,
                                            .y = boundingBox.height,
                                            .z = wWidth - (boundingBox.x + boundingBox.width / 2.0f
                                                          ), // Flipping these because gl_FragCoord is bottom-to-top
                                            .w = wHeight - (boundingBox.y + boundingBox.height / 2.0f)};
            }
            glBindBuffer(GL_ARRAY_BUFFER, rectBuffer);
            glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(RectVert), rect, GL_DYNAMIC_DRAW);
            glUseProgram(rectProgram);
            glBindVertexArray(rectVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_BORDER: {
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_NONE: {
            break;
        }
        }
    }
}
