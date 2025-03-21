#pragma once

struct KEYCODE
{
	unsigned char keycode;
	unsigned char value_length;//输入值的字节数
	short value_range_start;//初始或最小值
	short value_range_end;//按下或最大值
	char description[32];
};

extern const KEYCODE keycodes_3ds[37];
extern const KEYCODE keycodes_xbox[23];
extern const int index_used_keycodes_3ds[18];
extern const int index_used_keycodes_xbox[21];

int keystatus_3ds_by_keycode(unsigned char vconv_key);

#define PAD_X_TRIGGERED(keysheld) ((keysheld)&0x30000000)
#define PAD_Y_TRIGGERED(keysheld) ((keysheld)&0xC0000000)
#define CSTICK_X_TRIGGERED(keysheld) ((keysheld)&0x03000000)
#define CSTICK_Y_TRIGGERED(keysheld) ((keysheld)&0x0C000000)
#define USED_BUTTON_BITS(keystatus) ((keystatus)&0x0000CFFF)