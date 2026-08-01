#pragma once

#include <ppm.h>

#include <string_view>
#include <vector>

std::vector<PPM> mapPPMs(std::string_view input_file_mask);
