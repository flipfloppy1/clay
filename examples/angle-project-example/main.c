#include "../../renderers/angle/clay_angle_extras.c"
#include "../../renderers/angle/clay_renderer_angle.c"
#include "GLFW/glfw3.h"

int main() {
    int screenWidth = 1920;
    int screenHeight = 1080;
    Clay_Angle_Initialize(screenWidth, screenHeight, "Clay Angle Renderer Test");

    uint64_t capacity = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(capacity, malloc(capacity));
    Clay_Initialize(arena, (Clay_Dimensions){screenWidth, screenHeight}, (Clay_ErrorHandler){0});
    Clay_SetMeasureTextFunction(Clay_HB_MeasureText);

    glClearColor(0.5f,0.0f,0.5f,1.0f);

    while (!glfwWindowShouldClose(gWindow)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Clay_BeginLayout();

        CLAY(CLAY_LAYOUT({.sizing = {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_FIXED(100)}}),CLAY_RECTANGLE({.color = {0, 0, 0}})) {}

        Clay_RenderCommandArray commands = Clay_EndLayout();
        Clay_Angle_Render(commands);


        glfwSwapBuffers(gWindow);
    }
    return 0;
}
