#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <string>
#include <fstream>
#include <arpa/inet.h>

#include <3ds.h>
#include <citro3d.h>
#include <tex3ds.h>
#include <imgui.h>
#include <imgui_ctru.h>
#include <imgui_citro3d.h>

#include "utilities.h"
#include "vconv.h"
#include "keycodes.h"
#include "audio.h"
#include "gfx.h"
#include "gfx_t3x.h"
#include "xboximg1.h"
#include "xboximg1_t3x.h"
#include "xboximg2.h"
#include "xboximg2_t3x.h"

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

bool controller_enabled=false;

Tex3DS_Texture s_gfxT3x,s_xboximgT3x1,s_xboximgT3x2;
C3D_Tex s_gfxTexture,s_xboximgTexture1,s_xboximgTexture2;

void draw_controller();
void draw_status_bar();
void draw_vconv_window();
void image_init();
void image_release();
bool enable_backlights(bool);

int main(int argc, char *argv[]) {
	osSetSpeedupEnable(true);
	romfsInit();
	ptmuInit();
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

	audio_init();
	if(vconv_init()){
		audio_play(AUDIO_ID_SUCCESS);
	}else{
		audio_play(AUDIO_ID_ERROR);
	}
	image_init();

	while (running && aptMainLoop()) {

		hidScanInput ();
		if(controller_enabled){
			if(!vconv_send()){
				audio_play(AUDIO_ID_ERROR);
			}
		}else{
			auto const kDown = hidKeysDown ();
			if (kDown & KEY_START)
				break;
		}

		if(screen_light){
			imgui::ctru::newFrame();
			ImGui::NewFrame();

			draw_controller();
			draw_status_bar();
			draw_vconv_window();

			ImGui::Render();

			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

			imgui::citro3d::render(s_left,s_right, s_bottom);

			C3D_FrameEnd(0);
		}else{
			auto const kDown=hidKeysDown();
			if(kDown&KEY_TOUCH){
				screen_light=true;
				if(enable_backlights(screen_light)){
					audio_play(AUDIO_ID_SUCCESS);
				}else{
					audio_play(AUDIO_ID_ERROR);
				}
			}
		}
	}
	
	audio_release();
	image_release();
	vconv_release();

	imgui::citro3d::exit();
	ImGui::DestroyContext();

	C3D_RenderTargetDelete(s_bottom);
	C3D_RenderTargetDelete(s_left);
	C3D_RenderTargetDelete(s_right);
	
	vramFree(s_depthStencil);
	C3D_Fini();

	enable_backlights(true);

	gfxExit();
	ptmuExit();
	romfsExit();

	return 0;
}

