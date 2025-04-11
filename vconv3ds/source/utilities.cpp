#include "utilities.h"

#include <3ds.h>

bool get_text_input(std::string initial,std::string&out,int kbdType,int lengthLimit)
{
	SwkbdState kbd;

	swkbdInit (&kbd, (SwkbdType)kbdType, 2, lengthLimit);
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
