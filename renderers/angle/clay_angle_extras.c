#ifndef CLAY_ANGLE_OPENGL_HEADER
#include <angle_gl.h>
#else
#include CLAY_ANGLE_OPENGL_HEADER
#endif
#define CLAY_IMPLEMENTATION
#include "../../clay.h"
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include <hb.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include FT_FREETYPE_H

typedef struct {
    float x, y, z, w;
} vec4;

typedef struct {
    vec4 glPosTexPos;
    vec4 color;
} GlyphVert;

typedef struct {
    int length;
    GLuint textureId;
    GlyphVert *glyphVerts;
} GlyphVerts;

typedef struct {
    int xAdvance, yOff;
    float xTex, yTex, texWidth, texHeight;
} Glyph;

typedef struct {
    hb_font_t *hbFont;
    FT_Face ftFace;
    int size;
    Glyph glyphs[128];
    int texWidth, texHeight;
    GLuint textureId;
    void *texture;
} Angle_Font;

GLFWwindow *gWindow;
FT_Library gFTLib;

GLuint textBuffer;
GLuint textVAO;

int gWidth, gHeight;

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
    gWidth = *width;
    gHeight = *height;
}

void Clay_Angle_Initialize(int width, int height, const char *title) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to init glfw\n");
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    if (!(width && height)) {
        const GLFWvidmode *vidMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        width = vidMode->width;
        height = vidMode->height;
    }

    gWindow = glfwCreateWindow(width, height, title, NULL, NULL);

    if (!gWindow) {
        fprintf(stderr, "Failed to init glfw window\n");
        glfwTerminate();
        exit(1);
    }

    glfwMakeContextCurrent(gWindow);

    glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    glViewport(0, 0, width, height);
    glDebugMessageCallback(DebugMsg, NULL);

    if (FT_Init_FreeType(&gFTLib)) {
        fprintf(stderr, "Failed to init freetype\n");
        glfwTerminate();
        exit(1);
    }

    glfwGetFramebufferSize(gWindow, &width, &height);
    gWidth = width;
    gHeight = height;
    Clay_Angle_GL_Init(width, height);
}

int Clay_Angle_LoadFont(Clay_String *path, int size) {

    if (gFontNum >= MAX_FONTS) {
        fprintf(stderr, "Font limit reached\n");
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
        if (error == FT_Err_Cannot_Open_Resource) {
            fprintf(stderr, "Unrecognised format for font:'%s'\n", strArr);
        } else {
            fprintf(stderr, "Error loading font:'%s'\n", strArr);
        }
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
            fprintf(stderr, "Failed to load glyph %c in %s\n", c, strArr);
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
        font.glyphs[c].xAdvance = 0;

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

static GlyphVerts Clay_Angle_DrawText(Clay_String *text, Clay_TextElementConfig *config) {
    GlyphVerts verts;
    int printableCount;
    for (int i = 0; i < text->length; i++) {
        if (text->chars[i] < 32 || text->chars[i] > 126)
            continue;

        printableCount++;
    }
    verts.glyphVerts = malloc(sizeof(GlyphVert) * 6 * printableCount);
    int index = 0;
    float xAdvance = -1.0f;
    for (int i = 0; i < text->length; i++) {
        if (text->chars[i] < 32 || text->chars[i] > 126)
            continue;

        Angle_Font font = gAngleFonts[config->fontId];
        Glyph c = font.glyphs[text->chars[i]];
        float normAdvance = (float)c.xAdvance / (float)gWidth;
        vec4 textColor = (vec4){.x = config->textColor.r, .y = config->textColor.g, .z = config->textColor.b, .w = config->textColor.a};
        float xOff = (float)c.xAdvance / (float)gWidth;
        verts.glyphVerts[index * 6 + 0] = (GlyphVert){.color = textColor,.glPosTexPos.x = xAdvance + xOff, .glPosTexPos.y=c.yOff,.glPosTexPos.z=c.xTex,.glPosTexPos.w=c.yTex};
        verts.glyphVerts[index * 6 + 1] = (GlyphVert){.color = textColor,.glPosTexPos.x = xAdvance + xOff, .glPosTexPos.y = c.yOff};
        verts.glyphVerts[index * 6 + 2];
        verts.glyphVerts[index * 6 + 3];
        verts.glyphVerts[index * 6 + 4];
        verts.glyphVerts[index * 6 + 5];
        verts.length += 6;

        index++;
        xAdvance += xOff;
    }

    return verts;
}

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
