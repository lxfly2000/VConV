#include "keycodes.h"
#include <3ds.h>

//https://3dbrew.org/wiki/HID_Shared_Memory
//33-36需参照25-32的状态处理死区
const KEYCODE keycodes_3ds[]={
	{0,0,0,0,"(None)"},
	{1,1,0,1,"A"},
	{2,1,0,1,"B"},
	{3,1,0,1,"Select"},
	{4,1,0,1,"Start"},
	{5,1,0,1,"Right"},
	{6,1,0,1,"Left"},
	{7,1,0,1,"Up"},
	{8,1,0,1,"Down"},
	{9,1,0,1,"R"},
	{10,1,0,1,"L"},
	{11,1,0,1,"X"},
	{12,1,0,1,"Y"},
	{13,1,0,1,"Inverted value of GPIO bit0."},
	{14,1,0,1,"Inverted value of GPIO bit14."},
	{15,1,0,1,"ZL (N3DS)"},
	{16,1,0,1,"ZR (N3DS)"},
	{17,1,0,1,"(Undefined)"},
	{18,1,0,1,"(Undefined)"},
	{19,1,0,1,"(Undefined)"},
	{20,1,0,1,"(Undefined)"},
	{21,1,0,1,"Touch"},
	{22,1,0,1,"(Undefined)"},
	{23,1,0,1,"(Undefined)"},
	{24,1,0,1,"(Undefined)"},
	{25,1,0,1,"C-Stick Right (N3DS)"},
	{26,1,0,1,"C-Stick Left (N3DS)"},
	{27,1,0,1,"C-Stick Up (N3DS)"},
	{28,1,0,1,"C-Stick Down (N3DS)"},
	{29,1,0,1,"Circle Pad Right"},
	{30,1,0,1,"Circle Pad Left"},
	{31,1,0,1,"Circle Pad Up"},
	{32,1,0,1,"Circle Pad Down"},
	{33,2,-0x9C,0x9C,"X-Axis of Circle Pad"},
	{34,2,-0x9C,0x9C,"Y-Axis of Circle Pad"},
	{35,2,-146,146,"X-Axis of C-Stick (N3DS)"},
	{36,2,-146,146,"Y-Axis of C-Stick (N3DS)"},
};

//参考：https://docs.microsoft.com/en-us/windows/win32/api/xinput/ns-xinput-xinput_gamepad
const KEYCODE keycodes_xbox[23]={
	{0,0,0,0,"(None)"},
	{1,1,0,1,"Up"},
	{2,1,0,1,"Down"},
	{3,1,0,1,"Left"},
	{4,1,0,1,"Right"},
	{5,1,0,1,"Start"},
	{6,1,0,1,"Back"},
	{7,1,0,1,"Left Stick"},
	{8,1,0,1,"Right Stick"},
	{9,1,0,1,"Left Shoulder"},
	{10,1,0,1,"Right Shoulder"},
	{11,0,0,0,"(Undefined)"},
	{12,0,0,0,"(Undefined)"},
	{13,1,0,1,"A"},
	{14,1,0,1,"B"},
	{15,1,0,1,"X"},
	{16,1,0,1,"Y"},
	{17,1,0,255,"Left Trigger"},
	{18,1,0,255,"Right Trigger"},
	{19,2,-32768,32767,"X-Axis of Left Stick"},
	{20,2,-32768,32767,"Y-Axis of Left Stick"},
	{21,2,-32768,32767,"X-Axis of Right Stick"},
	{22,2,-32768,32767,"Y-Axis of Right Stick"},
};

const KEYCODE keycodes_onscreen_buttons[4]={
	{0,1,0,1,"X1"},
	{1,1,0,1,"X2"},
	{2,1,0,1,"X3"},
	{3,1,0,1,"X4"}
};

const int index_used_keycodes_3ds[18]={1,2,3,4,5,6,7,8,9,10,11,12,15,16,33,34,35,36};
const int keycodes_3ds_to_index_used[37]={-1,0,1,2,3,4,5,6,7,8,9,10,11,-1,-1,12,13,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,14,15,16,17};
const int index_used_keycodes_xbox[21]={0,1,2,3,4,5,6,7,8,9,10,13,14,15,16,17,18,19,20,21,22};

int keystatus_onscreen_buttons[4]={0,0,0,0};//触屏按钮状态

int keystatus_3ds_by_keycode(unsigned char vconv_key)
{
	if(vconv_key==0){
		return 0;
	}else if(vconv_key<=32){
		return (keysHeld()&(1<<(vconv_key-1)))?1:0;
	}else if(vconv_key<=34){
		circlePosition cp;
		circleRead(&cp);
		if(vconv_key==33){
			return cp.dx;
		}else{
			return cp.dy;
		}
	}else if(vconv_key<=36){
		circlePosition cp;
		hidCstickRead(&cp);
		if(vconv_key==35){
			return cp.dx;
		}else{
			return cp.dy;
		}
	}
	return 0;
}