void draw_controller()
{
	ImGuiIO &io=ImGui::GetIO();
	ImGuiStyle &style=ImGui::GetStyle();
	ImGui::GetForegroundDrawList()->AddText(ImVec2(style.FramePadding.x,io.DisplaySize.y/2-ImGui::GetTextLineHeight()-style.FramePadding.y),
	ImGui::GetColorU32(ImGuiCol_Text),error_msg.c_str());
	//绘制控制器状态

	//方向键和ABXY键的偏移方向（从上方开始顺时针）
	constexpr int direction_x[4] = {0, 1, 0, -1};
	constexpr int direction_y[4] = {-1, 0, 1, 0};

	//定义按键绘制位置
	constexpr auto dpad_pos=ImVec2(120,180);
	constexpr auto left_stick_pos=ImVec2(60,110);
	constexpr auto right_stick_pos=ImVec2(280,180);
	constexpr auto abxy_pos=ImVec2(340,110);
	constexpr auto start_pos=ImVec2(230,110);
	constexpr auto back_pos=ImVec2(170,110);
	constexpr auto left_shoulder_bottomleft_pos=ImVec2(30,60);
	constexpr auto left_trigger_bottomleft_pos=ImVec2(30,40);
	constexpr auto right_shoulder_bottomright_pos=ImVec2(370,60);
	constexpr auto right_trigger_bottomright_pos=ImVec2(370,40);

	//绘制方向键
	constexpr unsigned dpad_images[] = {
		xboximg2_up_idx,
		xboximg2_right_idx,
		xboximg2_down_idx,
		xboximg2_left_idx,
	};

	constexpr unsigned dpad_pressed_images[] = {
		xboximg2_uppress_idx,
		xboximg2_rightpress_idx,
		xboximg2_downpress_idx,
		xboximg2_leftpress_idx,
	};

	constexpr short dpad_keycodes[] = {1,4,2,3};

	for(int i=0;i<4;i++){
		auto const tex = Tex3DS_GetSubTexture(s_xboximgT3x2, xbox_controller_key_status[dpad_keycodes[i]] ? dpad_pressed_images[i] : dpad_images[i]);
		auto const posTopLeft = ImVec2(dpad_pos.x + (direction_x[i]-0.5f) * tex->width, dpad_pos.y + (direction_y[i]-0.5f) * tex->height);
		auto const posBottomRight = ImVec2(dpad_pos.x + (direction_x[i]+0.5f) * tex->width, dpad_pos.y + (direction_y[i]+0.5f) * tex->height);
		auto const uvTopLeft = ImVec2(tex->left, tex->top);
		auto const uvBottomRight = ImVec2(tex->right, tex->bottom);
		ImGui::GetForegroundDrawList()->AddImage((ImTextureID)&s_xboximgTexture2, posTopLeft, posBottomRight, uvTopLeft, uvBottomRight, ImGui::GetColorU32(ImGuiCol_Text));
	}

	//绘制ABXY键
	constexpr unsigned abxy_images[] = {
		xboximg1_y_idx,
		xboximg1_b_idx,
		xboximg1_a_idx,
		xboximg1_x_idx,
	};

	constexpr unsigned abxy_pressed_images[] = {
		xboximg1_ypress_idx,
		xboximg1_bpress_idx,
		xboximg1_apress_idx,
		xboximg1_xpress_idx,
	};

	constexpr short abxy_keycodes[] = {16, 14, 13, 15};

	for(int i=0;i<4;i++){
		auto const tex = Tex3DS_GetSubTexture(s_xboximgT3x1, xbox_controller_key_status[abxy_keycodes[i]] ? abxy_pressed_images[i] : abxy_images[i]);
		auto const posTopLeft = ImVec2(abxy_pos.x + (direction_x[i]-0.5f) * tex->width, abxy_pos.y + (direction_y[i]-0.5f) * tex->height);
		auto const posBottomRight = ImVec2(abxy_pos.x + (direction_x[i]+0.5f) * tex->width, abxy_pos.y + (direction_y[i]+0.5f) * tex->height);
		auto const uvTopLeft = ImVec2(tex->left, tex->top);
		auto const uvBottomRight = ImVec2(tex->right, tex->bottom);
		ImGui::GetForegroundDrawList()->AddImage((ImTextureID)&s_xboximgTexture1, posTopLeft, posBottomRight, uvTopLeft, uvBottomRight, ImGui::GetColorU32(ImGuiCol_Text));
	}

	//绘制开始、返回键
	auto const startTex = Tex3DS_GetSubTexture(s_xboximgT3x1, xbox_controller_key_status[5] ? xboximg1_stpress_idx : xboximg1_st_idx);
	auto const startPosTopLeft = ImVec2(start_pos.x - startTex->width / 2, start_pos.y - startTex->height / 2);
	auto const startPosBottomRight = ImVec2(start_pos.x + startTex->width / 2, start_pos.y + startTex->height / 2);
	auto const startUvTopLeft = ImVec2(startTex->left, startTex->top);
	auto const startUvBottomRight = ImVec2(startTex->right, startTex->bottom);
	ImGui::GetForegroundDrawList()->AddImage(
		(ImTextureID)&s_xboximgTexture1, startPosTopLeft, startPosBottomRight,
		startUvTopLeft, startUvBottomRight, ImGui::GetColorU32(ImGuiCol_Text));
	
	auto const backTex = Tex3DS_GetSubTexture(s_xboximgT3x1, xbox_controller_key_status[6] ? xboximg1_bkpress_idx : xboximg1_bk_idx);
	auto const backPosTopLeft = ImVec2(back_pos.x - backTex->width / 2, back_pos.y - backTex->height / 2);
	auto const backPosBottomRight = ImVec2(back_pos.x + backTex->width / 2, back_pos.y + backTex->height / 2);
	auto const backUvTopLeft = ImVec2(backTex->left, backTex->top);
	auto const backUvBottomRight = ImVec2(backTex->right, backTex->bottom);
	ImGui::GetForegroundDrawList()->AddImage(
		(ImTextureID)&s_xboximgTexture1, backPosTopLeft, backPosBottomRight,
		backUvTopLeft, backUvBottomRight, ImGui::GetColorU32(ImGuiCol_Text));
	
	//绘制左肩键
	auto const leftShoulderTex = Tex3DS_GetSubTexture(s_xboximgT3x2, xbox_controller_key_status[9] ? xboximg2_lbpress_idx : xboximg2_lb_idx);
	auto const leftShoulderPosTopLeft = ImVec2(left_shoulder_bottomleft_pos.x, left_shoulder_bottomleft_pos.y - leftShoulderTex->height);
	auto const leftShoulderPosBottomRight = ImVec2(left_shoulder_bottomleft_pos.x + leftShoulderTex->width, left_shoulder_bottomleft_pos.y);
	auto const leftShoulderUvTopLeft = ImVec2(leftShoulderTex->left, leftShoulderTex->top);
	auto const leftShoulderUvBottomRight = ImVec2(leftShoulderTex->right, leftShoulderTex->bottom);
	ImGui::GetForegroundDrawList()->AddImage(
		(ImTextureID)&s_xboximgTexture2, leftShoulderPosTopLeft, leftShoulderPosBottomRight,
		leftShoulderUvTopLeft, leftShoulderUvBottomRight, ImGui::GetColorU32(ImGuiCol_Text));
	
	//绘制右肩键
	auto const rightShoulderTex = Tex3DS_GetSubTexture(s_xboximgT3x2, xbox_controller_key_status[10] ? xboximg2_rbpress_idx : xboximg2_rb_idx);
	auto const rightShoulderPosTopLeft = ImVec2(right_shoulder_bottomright_pos.x - rightShoulderTex->width, right_shoulder_bottomright_pos.y - rightShoulderTex->height);
	auto const rightShoulderPosBottomRight = right_shoulder_bottomright_pos;
	auto const rightShoulderUvTopLeft = ImVec2(rightShoulderTex->left, rightShoulderTex->top);
	auto const rightShoulderUvBottomRight = ImVec2(rightShoulderTex->right, rightShoulderTex->bottom);
	ImGui::GetForegroundDrawList()->AddImage(
		(ImTextureID)&s_xboximgTexture2, rightShoulderPosTopLeft, rightShoulderPosBottomRight,
		rightShoulderUvTopLeft, rightShoulderUvBottomRight, ImGui::GetColorU32(ImGuiCol_Text));
	
	//绘制左扳机
	auto const leftTriggerTexRelease = Tex3DS_GetSubTexture(s_xboximgT3x1, xboximg1_lt_idx);
	auto const leftTriggerPosReleaseTopLeft = ImVec2(left_trigger_bottomleft_pos.x, left_trigger_bottomleft_pos.y - leftTriggerTexRelease->height);
	auto const leftTriggerPosReleaseBottomRight = ImVec2(left_trigger_bottomleft_pos.x + leftTriggerTexRelease->width, left_trigger_bottomleft_pos.y);
	auto const leftTriggerUvReleaseTopLeft = ImVec2(leftTriggerTexRelease->left, leftTriggerTexRelease->top);
	auto const leftTriggerUvReleaseBottomRight = ImVec2(leftTriggerTexRelease->right, leftTriggerTexRelease->bottom);
	ImGui::GetForegroundDrawList()->AddImage(
		(ImTextureID)&s_xboximgTexture1, leftTriggerPosReleaseTopLeft, leftTriggerPosReleaseBottomRight,
		leftTriggerUvReleaseTopLeft, leftTriggerUvReleaseBottomRight, ImGui::GetColorU32(ImGuiCol_Text));
	
	auto const leftTriggerTexPress = Tex3DS_GetSubTexture(s_xboximgT3x1, xboximg1_ltpress_idx);
	auto const leftTriggerPosPressTopLeft = ImVec2(left_trigger_bottomleft_pos.x, left_trigger_bottomleft_pos.y - leftTriggerTexPress->height);
	auto const leftTriggerPosPressBottomRight = ImVec2(left_trigger_bottomleft_pos.x + leftTriggerTexPress->width, left_trigger_bottomleft_pos.y - leftTriggerTexPress->height + leftTriggerTexPress->height*xbox_controller_key_status[17]/255.0f);
	auto const leftTriggerUvPressTopLeft = ImVec2(leftTriggerTexPress->left, leftTriggerTexPress->top);
	auto const leftTriggerUvPressBottomRight = ImVec2(leftTriggerTexPress->right, leftTriggerTexPress->bottom);
	ImGui::GetForegroundDrawList()->AddImage(
		(ImTextureID)&s_xboximgTexture1, leftTriggerPosPressTopLeft, leftTriggerPosPressBottomRight,
		leftTriggerUvPressTopLeft, leftTriggerUvPressBottomRight, ImGui::GetColorU32(ImGuiCol_Text));

	//绘制右扳机
	auto const rightTriggerTexRelease = Tex3DS_GetSubTexture(s_xboximgT3x1, xboximg1_rt_idx);
	auto const rightTriggerPosReleaseTopLeft = ImVec2(right_trigger_bottomright_pos.x - rightTriggerTexRelease->width, right_trigger_bottomright_pos.y - rightTriggerTexRelease->height);
	auto const rightTriggerPosReleaseBottomRight = right_trigger_bottomright_pos;
	auto const rightTriggerUvReleaseTopLeft = ImVec2(rightTriggerTexRelease->left, rightTriggerTexRelease->top);
	auto const rightTriggerUvReleaseBottomRight = ImVec2(rightTriggerTexRelease->right, rightTriggerTexRelease->bottom);
	ImGui::GetForegroundDrawList()->AddImage(
		(ImTextureID)&s_xboximgTexture1, rightTriggerPosReleaseTopLeft, rightTriggerPosReleaseBottomRight,
		rightTriggerUvReleaseTopLeft, rightTriggerUvReleaseBottomRight, ImGui::GetColorU32(ImGuiCol_Text));
	
	auto const rightTriggerTexPress = Tex3DS_GetSubTexture(s_xboximgT3x1, xboximg1_rtpress_idx);
	auto const rightTriggerPosPressTopLeft = ImVec2(right_trigger_bottomright_pos.x - rightTriggerTexPress->width, right_trigger_bottomright_pos.y - rightTriggerTexPress->height);
	auto const rightTriggerPosPressBottomRight = ImVec2(right_trigger_bottomright_pos.x, right_trigger_bottomright_pos.y - rightTriggerTexPress->height + rightTriggerTexPress->height*xbox_controller_key_status[18]/255.0f);
	auto const rightTriggerUvPressTopLeft = ImVec2(rightTriggerTexPress->left, rightTriggerTexPress->top);
	auto const rightTriggerUvPressBottomRight = ImVec2(rightTriggerTexPress->right, rightTriggerTexPress->bottom);
	ImGui::GetForegroundDrawList()->AddImage(
		(ImTextureID)&s_xboximgTexture1, rightTriggerPosPressTopLeft, rightTriggerPosPressBottomRight,
		rightTriggerUvPressTopLeft, rightTriggerUvPressBottomRight, ImGui::GetColorU32(ImGuiCol_Text));
	
	//绘制左摇杆
	auto const leftStickRangeTex = Tex3DS_GetSubTexture(s_xboximgT3x1, xboximg1_lsrange_idx);
	auto const leftStickRangePosTopLeft = ImVec2(left_stick_pos.x - leftStickRangeTex->width / 2, left_stick_pos.y - leftStickRangeTex->height / 2);
	auto const leftStickRangePosBottomRight = ImVec2(left_stick_pos.x + leftStickRangeTex->width / 2, left_stick_pos.y + leftStickRangeTex->height / 2);
	auto const leftStickRangeUvTopLeft = ImVec2(leftStickRangeTex->left, leftStickRangeTex->top);
	auto const leftStickRangeUvBottomRight = ImVec2(leftStickRangeTex->right, leftStickRangeTex->bottom);
	ImGui::GetForegroundDrawList()->AddImage(
		(ImTextureID)&s_xboximgTexture1, leftStickRangePosTopLeft, leftStickRangePosBottomRight,
		leftStickRangeUvTopLeft, leftStickRangeUvBottomRight, ImGui::GetColorU32(ImGuiCol_Text));
	
	auto const leftStickCenterPos = ImVec2(left_stick_pos.x + leftStickRangeTex->width / 2.0f * xbox_controller_key_status[19] / 32768.0f,
		left_stick_pos.y - leftStickRangeTex->height / 2.0f * xbox_controller_key_status[20] / 32768.0f);
	auto const leftStickTex = Tex3DS_GetSubTexture(s_xboximgT3x1, xbox_controller_key_status[7] ? xboximg1_lspress_idx : xboximg1_ls_idx);
	auto const leftStickPosTopLeft = ImVec2(leftStickCenterPos.x - leftStickTex->width / 2, leftStickCenterPos.y - leftStickTex->height / 2);
	auto const leftStickPosBottomRight = ImVec2(leftStickCenterPos.x + leftStickTex->width / 2, leftStickCenterPos.y + leftStickTex->height / 2);
	auto const leftStickUvTopLeft = ImVec2(leftStickTex->left, leftStickTex->top);
	auto const leftStickUvBottomRight = ImVec2(leftStickTex->right, leftStickTex->bottom);
	ImGui::GetForegroundDrawList()->AddImage(
		(ImTextureID)&s_xboximgTexture1, leftStickPosTopLeft, leftStickPosBottomRight,
		leftStickUvTopLeft, leftStickUvBottomRight, ImGui::GetColorU32(ImGuiCol_Text));
	
	//绘制右摇杆
	auto const rightStickRangeTex = Tex3DS_GetSubTexture(s_xboximgT3x1, xboximg1_rsrange_idx);
	auto const rightStickRangePosTopLeft = ImVec2(right_stick_pos.x - rightStickRangeTex->width / 2, right_stick_pos.y - rightStickRangeTex->height / 2);
	auto const rightStickRangePosBottomRight = ImVec2(right_stick_pos.x + rightStickRangeTex->width / 2,
		right_stick_pos.y + rightStickRangeTex->height / 2);
	auto const rightStickRangeUvTopLeft = ImVec2(rightStickRangeTex->left, rightStickRangeTex->top);
	auto const rightStickRangeUvBottomRight = ImVec2(rightStickRangeTex->right, rightStickRangeTex->bottom);
	ImGui::GetForegroundDrawList()->AddImage(
		(ImTextureID)&s_xboximgTexture1, rightStickRangePosTopLeft, rightStickRangePosBottomRight,
		rightStickRangeUvTopLeft, rightStickRangeUvBottomRight, ImGui::GetColorU32(ImGuiCol_Text));
	
	auto const rightStickCenterPos = ImVec2(right_stick_pos.x + rightStickRangeTex->width / 2.0f * xbox_controller_key_status[21] / 32768.0f,
		right_stick_pos.y - rightStickRangeTex->height / 2.0f * xbox_controller_key_status[22] / 32768.0f);
	auto const rightStickTex = Tex3DS_GetSubTexture(s_xboximgT3x1, xbox_controller_key_status[8] ? xboximg1_rspress_idx : xboximg1_rs_idx);
	auto const rightStickPosTopLeft = ImVec2(rightStickCenterPos.x - rightStickTex->width / 2, rightStickCenterPos.y - rightStickTex->height / 2);
	auto const rightStickPosBottomRight = ImVec2(rightStickCenterPos.x + rightStickTex->width / 2, rightStickCenterPos.y + rightStickTex->height / 2);
	auto const rightStickUvTopLeft = ImVec2(rightStickTex->left, rightStickTex->top);
	auto const rightStickUvBottomRight = ImVec2(rightStickTex->right, rightStickTex->bottom);
	ImGui::GetForegroundDrawList()->AddImage(
		(ImTextureID)&s_xboximgTexture1, rightStickPosTopLeft, rightStickPosBottomRight,
		rightStickUvTopLeft, rightStickUvBottomRight, ImGui::GetColorU32(ImGuiCol_Text));
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

	u8 isCharging=0,batteryLevel=0;
	PTMU_GetBatteryChargeState(&isCharging);
	PTMU_GetBatteryLevel(&batteryLevel);
	u8 wifiLevel=osGetWifiStrength();

	constexpr unsigned batteryLevels[] = {
	    gfx_battery0_idx,
	    gfx_battery0_idx,
	    gfx_battery1_idx,
	    gfx_battery2_idx,
	    gfx_battery3_idx,
	    gfx_battery4_idx,
	};

	constexpr unsigned wifiLevels[] = {
	    gfx_wifi0_idx,
	    gfx_wifi1_idx,
	    gfx_wifi2_idx,
	    gfx_wifi3_idx,
	};

	constexpr unsigned chargingStatus[]={
		gfx_charge0_idx,
		gfx_charge1_idx,
	};

	auto lineHeight=16.0f;//ImGui::GetTextLineHeight();
	
	auto const batteryTex = Tex3DS_GetSubTexture (s_gfxT3x, batteryLevels[batteryLevel]);
	auto const p3=ImVec2(style.FramePadding.x,style.FramePadding.y);
	auto const p4=ImVec2(batteryTex->width*lineHeight/batteryTex->height,lineHeight+style.FramePadding.y);
	auto const uv3=ImVec2(batteryTex->left,batteryTex->top);
	auto const uv4=ImVec2(batteryTex->right,batteryTex->bottom);
	ImGui::GetForegroundDrawList()->AddImage((ImTextureID)&s_gfxTexture, p3, p4, uv3, uv4, ImGui::GetColorU32 (ImGuiCol_Text));

	auto const chargingTex = Tex3DS_GetSubTexture (s_gfxT3x, chargingStatus[isCharging]);
	auto const p5=ImVec2(p4.x+style.ItemSpacing.x,style.FramePadding.y);
	auto const p6=ImVec2(p4.x+style.ItemSpacing.x+chargingTex->width*lineHeight/chargingTex->height,lineHeight+style.FramePadding.y);
	auto const uv5=ImVec2(chargingTex->left,chargingTex->top);
	auto const uv6=ImVec2(chargingTex->right,chargingTex->bottom);
	ImGui::GetForegroundDrawList()->AddImage((ImTextureID)&s_gfxTexture, p5, p6, uv5, uv6, ImGui::GetColorU32 (ImGuiCol_Text));

	auto const wifiTex = Tex3DS_GetSubTexture (s_gfxT3x, wifiLevels[wifiLevel]);
	auto const p7=ImVec2(p6.x+style.ItemSpacing.x,style.FramePadding.y);
	auto const p8=ImVec2(p6.x+style.ItemSpacing.x+wifiTex->width*lineHeight/wifiTex->height,lineHeight+style.FramePadding.y);
	auto const uv7=ImVec2(wifiTex->left,wifiTex->top);
	auto const uv8=ImVec2(wifiTex->right,wifiTex->bottom);
	ImGui::GetForegroundDrawList()->AddImage((ImTextureID)&s_gfxTexture, p7, p8, uv7, uv8, ImGui::GetColorU32 (ImGuiCol_Text));
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
			memset(xbox_controller_key_status,0,sizeof(xbox_controller_key_status));
		}else{
			io.ConfigFlags=(~ImGuiConfigFlags_NoKeyboard)&io.ConfigFlags;
		}
		audio_play(controller_enabled?AUDIO_ID_SUCCESS:AUDIO_ID_ERROR);
	}
	ImGui::SetItemTooltip("UI input will be disabled.");
	ImGui::SameLine();
	if(ImGui::Button("Turn off backlights")){
		//不要用PowerOn/OffAllBacklights，疑似有Bug
		screen_light=false;
		enable_backlights(screen_light);
		audio_play(AUDIO_ID_ERROR);
	}
	ImGui::SetItemTooltip("Touch screen to turn on backlights.");
	ImGui::SameLine(260-style.WindowPadding.x-style.ScrollbarSize);
	if(ImGui::Button("Exit",ImVec2(60,0))){//必须有通过界面上退出的按钮，因为控制器按钮在程序工作时全被占用了
		running=false;
	}
	ImGui::Text("IP");
	ImGui::SameLine();
	if(ImGui::Button(serverIp.c_str(),ImVec2(150,0))){
		std::string newIp;
		if(get_text_input(serverIp,newIp,1,39)){
			auto ip=inet_addr(newIp.c_str());
			if(ip!=INADDR_ANY&&ip!=INADDR_NONE){
				serverIp=newIp;
				update_sockets_config();
			}
		}
	}
	ImGui::SameLine();
	ImGui::Text("Port");
	ImGui::SameLine();
	if(ImGui::Button(std::to_string(serverPort).c_str(),ImVec2(80,0))){
		std::string sPort=std::to_string(serverPort);
		if(get_text_input(sPort,sPort,2,5)){
			serverPort=atoi(sPort.c_str());
			update_sockets_config();
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

void image_init()
{
	s_gfxT3x=Tex3DS_TextureImport(gfx_t3x,gfx_t3x_size,&s_gfxTexture,nullptr,false);
	if(!s_gfxT3x)
	{
		error_msg="Cannot import texture.";
		return;
	}
	s_xboximgT3x1=Tex3DS_TextureImport(xboximg1_t3x,xboximg1_t3x_size,&s_xboximgTexture1,nullptr,false);
	if(!s_xboximgT3x1)
	{
		error_msg="Cannot import Xbox image texture1.";
		return;
	}
	s_xboximgT3x2=Tex3DS_TextureImport(xboximg2_t3x,xboximg2_t3x_size,&s_xboximgTexture2,nullptr,false);
	if(!s_xboximgT3x2)
	{
		error_msg="Cannot import Xbox image texture2.";
		return;
	}
	C3D_TexSetFilter (&s_gfxTexture, GPU_LINEAR, GPU_LINEAR);
	C3D_TexSetFilter (&s_xboximgTexture1, GPU_LINEAR, GPU_LINEAR);
	C3D_TexSetFilter (&s_xboximgTexture2, GPU_LINEAR, GPU_LINEAR);
}

void image_release()
{
	Tex3DS_TextureFree(s_gfxT3x);
	Tex3DS_TextureFree(s_xboximgT3x1);
	Tex3DS_TextureFree(s_xboximgT3x2);
	C3D_TexDelete(&s_gfxTexture);
	C3D_TexDelete(&s_xboximgTexture1);
	C3D_TexDelete(&s_xboximgTexture2);
}

bool enable_backlights(bool b)
{
	gspLcdInit();
	Result r=0;
	if(b){
		r=GSPLCD_PowerOnBacklight(GSPLCD_SCREEN_BOTH);
	}else{
		r=GSPLCD_PowerOffBacklight(GSPLCD_SCREEN_BOTH);
	}
	gspLcdExit();
	return r==0;
}
