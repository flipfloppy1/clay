#include "../../renderers/angle/clay_angle_extras.c"
#include "../../renderers/angle/clay_renderer_angle.c"
#ifndef CLAY_ANGLE_OPENGL_HEADER
#include <angle_gl.h>
#else
#include CLAY_ANGLE_OPENGL_HEADER
#endif
#include "GLFW/glfw3.h"

const Clay_Color COLOR_LIGHT = (Clay_Color) {200, 200, 200, 255};
const Clay_Color COLOR_RED = (Clay_Color) {168, 66, 28, 255};
const Clay_Color COLOR_ORANGE = (Clay_Color) {225, 138, 50, 255};

// Layout config is just a struct that can be declared statically, or inline
Clay_LayoutConfig sidebarItemLayout = (Clay_LayoutConfig) {
    .sizing = { .width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_FIXED(50) },
};

Clay_LayoutConfig otherItemLayout = (Clay_LayoutConfig) {
    .sizing = { .width = CLAY_SIZING_GROW() }, .padding = {16, 16}, .childGap = 16, .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
};

void SidebarItemComponent() {
    CLAY(CLAY_LAYOUT(sidebarItemLayout), CLAY_RECTANGLE({ .color = COLOR_ORANGE })) {}
}

int main() {
    Clay_Angle_Initialize(0, 0, "Clay Angle Renderer Test");

    uint64_t capacity = Clay_MinMemorySize();
    int screenWidth, screenHeight;
    glfwGetWindowSize(gWindow, &screenWidth, &screenHeight);
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(capacity, malloc(capacity));
    Clay_Initialize(arena, (Clay_Dimensions){screenWidth, screenHeight}, (Clay_ErrorHandler){0});
    Clay_SetMeasureTextFunction(Clay_HB_MeasureText);


    //Clay_TextElementConfig textConfig = (Clay_TextElementConfig){.textColor = {0,0,0,1},.fontId = Clay_Angle_LoadFont(&CLAY_STRING("IosevkaAile-Light.ttc"),18),.fontSize = 18};

    glClearColor(0.2f,0.2f,0.5f,1.0f);

    while (!glfwWindowShouldClose(gWindow)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Clay_BeginLayout();

        // CLAY(CLAY_LAYOUT({.childAlignment={.x=CLAY_ALIGN_X_CENTER,.y=CLAY_ALIGN_Y_CENTER},.sizing = {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()}})) {
        //     CLAY(CLAY_LAYOUT({.sizing = {.width = CLAY_SIZING_FIXED(300), .height = CLAY_SIZING_FIXED(300) }}),CLAY_RECTANGLE({.color = {0.1, 0.4, 0.65, 1}})) {
        //         CLAY(CLAY_TEXT(CLAY_STRING("This is some text"),&textConfig)) {}
        //     }
        // }

        // An example of laying out a UI with a fixed width sidebar and flexible width main content
        CLAY(CLAY_ID("OuterContainer"), CLAY_LAYOUT({ .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}, .padding = {16, 16}, .childGap = 16 }), CLAY_RECTANGLE({ .color = {250,250,255,255} })) {
            CLAY(CLAY_ID("SideBar"),
                CLAY_LAYOUT({ .layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = { .width = CLAY_SIZING_FIXED(300), .height = CLAY_SIZING_GROW() }, .padding = {16, 16}, .childGap = 16 }),
                CLAY_RECTANGLE({ .color = COLOR_LIGHT })
            ) {
                CLAY(CLAY_ID("ProfilePictureOuter"), CLAY_LAYOUT(otherItemLayout), CLAY_RECTANGLE({ .color = COLOR_RED })) {
                    CLAY(CLAY_ID("ProfilePicture"), CLAY_LAYOUT({ .sizing = { .width = CLAY_SIZING_FIXED(60), .height = CLAY_SIZING_FIXED(60) }})) {}
                    //CLAY_TEXT(CLAY_STRING("Clay - UI Library"), CLAY_TEXT_CONFIG({ .fontSize = 24, .textColor = {255, 255, 255, 255} }));
                }

                // Standard C code like loops etc work inside components
                for (int i = 0; i < 5; i++) {
                    SidebarItemComponent();
                }
            }

            CLAY(CLAY_ID("MainContent"), CLAY_LAYOUT({ .sizing = { .width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW() }}), CLAY_RECTANGLE({ .color = COLOR_LIGHT })) {}
        }


        Clay_RenderCommandArray commands = Clay_EndLayout();
        Clay_Angle_Render(commands);


        glfwSwapBuffers(gWindow);
    }

    glfwTerminate();
    return 0;
}
