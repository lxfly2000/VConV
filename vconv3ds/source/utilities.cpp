#include "utilities.h"

#include <3ds.h>

bool get_text_input(std::string initial,std::string&out)
{
	SwkbdState kbd;

	swkbdInit (&kbd, SWKBD_TYPE_NORMAL, 2, -1);
	swkbdSetButton (&kbd, SWKBD_BUTTON_LEFT, "Cancel", false);
	swkbdSetButton (&kbd, SWKBD_BUTTON_RIGHT, "OK", true);
	swkbdSetInitialText (&kbd,initial.c_str());

	char buffer[32]   = {0};
	auto const button = swkbdInputText (&kbd, buffer, ARRAYSIZE (buffer));
	if (button == SWKBD_BUTTON_RIGHT){
		out=buffer;
		return true;
	}
	return false;
}
