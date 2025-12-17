#include "app/gui.h"
#include "app/main.h"

#include <imgui.h>

int main(int argc, char *argv[]) {
    __APP_MAIN_INIT__;

    axby::gui_init("Gui Example");
    while (!axby::gui_wants_quit()) {
        axby::gui_loop_begin();

        ImGui::ShowDemoWindow();

        axby::gui_loop_end();
    }
    axby::gui_cleanup();

    return 0;
}
