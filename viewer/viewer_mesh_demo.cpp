#include <imgui.h>

#include <Eigen/Dense>
#include <limits>
#include <random>
#include <vector>

#include "app/files.h"
#include "app/gui.h"
#include "app/main.h"
#include "colors/colors.h"
#include "debug/log.h"
#include "debug/to_sstring.h"
#include "seq/seq.h"
#include "viewer/mesh.h"
#include "viewer/viewer.h"
#include "wrappers/assimp.h"
#include "wrappers/eigen.h"

int main(int argc, char *argv[]) {
    __APP_MAIN_INIT__;

    // load mesh
    std::string sphere_ply_path =
        axby::get_rlocation("_main/viewer/sphere.ply");
    LOG(INFO) << "loading model " << sphere_ply_path;

    std::vector<axby::Mesh> sphere_model =
        load_assimp_model_from_file(sphere_ply_path);
    LOG(INFO) << "The model had " << sphere_model.size() << " meshes";
    for (const auto& mesh : sphere_model) {
        LOG(INFO) << "\tMesh with " << mesh.xyzs.size() / 3 << " vertices";
        LOG(INFO) << "\tMesh with " << mesh.rgbs.size() / 3 << " colors";
    }

    axby::gui_init("Viewer Mesh Demo", 1024, 1024);
    axby::viewer::init();
    axby::viewer::enable_auto_orbit();
    axby::viewer::add_model("sphere", sphere_model);

    Eigen::Matrix4f tx_world_sphere = Id4f();
    tx_world_sphere(2, 3) = 2.0f;
    axby::viewer::update_tx_world_object("sphere", tx_world_sphere);

    while (!axby::gui_wants_quit()) {
        axby::gui_loop_begin();
        axby::viewer::new_frame(ImGui::GetIO());

        static axby::MeshMaterial material;
        if (ImGui::Begin("Options")) {
            ImGui::SliderFloat3("diffuse", (float*)&material.diffuse, 0, 1);
            ImGui::SliderFloat3("specular", (float*)&material.specular, 0, 1);
            ImGui::SliderFloat("specular_exponent", &material.specular_exponent,
                               0, 30);
            ImGui::SliderFloat3("ambient", (float*)&material.ambient, 0, 1);            
            ImGui::SliderFloat3("emissive", (float*)&material.emissive, 0, 1);
            ImGui::End();
        }

        axby::viewer::set_model_material("sphere", material);

        axby::viewer::draw_model("sphere");
        axby::viewer::draw_coordinate_frame("sphere");

        axby::gui_loop_end();
    }
    axby::gui_cleanup();

    return 0;
}
