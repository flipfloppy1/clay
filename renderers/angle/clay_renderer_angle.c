
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef CLAY_ANGLE_OPENGL_HEADER
#include <angle_gl.h>
#else
#include CLAY_ANGLE_OPENGL_HEADER
#endif

#include "../../clay.h"

GLuint rectProgram;
GLuint rectBuffer;
GLuint rectVAO;

typedef struct {
    float x, y, z, w;
} vec4;

typedef struct {
    vec4 position, color, sizeCenter, radius;
} RectVert;

typedef struct {
    RectVert *data;
    unsigned long length, capacity;
} RectVector;

void GetFramebufferSize(int *width, int *height);

RectVector makeRectVector() { return (RectVector){.data = NULL, .length = 0, .capacity = 0}; }

void rectPut(RectVector *vector, RectVert *verts, unsigned long length) {
    if (vector->data == NULL) {
        vector->capacity = length < 4 ? 4 : length;
        vector->length = length;
        vector->data = malloc(vector->capacity * sizeof(RectVert));
        memcpy(vector->data, verts, length * sizeof(RectVert));

    } else if (vector->length + length > vector->capacity) {

        while (vector->capacity < vector->length + length) {
            vector->capacity *= 2;
        }

        void *new = realloc(vector->data, vector->capacity * sizeof(RectVert));
        if (!new) { // Just clear the data if it fails and write again
            free(vector->data);
            vector->length = length;
            vector->capacity = length;
            vector->data = malloc(sizeof(RectVert) * length);
            memcpy(vector->data, verts, length * sizeof(RectVert));
        } else {
            vector->length = vector->length + length;
        }
    } else {
        memcpy(vector->data + (vector->length * sizeof(RectVector)), verts, length * sizeof(RectVert));
        vector->length = vector->length + length;
    }
}

void rectFree(RectVector vector) {
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
                             "    v_sizeCenter = vec4(a_sizeCenter.xy,a_sizeCenter.zw);\n"
                             "    v_color = a_color;\n"
                             "    v_radius = a_radius;\n"
                             "    gl_Position = vec4(a_position.x,a_position.y,0.0,1.0);\n"
                             "}\n";

const char *rectFragShader = "// Adapted from https://www.shadertoy.com/view/fsdyzB by inobelar (coz I can't shader for the life of me)\n"
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
                             "    vec2 fragCoord = vec2(gl_FragCoord.x,1440.0-gl_FragCoord.y);\n"
                             "    vec2 halfSize = (v_sizeCenter.xy / 2.0); // Rectangle extents (half of the size)\n"
                             "    float edgeSoftness   = 2.0; // How soft the edges should be (in pixels).\n"
                             "    float distance = roundedBoxSDF(gl_FragCoord.xy - v_sizeCenter.zw, halfSize, v_radius);\n"
                             "    float smoothedAlpha = 1.0-smoothstep(0.0, edgeSoftness, distance);\n"
                             "    gl_FragColor = vec4(v_color.xyz,v_color.a * smoothedAlpha);\n"
                             "}\n";

void Clay_Angle_GL_Init(int winWidth, int winHeight) {

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    rectProgram = glCreateProgram();
    GLuint rectVert = glCreateShader(GL_VERTEX_SHADER);
    GLuint rectFrag = glCreateShader(GL_FRAGMENT_SHADER);

    GLint rectVertSize = strlen(rectVertShader);
    glShaderSource(rectVert, 1, &rectVertShader, &rectVertSize);

    GLint rectFragSize = strlen(rectFragShader);
    glShaderSource(rectFrag, 1, &rectFragShader, &rectFragSize);

    glAttachShader(rectProgram, rectVert);
    glAttachShader(rectProgram, rectFrag);

    glBindAttribLocation(rectProgram, 0, "a_position");
    glBindAttribLocation(rectProgram, 1, "a_color");
    glBindAttribLocation(rectProgram, 2, "a_sizeCenter");
    glBindAttribLocation(rectProgram, 3, "a_radius");

    glCompileShader(rectVert);
    glCompileShader(rectFrag);

    glLinkProgram(rectProgram);

    glValidateProgram(rectProgram);

    GLint success;
    glGetProgramiv(rectProgram, GL_VALIDATE_STATUS, &success);
    if (!success) {
        char buf[256];
        GLsizei length;
        glGetProgramInfoLog(rectProgram, 256, &length, buf);
        buf[length] = '\0';
        fprintf(stderr, "%s", buf);
    }

    glUseProgram(rectProgram);

    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, rectBuffer);
    glBindVertexArray(rectVAO);
    for (int i = 0; i < 4; i++) {
        glVertexAttribPointer(i, 4, GL_FLOAT, false, sizeof(RectVert), (void *)(i * sizeof(float) * 4));
        glEnableVertexAttribArray(i);
    }
}

