#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <string>
#include <arpa/inet.h>

#include <3ds.h>
#include <citro3d.h>
#include <imgui.h>
#include <imgui_ctru.h>
#include <imgui_citro3d.h>

#include "utilities.h"
#include "vconv.h"
#include "keycodes.h"

constexpr auto SCREEN_WIDTH = 400.0f;
constexpr auto SCREEN_HEIGHT = 480.0f;
constexpr auto FB_SCALE = 2.0f;
constexpr auto FB_WIDTH = SCREEN_WIDTH * FB_SCALE;
constexpr auto FB_HEIGHT = SCREEN_HEIGHT * FB_SCALE;
constexpr auto DISPLAY_TRANSFER_FLAGS =
	GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_XY);

C3D_RenderTarget *s_left = nullptr;
C3D_RenderTarget *s_right = nullptr;
C3D_RenderTarget *s_bottom = nullptr;
void *s_depthStencil = nullptr;

bool running=true;

void draw_controller();
void draw_status_bar();
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

	s_left = C3D_RenderTargetCreate (FB_HEIGHT * 0.5f, FB_WIDTH, GPU_RB_RGBA8, -1);
	C3D_RenderTargetSetOutput (s_left, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
	s_right = C3D_RenderTargetCreate (FB_HEIGHT * 0.5f, FB_WIDTH, GPU_RB_RGBA8, -1);
	C3D_RenderTargetSetOutput (s_right, GFX_TOP, GFX_RIGHT, DISPLAY_TRANSFER_FLAGS);
	s_bottom = C3D_RenderTargetCreate (FB_HEIGHT * 0.5f, FB_WIDTH * 0.8f, GPU_RB_RGBA8, -1);
	C3D_RenderTargetSetOutput (s_bottom, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	// create and attach depth/stencil buffer
	{
		auto const size =
		    C3D_CalcDepthBufSize (FB_HEIGHT * 0.5f, FB_WIDTH, GPU_RB_DEPTH24_STENCIL8);
		s_depthStencil = vramAlloc (size);
		C3D_FrameBufDepth (&s_left->frameBuf, s_depthStencil, GPU_RB_DEPTH24_STENCIL8);
		C3D_FrameBufDepth (&s_right->frameBuf, s_depthStencil, GPU_RB_DEPTH24_STENCIL8);
		C3D_FrameBufDepth (&s_bottom->frameBuf, s_depthStencil, GPU_RB_DEPTH24_STENCIL8);
	}

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
	style.ScrollbarSize=15;
	io.DisplaySize = ImVec2(SCREEN_WIDTH, SCREEN_HEIGHT);
	io.DisplayFramebufferScale = ImVec2(FB_SCALE, FB_SCALE);

	vconv_init();

	while (running && aptMainLoop()) {

		hidScanInput ();
		if(!controller_enabled){
			auto const kDown = hidKeysDown ();
			if (kDown & KEY_START)
				break;
		}

		imgui::ctru::newFrame();
		ImGui::NewFrame();

		draw_controller();
		draw_status_bar();
		draw_vconv_window();

		ImGui::Render();

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

		imgui::citro3d::render(s_left,s_right, s_bottom);

		C3D_FrameEnd(0);

		vconv_send();
	}
	
	vconv_release();

	imgui::citro3d::exit();
	ImGui::DestroyContext();

	C3D_RenderTargetDelete(s_bottom);
	C3D_RenderTargetDelete(s_left);
	C3D_RenderTargetDelete(s_right);
	
	vramFree(s_depthStencil);
	C3D_Fini();

	romfsExit();

	return 0;
}

void draw_controller()
{
	//TODO：绘制控制器状态
}

void draw_status_bar()
{
	ImGuiIO &io=ImGui::GetIO();
	ImGuiStyle &style=ImGui::GetStyle();
	// 1. draw current timestamp
	char timeBuffer[16];
	auto const now = std::time (nullptr);
	std::strftime (timeBuffer, sizeof (timeBuffer), "%H:%M:%S", std::localtime (&now));
	auto const size = ImGui::CalcTextSize (timeBuffer);
	auto const screenWidth = io.DisplaySize.x;
	auto const p1 = ImVec2 (screenWidth, 0.0f);
	auto const p2 = ImVec2 (p1.x - size.x - style.FramePadding.x, style.FramePadding.y);
	ImGui::GetForegroundDrawList ()->AddText (p2, ImGui::GetColorU32 (ImGuiCol_Text), timeBuffer);
	ImGui::GetForegroundDrawList()->AddText(ImVec2(0,io.DisplaySize.y/2-ImGui::GetTextLineHeight()),
	ImGui::GetColorU32(ImGuiCol_Text),error_msg.c_str());

	//TODO：绘制电池状态
	//TODO：绘制网络状态
}

void draw_vconv_window()
{
	ImGuiStyle &style=ImGui::GetStyle();
	ImGui::SetNextWindowPos(ImVec2(40,240));
	ImGui::SetNextWindowSize(ImVec2(320,240));
	ImGuiWindowFlags wf=ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse;
	ImGui::Begin("VConV",NULL,wf);

	if(ImGui::Checkbox("Enable",&controller_enabled)){
		ImGuiIO&io=ImGui::GetIO();
		if(controller_enabled){
			io.ConfigFlags|=ImGuiConfigFlags_NoKeyboard;
			update_sockets_config();
		}else{
			io.ConfigFlags=(~ImGuiConfigFlags_NoKeyboard)&io.ConfigFlags;
		}
	}
	ImGui::SameLine(260-style.WindowPadding.x-style.ScrollbarSize);
	if(ImGui::Button("Exit",ImVec2(60,0))){//必须有通过界面上退出的按钮，因为按钮在程序工作时全被占用了
		running=false;
	}
	ImGui::Text("IP");
	ImGui::SameLine();
	if(ImGui::Button(serverIp.c_str(),ImVec2(150,0))){
		std::string newIp;
		if(get_text_input(serverIp,newIp)){
			auto ip=inet_addr(newIp.c_str());
			if(ip!=INADDR_ANY&&ip!=INADDR_NONE){
				serverIp=newIp;
			}
		}
	}
	ImGui::SameLine();
	ImGui::Text("Port");
	ImGui::SameLine();
	if(ImGui::Button(std::to_string(serverPort).c_str(),ImVec2(80,0))){
		std::string sPort=std::to_string(serverPort);
		if(get_text_input(sPort,sPort)){
			serverPort=atoi(sPort.c_str());
			//TODO:更新设置
		}
	}

	//https://xinqinew.github.io/2022/02/imgui%E8%A1%A8%E6%A0%BC/
	if(ImGui::BeginTable("Mapping",3)){
		ImGui::TableSetupColumn("3DS Key",ImGuiTableColumnFlags_WidthFixed,155);
		ImGui::TableSetupColumn("Value",ImGuiTableColumnFlags_WidthFixed,30);
		ImGui::TableSetupColumn("Mapping",ImGuiTableColumnFlags_WidthStretch);
		for(int i=0;i<ARRAYSIZE(button_mapping_3ds_xbox);i++){
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text(keycodes_3ds[index_used_keycodes_3ds[i]].description);
			ImGui::TableNextColumn();
			ImGui::Text("%d",keystatus_3ds_by_keycode(keycodes_3ds[index_used_keycodes_3ds[i]].keycode));
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(105);
			ImGui::PushID(2000+i);
			if(ImGui::BeginCombo("",keycodes_xbox[index_used_keycodes_xbox[button_mapping_3ds_xbox[i]]].description,ImGuiComboFlags_PopupAlignLeft)){
				for(int m=0;m<ARRAYSIZE(index_used_keycodes_xbox);m++){
					const bool is_selected = (button_mapping_3ds_xbox[i] == m);
					if (ImGui::Selectable(keycodes_xbox[index_used_keycodes_xbox[m]].description, is_selected)){
						button_mapping_3ds_xbox[i] = m;
					}
					
					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected){
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
	ImGui::End();
}
