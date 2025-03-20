#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <vector>
#include <set>

#include <3ds.h>
#include <citro3d.h>
#include <imgui.h>
#include <imgui_ctru.h>
#include <imgui_citro3d.h>


constexpr auto CLEAR_COLOR = 0x204B7AFF;
constexpr auto SCREEN_WIDTH = 400.0f;
constexpr auto SCREEN_HEIGHT = 480.0f;
constexpr auto FB_SCALE = 2.0f;
constexpr auto FB_WIDTH = SCREEN_WIDTH * FB_SCALE;
constexpr auto FB_HEIGHT = SCREEN_HEIGHT * FB_SCALE;
constexpr auto DISPLAY_TRANSFER_FLAGS =
    GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_XY);

C3D_RenderTarget *s_top = nullptr;
C3D_RenderTarget *s_bottom = nullptr;

int main(int argc, char *argv[]) {
	osSetSpeedupEnable(true);
	romfsInit();

	gfxInitDefault();

#ifndef NDEBUG
	consoleDebugInit(debugDevice_SVC);
	std::setvbuf(stderr, nullptr, _IOLBF, 0);
#endif

	C3D_Init(2 * C3D_DEFAULT_CMDBUF_SIZE);

	s_top = C3D_RenderTargetCreate(FB_HEIGHT * 0.5f, FB_WIDTH, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(s_top, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
	s_bottom = C3D_RenderTargetCreate(FB_HEIGHT * 0.5f, FB_WIDTH * 0.8f, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(s_bottom, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	if (!imgui::ctru::init()) return false;
	imgui::citro3d::init();

	auto &io = ImGui::GetIO();
	io.IniFilename = nullptr; // disable imgui.ini file
	ImGui::StyleColorsDark();
	auto &style = ImGui::GetStyle();
	style.Colors[ImGuiCol_WindowBg].w = 0.8f;
	style.ScaleAllSizes(0.5f);
	io.DisplaySize = ImVec2(SCREEN_WIDTH, SCREEN_HEIGHT);
	io.DisplayFramebufferScale = ImVec2(FB_SCALE, FB_SCALE);

	while (aptMainLoop()) {

		hidScanInput ();
		auto const kDown = hidKeysDown ();
		if (kDown & KEY_START)
			break;

		imgui::ctru::newFrame();
		ImGui::NewFrame();

		// 1. draw current timestamp
		char timeBuffer[16];
		auto const now = std::time (nullptr);
		std::strftime (timeBuffer, sizeof (timeBuffer), "%H:%M:%S", std::localtime (&now));
		auto const size = ImGui::CalcTextSize (timeBuffer);
		auto const screenWidth = io.DisplaySize.x;
		auto const p1 = ImVec2 (screenWidth, 0.0f);
		auto const p2 = ImVec2 (p1.x - size.x - style.FramePadding.x, style.FramePadding.y);
		ImGui::GetForegroundDrawList ()->AddText (p2, ImGui::GetColorU32 (ImGuiCol_Text), timeBuffer);


		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		{
			ImGui::Begin("VConV");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("TODO");               // Display some text (you can use a format strings too)
			if(ImGui::Button("Exit")){//必须有通过界面上退出的按钮，因为按钮在程序工作时全被占用了
				break;
			}
			ImGui::End();
		}

		ImGui::Render();

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

		// clear frame/depth buffers
		C3D_RenderTargetClear(s_top, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
		C3D_RenderTargetClear(s_bottom, C3D_CLEAR_ALL, CLEAR_COLOR, 0);

		imgui::citro3d::render(s_top, s_bottom);

		C3D_FrameEnd(0);
	}

	imgui::citro3d::exit();
	ImGui::DestroyContext();

	C3D_RenderTargetDelete(s_bottom);
	C3D_RenderTargetDelete(s_top);
	C3D_Fini();

	romfsExit();

	return 0;
}
