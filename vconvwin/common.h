#pragma once

#define PhyPixelsX(logicPixelsX) MulDiv(logicPixelsX,GetDeviceCaps(GetDC(0),LOGPIXELSX),USER_DEFAULT_SCREEN_DPI)
#define PhyPixelsY(logicPixelsY) MulDiv(logicPixelsY,GetDeviceCaps(GetDC(0),LOGPIXELSY),USER_DEFAULT_SCREEN_DPI)