void Clay_Angle_DrawText(Clay_String *text, Clay_TextElementConfig *config);

void Clay_Angle_Render(Clay_RenderCommandArray renderCommands) {
    int width, height;
    GetFramebufferSize(&width, &height);
    float wWidth = (float)width;
    float wHeight = (float)height;
    RectVector rectVector = makeRectVector();

    for (int j = 0; j < renderCommands.length; j++) {

        Clay_RenderCommand *command = Clay_RenderCommandArray_Get(&renderCommands, j);
        Clay_BoundingBox boundingBox = command->boundingBox;

        switch (command->commandType) {
        case CLAY_RENDER_COMMAND_TYPE_TEXT: {
            Clay_String text = command->text;
            Clay_Angle_DrawText(&command->text, command->config.textElementConfig);
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
            // printf("Bounds are: %f,%f,%f,%f\n", boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height);
            rect[0].position = (vec4
            ){.x = boundingBox.x,
              .y = boundingBox.y,
              .z = boundingBox.x + boundingBox.width,
              .w = boundingBox.height + boundingBox.y};
            rect[1].position = (vec4
            ){.x = boundingBox.x + boundingBox.width,
              .y = boundingBox.y,
              .z = boundingBox.x + boundingBox.width,
              .w = boundingBox.height + boundingBox.y};
            rect[2].position = (vec4
            ){.x = boundingBox.x + boundingBox.width,
              .y = boundingBox.y + boundingBox.height,
              .z = boundingBox.x + boundingBox.width,
              .w = boundingBox.height + boundingBox.y};
            rect[3].position = (vec4
            ){.x = boundingBox.x,
              .y = boundingBox.y + boundingBox.height,
              .z = boundingBox.x + boundingBox.width,
              .w = boundingBox.height + boundingBox.y};
            rect[4].position = (vec4
            ){.x = boundingBox.x,
              .y = boundingBox.y,
              .z = boundingBox.x + boundingBox.width,
              .w = boundingBox.height + boundingBox.y};
            rect[5].position = (vec4
            ){.x = boundingBox.x + boundingBox.width,
              .y = boundingBox.y + boundingBox.height,
              .z = boundingBox.x + boundingBox.width,
              .w = boundingBox.height + boundingBox.y};
            for (int i = 0; i < 6; i++) {
                rect[i].color = (vec4){.x = config->color.r, .y = config->color.g, .z = config->color.b, .w = config->color.a};
                rect[i].radius = (vec4){ .x = (float)config->cornerRadius.topRight, .y = (float)config->cornerRadius.bottomRight, .z = (float)config->cornerRadius.topLeft, .w = (float)config->cornerRadius.bottomLeft };
                // Transform to GL coord space
                rect[i].position.x = (rect[i].position.x / wWidth) * 2.0f - 1.0f;
                rect[i].position.y = 1.0f - ((rect[i].position.y / wHeight) * 2.0f);
                rect[i].position.z = (rect[i].position.z / wWidth) * 2.0f - 1.0f;
                rect[i].position.w = 1.0f - ((rect[i].position.w / wHeight) * 2.0f);
                rect[i].sizeCenter = (vec4){ .x = boundingBox.width, .y = boundingBox.height, .z = boundingBox.x + boundingBox.width / 2.0f, .w = boundingBox.y + boundingBox.height / 2.0f };
                // printf("Got a vertex: %f,%f\n", rect[i].bounds.x, rect[i].bounds.y);
            }
            rectPut(&rectVector, rect, 6);
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

    glBindBuffer(GL_ARRAY_BUFFER, rectBuffer);
    glBufferData(GL_ARRAY_BUFFER, rectVector.length * sizeof(RectVert), rectVector.data, GL_DYNAMIC_DRAW);

    glUseProgram(rectProgram);
    glBindVertexArray(rectVAO);
    glDrawArrays(GL_TRIANGLES, 0, rectVector.length);

    rectFree(rectVector);
}
