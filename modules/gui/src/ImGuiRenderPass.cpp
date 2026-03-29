#include "ImGuiRenderPass.hpp"
#include "RenderPass.hpp"

#define VOLK_IMPLEMENTATION
#include <volk.h>

#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_sdl2.h"

namespace tw::drw::gui {

static void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
    fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
    return VK_FALSE;
}

ImGui_ImplVulkan_InitInfo create_imgui_init_info(const Gpu* gpu, VkRenderPass rp) {
    ImGui_ImplVulkan_InitInfo init_info = {
        .Instance = gpu->instance()->instance(),
        .PhysicalDevice = gpu->gpu(),
        .Device = gpu->dev(),
        .QueueFamily = 0,
        .Queue = gpu->graphics_queue(),
        .DescriptorPool = gpu->descriptor_pool(),
        .MinImageCount = 2,
        .ImageCount = 2,
        .PipelineCache = nullptr,
        .PipelineInfoMain = {
            .RenderPass = rp,
            .Subpass = 0,
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        },
        .Allocator = nullptr,
        .CheckVkResultFn = check_vk_result,
    };

    return init_info;
}

lft::rg::RenderTaskBuilder ImGuiRenderPass::create_render_task() {
    VkInstance ins = m_gpu->instance()->instance();
    // ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_0,
    //         [](const char *function_name, void *vulkan_instance) {
    //             return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance *>(vulkan_instance)),
    //                     function_name);
    //         }, &ins);

    return lft::rg::render_task<ImGuiRenderPass>(
        "imgui", this,
        [](const lft::rg::TaskBuildInfo& info,
            ImGuiRenderPass* context) {

            if(context->is_initialized) {
                return;
            }

            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

            const std::string font_path = "/home/martin/projects/mmo/external/imgui/misc/fonts/ProggyClean.ttf";
            io.Fonts->AddFontFromFileTTF(font_path.c_str(), 13.0f);
            auto init_info = create_imgui_init_info(info.gpu(), info.renderpass());
            ImGui_ImplSDL2_InitForVulkan(context->m_window->get_handle());
            ImGui_ImplVulkan_Init(&init_info);

            context->is_initialized = true;
        }, [&](const lft::rg::TaskRecordInfo& info, ImGuiRenderPass* context) {
            ImGui::Render();
            ImDrawData* draw_data = ImGui::GetDrawData();
            ImGui_ImplVulkan_RenderDrawData(draw_data, info.recording().cmdbuf());
        })
        .set_output_to_final()
            .add_dependency("character")
            .add_dependency("terrain");
}

}
