#pragma once

#include <string>

#define ARRAYSIZE(x) ((int)(sizeof(x)/sizeof(*(x))))

//kbdType:0=所有布局 1=26键英文键盘 2=数字 3=西方
bool get_text_input(std::string initial,std::string&out,int kbdType,int lengthLimit);
