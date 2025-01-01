
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
    vec4 bounds, color;
    float radius;
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
                             "attribute float a_radius;\n"
                             "varying vec4 v_color;\n"
                             "varying vec4 v_bounds;\n"
                             "varying float v_radius;\n"
                             "void main() {\n"
                             "    v_bounds = a_position;\n"
                             "    v_color = a_color;\n"
                             "    v_radius = a_radius;\n"
                             "    gl_Position = vec4(a_position.xy,0.0f,1.0f);\n"
                             "}\n";

const char *rectFragShader = "precision highp float;\n"
                             "varying vec4 v_bounds;\n"
                             "varying vec4 v_color;\n"
                             "varying float v_radius;\n"
                             "uniform vec2 u_resolution;\n"
                             "void main() {\n"
                             "    float x = v_bounds.x;\n"
                             "    float y = v_bounds.y;\n"
                             "    float width = v_bounds.z;\n"
                             "    float height = v_bounds.w;\n"
                             "    float distX = abs(gl_FragCoord.x - (x + width * 0.5));\n"
                             "    float distY = abs(gl_FragCoord.y - (y + height * 0.5));\n"
                             "    if (distX > (width * 0.5 + v_radius) || distY > (height * 0.5 + v_radius)) {\n"
                             "        //discard;\n"
                             "    }\n"
                             "    if (distX > (width * 0.5) && distY > (height * 0.5)) {\n"
                             "        float cornerDist = length(vec2(distX - width * 0.5, distY - height * 0.5));\n"
                             "        if (cornerDist > v_radius) {\n"
                             "            //discard;\n"
                             "        }\n"
                             "    }\n"
                             "    gl_FragColor = v_color;\n"
                             "}\n";

void Clay_Angle_GL_Init(int winWidth, int winHeight) {

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
    glBindAttribLocation(rectProgram, 2, "a_radius");

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

    glUniform2f(glGetUniformLocation(rectProgram, "u_resolution"), (float)winWidth, (float)winHeight);

    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, rectBuffer);
    glBindVertexArray(rectVAO);
    glVertexAttribPointer(0, 4, GL_FLOAT, false, sizeof(RectVert), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, false, sizeof(RectVert), (void *)(sizeof(float) * 4));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, false, sizeof(RectVert), (void *)(sizeof(float) * 5));
    glEnableVertexAttribArray(2);
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
            //printf("Bounds are: %f,%f,%f,%f\n", boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height);
            rect[0].bounds = (vec4
            ){.x = boundingBox.x,
              .y = boundingBox.y,
              .z = boundingBox.x + boundingBox.width,
              .w = boundingBox.height + boundingBox.y};
            rect[1].bounds = (vec4
            ){.x = boundingBox.x + boundingBox.width,
              .y = boundingBox.y,
              .z = boundingBox.x + boundingBox.width,
              .w = boundingBox.height + boundingBox.y};
            rect[2].bounds = (vec4
            ){.x = boundingBox.x + boundingBox.width,
              .y = boundingBox.y + boundingBox.height,
              .z = boundingBox.x + boundingBox.width,
              .w = boundingBox.height + boundingBox.y};
            rect[3].bounds = (vec4
            ){.x = boundingBox.x,
              .y = boundingBox.y + boundingBox.height,
              .z = boundingBox.x + boundingBox.width,
              .w = boundingBox.height + boundingBox.y};
            rect[4].bounds = (vec4
            ){.x = boundingBox.x,
              .y = boundingBox.y,
              .z = boundingBox.x + boundingBox.width,
              .w = boundingBox.height + boundingBox.y};
            rect[5].bounds = (vec4
            ){.x = boundingBox.x + boundingBox.width,
              .y = boundingBox.y + boundingBox.height,
              .z = boundingBox.x + boundingBox.width,
              .w = boundingBox.height + boundingBox.y};
            for (int i = 0; i < 6; i++) {
                rect[i].color = (vec4){.x = config->color.r, .y = config->color.g, .z = config->color.b, .w = config->color.a};
                rect[i].radius = config->cornerRadius.topLeft;
                // Transform to GL coord space
                rect[i].bounds.x = (rect[i].bounds.x / wWidth) * 2.0f - 1.0f;
                rect[i].bounds.y = 1.0f - ((rect[i].bounds.y / wHeight) * 2.0f);
                rect[i].bounds.z = (rect[i].bounds.z / wWidth) * 2.0f - 1.0f;
                rect[i].bounds.w = 1.0f - ((rect[i].bounds.w / wHeight) * 2.0f);
                //printf("Got a vertex: %f,%f\n", rect[i].bounds.x, rect[i].bounds.y);
            }
            printf("color: %f, %f, %f, %f\n",rect[0].color.x,rect[0].color.y,rect[0].color.z,rect[0].color.w);
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
