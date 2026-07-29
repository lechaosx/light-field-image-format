/**
* @file plenoppm.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Module for reading and writin ppm plenoptic images.
*/

#pragma once

#include <ppm.h>

#include <string_view>
#include <vector>

std::vector<PPM> mapPPMs(std::string_view input_file_mask);
