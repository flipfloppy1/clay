
#ifndef CLAY_ANGLE_OPENGL_HEADER
#include <angle_gl.h>
#else
#include CLAY_ANGLE_OPENGL_HEADER
#endif

#include "../../clay.h"

void Clay_Angle_DrawText(Clay_String *text, Clay_TextElementConfig *config);

void Clay_Angle_Render(Clay_RenderCommandArray renderCommands) {

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
