#define CLAY_IMPLEMENTATION
#include "../../clay.h"
#include <GLFW/glfw3.h>
#ifndef CLAY_ANGLE_OPENGL_HEADER
#include <angle_gl.h>
#else
#include CLAY_ANGLE_OPENGL_HEADER
#endif
#include <ft2build.h>
#include <hb.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include FT_FREETYPE_H

typedef struct {
    int xAdvance;
    float xTex, yTex, texWidth, texHeight;
} Glyph;

typedef struct {
    hb_font_t *hbFont;
    FT_Face ftFace;
    int size;
    Glyph glyphs[128];
    int texWidth, texHeight;
    void *texture;
} Angle_Font;

GLFWwindow *gWindow;
FT_Library gFTLib;

#define MAX_FONTS 16

int gFontNum = 0;
Angle_Font gAngleFonts[MAX_FONTS];

void Clay_Angle_GL_Init(int width, int height);

void DebugMsg(
    GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam
) {
    fprintf(stderr, "%s\n", message);
}

void GetFramebufferSize(int *width, int *height) {
    glfwGetFramebufferSize(gWindow, width, height);
}

void Clay_Angle_Initialize(int width, int height, const char *title) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to init glfw");
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    gWindow = glfwCreateWindow(width, height, title, NULL, NULL);

    glfwMakeContextCurrent(gWindow);

    glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gWindow) {
        fprintf(stderr, "Failed to init glfw window");
        glfwTerminate();
        exit(1);
    }

    glViewport(0, 0, width, height);
    glDebugMessageCallback(DebugMsg, NULL);

        if (FT_Init_FreeType(&gFTLib)) {
        fprintf(stderr, "Failed to init freetype");
        glfwTerminate();
        exit(1);
    }

    glfwGetFramebufferSize(gWindow, &width, &height);
    Clay_Angle_GL_Init(width, height);
}

int Clay_Angle_LoadFont(Clay_String *path, int size) {

    if (gFontNum >= MAX_FONTS) {
        fprintf(stderr, "Font limit reached");
        return -1;
    }

    char *strArr = malloc(sizeof(*path->chars) * (path->length + 1));
    memcpy(strArr, (void *)path->chars, sizeof(*path->chars) * path->length);
    strArr[path->length] = '\0';

    Angle_Font font;

    hb_blob_t *blob = hb_blob_create_from_file(strArr);
    hb_face_t *face = hb_face_create(blob, 0);
    font.hbFont = hb_font_create(face);
    hb_font_set_ptem(font.hbFont, (float)size);

    FT_Error error = FT_New_Face(gFTLib, strArr, 0, &font.ftFace);

    if (error) {
        fprintf(stderr, "Error loading font:'%s'", strArr);
        hb_blob_destroy(blob);
        hb_face_destroy(face);
        hb_font_destroy(font.hbFont);
        free(strArr);
        return -1;
    }

    font.texWidth = font.ftFace->max_advance_width * font.ftFace->size->metrics.x_scale * 12.0f / 64.0f;
    font.texHeight = font.ftFace->max_advance_height * font.ftFace->size->metrics.y_scale * 12.0f / 64.0f;
    font.texture = malloc(font.texWidth * font.texHeight);

    FT_Set_Char_Size(font.ftFace, 0, size * 64, 0, 0);
    int xPos = 0;
    int yPos = 0;
    int yMaxAdvance = 0;

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(font.ftFace, c, FT_LOAD_RENDER)) {
            fprintf(stderr, "Failed to load glyph %c in %s", c, strArr);
            continue;
        }
        FT_Bitmap bitmap = font.ftFace->glyph->bitmap;

        // Move down a row if end of row reached
        if (bitmap.width + xPos > font.texWidth) {
            yPos += yMaxAdvance;
            xPos = 0;
            yMaxAdvance = 0;
        }

        font.glyphs[c].xTex = (float)xPos / (float)font.texWidth;
        font.glyphs[c].yTex = 1.0f - (float)yPos / (float)font.texHeight;
        font.glyphs[c].texWidth = (float)bitmap.width / (float)font.texWidth;
        font.glyphs[c].texHeight = (float)bitmap.rows / (float)font.texHeight;

        for (int y = 0; y < bitmap.rows; y++) {
            memcpy(font.texture + xPos + (yPos + y) * font.texWidth, bitmap.buffer + y * bitmap.width, bitmap.width);
        }

        xPos += bitmap.width;
        yMaxAdvance = yMaxAdvance > bitmap.rows ? yMaxAdvance : bitmap.rows;
    }

    gAngleFonts[gFontNum] = font;
    gFontNum++;
    free(strArr);

    return gFontNum - 1;
}

void Clay_Angle_DrawText(Clay_String *text, Clay_TextElementConfig *config) {}

static Clay_Dimensions Clay_HB_MeasureText(Clay_String *text, Clay_TextElementConfig *config) {
    Clay_Dimensions textSize = {0};
    Angle_Font font = gAngleFonts[config->fontId];

    hb_buffer_t *buf;
    buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, text->chars, text->length, 0, -1);

    hb_buffer_guess_segment_properties(buf);

    hb_font_set_ptem(font.hbFont, (float)config->fontSize);
    hb_shape(font.hbFont, buf, NULL, 0);

    unsigned int glyphCount;
    hb_glyph_position_t *glyphPos = hb_buffer_get_glyph_positions(buf, &glyphCount);

    float maxTextWidth = 0.0f;
    float lineTextWidth = 0.0f;
    int glyphIndex = 0;

    for (int i = 0; i < text->length; i++) {
        if (text->chars[i] == '\n') {
            maxTextWidth = fmax(maxTextWidth, lineTextWidth);
            lineTextWidth = 0.0f;
            continue; // Don't have to increment glyphIndex since printable
                      // characters aren't part of the set
        }
        int index = text->chars[i] - 32;
        if (glyphCount <= i) {
            break;
        }
        lineTextWidth += glyphPos[glyphIndex].x_advance;
        glyphIndex++;
    }

    maxTextWidth = fmax(maxTextWidth, lineTextWidth);

    textSize.width = maxTextWidth;
    textSize.height = config->fontSize;

    return textSize;
}
