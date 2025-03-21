#pragma once

#include <string>

#define ARRAYSIZE(x) ((int)(sizeof(x)/sizeof(*(x))))

bool get_text_input(std::string initial,std::string&out);
