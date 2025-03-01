#include "../../renderers/angle/clay_angle_extras.c"
#include "../../renderers/angle/clay_renderer_angle.c"
#ifndef CLAY_ANGLE_OPENGL_HEADER
#include <angle_gl.h>
#else
#include CLAY_ANGLE_OPENGL_HEADER
#endif
#include "GLFW/glfw3.h"

int main() {
    Clay_Angle_Initialize(0, 0, "Clay Angle Renderer Test");

    uint64_t capacity = Clay_MinMemorySize();
    int screenWidth, screenHeight;
    glfwGetWindowSize(gWindow, &screenWidth, &screenHeight);
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(capacity, malloc(capacity));
    Clay_Initialize(arena, (Clay_Dimensions){screenWidth, screenHeight}, (Clay_ErrorHandler){0});
    Clay_SetMeasureTextFunction(Clay_HB_MeasureText);


    Clay_TextElementConfig textConfig = (Clay_TextElementConfig){.textColor = {0,0,0,1},.fontId = Clay_Angle_LoadFont(&CLAY_STRING("IosevkaAile-Light.ttc"),18),.fontSize = 18};

    glClearColor(0.2f,0.2f,0.5f,1.0f);

    while (!glfwWindowShouldClose(gWindow)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Clay_BeginLayout();

        CLAY(CLAY_LAYOUT({.childAlignment={.x=CLAY_ALIGN_X_CENTER,.y=CLAY_ALIGN_Y_TOP},.sizing = {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()}})) {
            CLAY(CLAY_LAYOUT({.sizing = {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_FIXED(300) }}),CLAY_RECTANGLE({.color = {0.1, 0.4, 0.65, 1}})) {
                CLAY(CLAY_TEXT(CLAY_STRING("This is some text"),&textConfig)) {}
            }
        }

        Clay_RenderCommandArray commands = Clay_EndLayout();
        Clay_Angle_Render(commands);


        glfwSwapBuffers(gWindow);
    }

    glfwTerminate();
    return 0;
}
