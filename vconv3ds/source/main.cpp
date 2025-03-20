#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <vector>
#include <set>
#include <algorithm>
#include <regex>

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

bool running=true;
bool controller_enabled=false;
#define SERVER_IP_DEFAULT "192.168.1.2"
char serverIp[16]=SERVER_IP_DEFAULT;
int serverPort=32000;

void init_sockets();

void draw_controller();
void draw_status_bar(ImGuiIO &io,ImGuiStyle &style);
void draw_vconv_window();

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

	init_sockets();

	while (running && aptMainLoop()) {

		hidScanInput ();
		auto const kDown = hidKeysDown ();
		if (kDown & KEY_START)
			break;

		imgui::ctru::newFrame();
		ImGui::NewFrame();

		ImGui::ShowDemoWindow();

		draw_controller();
		draw_status_bar(io,style);
		draw_vconv_window();

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

void init_sockets()
{
	//TODO：初始化发送和接收的Socket
}

void draw_controller()
{
	//TODO：绘制控制器状态
}

void draw_status_bar(ImGuiIO &io,ImGuiStyle &style)
{
	// 1. draw current timestamp
	char timeBuffer[16];
	auto const now = std::time (nullptr);
	std::strftime (timeBuffer, sizeof (timeBuffer), "%H:%M:%S", std::localtime (&now));
	auto const size = ImGui::CalcTextSize (timeBuffer);
	auto const screenWidth = io.DisplaySize.x;
	auto const p1 = ImVec2 (screenWidth, 0.0f);
	auto const p2 = ImVec2 (p1.x - size.x - style.FramePadding.x, style.FramePadding.y);
	ImGui::GetForegroundDrawList ()->AddText (p2, ImGui::GetColorU32 (ImGuiCol_Text), timeBuffer);

	//TODO：绘制电池状态
	//TODO：绘制网络状态
}

void draw_vconv_window()
{
	ImGui::SetNextWindowPos(ImVec2(40,240));
	ImGui::SetNextWindowSize(ImVec2(320,240));
	ImGuiWindowFlags wf=ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse;
	ImGui::Begin("VConV",NULL,wf);                          // Create a window called "Hello, world!" and append into it.

	ImGui::Checkbox("Enable Input",&controller_enabled);
	ImGui::SameLine();
	if(ImGui::Button("Exit")){//必须有通过界面上退出的按钮，因为按钮在程序工作时全被占用了
		running=false;
	}
	if(ImGui::InputText("Server IP Address",serverIp,std::size(serverIp),ImGuiInputTextFlags_AutoSelectAll)){
		std::string str=serverIp;
		std::smatch sm;
		if(strlen(serverIp)>15||!std::regex_match(str,sm,std::regex("(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)"))){
			strcpy(serverIp,SERVER_IP_DEFAULT);
		}else{
			for(const auto &m:sm){
				if(atoi(m.str().c_str())>255){
					strcpy(serverIp,SERVER_IP_DEFAULT);
					break;
				}
			}
		}
	}
	if(ImGui::InputInt("Server Port",&serverPort)){
		serverPort=std::min(std::max(0,serverPort),65535);
	}
	ImGui::End();
}
