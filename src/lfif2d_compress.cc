/*******************************************************\
* SOUBOR: lfif2d_encoder.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 16. 12. 2018
\*******************************************************/

#include "lfif_encoder.h"
#include "ppm.h"

#include <getopt.h>

#include <iostream>
#include <bitset>
#include <iomanip>
#include <sstream>

using namespace std;

void print_usage(const char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <file-mask> -o <file> -q <quality>" << endl;
}

int main(int argc, char *argv[]) {
  char *input_file_mask {nullptr};
  char *output_file_name    {nullptr};
  char *arg_quality         {nullptr};

  /*******************************************************\
  * Argument parsing
  \*******************************************************/
  char opt;
  while ((opt = getopt(argc, argv, "i:o:q:")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_mask) {
          input_file_mask = optarg;
          continue;
        }
        break;

      case 'o':
        if (!output_file_name) {
          output_file_name = optarg;
          continue;
        }
        break;

      case 'q':
        if (!arg_quality) {
          arg_quality = optarg;
          continue;
        }
        break;

      default:
        break;
    }

    print_usage(argv[0]);
    return -1;
  }

  if ((!input_file_mask) || (!output_file_name) || (!arg_quality)) {
    print_usage(argv[0]);
    return -1;
  }

  string input_file_name {input_file_mask};
  uint8_t quality        {};

  int tmp_quality = atoi(arg_quality);
  if ((tmp_quality < 1) || (tmp_quality > 100)) {
    print_usage(argv[0]);
    return -2;
  }
  else {
    quality = tmp_quality;
  }

  vector<size_t> mask_indexes {};

  for (size_t i = 0; input_file_mask[i] != '\0'; i++) {
    if (input_file_mask[i] == '#') {
      mask_indexes.push_back(i);
    }
  }

  size_t image_count {};

  uint64_t width  {};
  uint64_t height {};

  vector<RGBData> rgb_data {};

  for (size_t image = 0; image < pow(10, mask_indexes.size()); image++) {
    stringstream image_number {};
    image_number << setw(mask_indexes.size()) << setfill('0') << to_string(image);

    for (size_t index = 0; index < mask_indexes.size(); index++) {
      input_file_name[mask_indexes[index]] = image_number.str()[index];
    }

    ifstream input(input_file_name);
    if (input.fail()) {
      continue;
    }

    image_count++;

    uint64_t image_width {};
    uint64_t image_height {};

    rgb_data.emplace_back();
    if (!readPPM(input, image_width, image_height, rgb_data.back())) {
      cerr << "ERROR: BAD PPM" << endl;
      return -3;
    }

    if (width && height) {
      if ((image_width != width) || (image_height != height)) {
        cerr << "ERROR: WIDTHS NOT SAME" << endl;
        return -4;
      }
    }

    width = image_width;
    height = image_height;
  }

  if (static_cast<uint64_t>(sqrt(image_count) * sqrt(image_count)) != image_count) {
    cerr << "ERROR: NOT SQUARE" << endl;
    return -5;
  }

  /**********************/

  size_t blocks_cnt = ceil(width/8.) * ceil(height/8.);
  Dimensions<2> dims{width, height};
  QuantTable<2> quant_table = scaleQuantTable<2>(baseQuantTable<2>(), quality);
  RefereceBlock<2> reference_block {};

  auto blockize = [&](const YCbCrData &input, vector<YCbCrDataBlock<2>> &output){
    size_t prev_size = output.size();
    output.resize(prev_size + blocks_cnt);
    convertToBlocks<2>([&](size_t index){ return input[index]; },  dims.data(), [&](size_t block_index, size_t pixel_index) -> YCbCrDataUnit &{ return output[prev_size + block_index][pixel_index]; });
  };

  auto quantize = [&](const vector<YCbCrDataBlock<2>> &input) {
    return quantizeBlocks<2>(input, quant_table);
  };

  vector<YCbCrDataBlock<2>> blocks_Y  {};
  vector<YCbCrDataBlock<2>> blocks_Cb {};
  vector<YCbCrDataBlock<2>> blocks_Cr {};

  for (size_t i = 0; i < image_count; i++) {
    blockize(shiftData(convertRGB(rgb_data[i], RGBtoY)), blocks_Y);
    blockize(shiftData(convertRGB(rgb_data[i], RGBtoCb)), blocks_Cb);
    blockize(shiftData(convertRGB(rgb_data[i], RGBtoCr)), blocks_Cr);
  }

  auto quantized_Y  = quantize(transformBlocks<2>(blocks_Y));
  auto quantized_Cb = quantize(transformBlocks<2>(blocks_Cb));
  auto quantized_Cr = quantize(transformBlocks<2>(blocks_Cr));

  getReference<2>(quantized_Y,  reference_block);
  getReference<2>(quantized_Cb, reference_block);
  getReference<2>(quantized_Cr, reference_block);

  TraversalTable<2> traversal_table = constructTraversalTableByReference<2>(reference_block);

  HuffmanWeights weights_luma_AC   {};
  HuffmanWeights weights_luma_DC   {};

  HuffmanWeights weights_chroma_AC {};
  HuffmanWeights weights_chroma_DC {};

  RunLengthEncodedImage runlength_Y  = diffEncodePairs(runLenghtEncodeBlocks<2>(traverseBlocks<2>(quantized_Y,  traversal_table)));
  RunLengthEncodedImage runlength_Cb = diffEncodePairs(runLenghtEncodeBlocks<2>(traverseBlocks<2>(quantized_Cb, traversal_table)));
  RunLengthEncodedImage runlength_Cr = diffEncodePairs(runLenghtEncodeBlocks<2>(traverseBlocks<2>(quantized_Cr, traversal_table)));

  huffmanGetWeightsAC(runlength_Y,  weights_luma_AC);
  huffmanGetWeightsDC(runlength_Y,  weights_luma_DC);

  huffmanGetWeightsAC(runlength_Cb, weights_chroma_AC);
  huffmanGetWeightsAC(runlength_Cr, weights_chroma_AC);
  huffmanGetWeightsDC(runlength_Cb, weights_chroma_DC);
  huffmanGetWeightsDC(runlength_Cr, weights_chroma_DC);

  HuffmanCodelengths codelengths_luma_DC   = generateHuffmanCodelengths(weights_luma_DC);
  HuffmanCodelengths codelengths_luma_AC   = generateHuffmanCodelengths(weights_luma_AC);

  HuffmanCodelengths codelengths_chroma_DC = generateHuffmanCodelengths(weights_chroma_DC);
  HuffmanCodelengths codelengths_chroma_AC = generateHuffmanCodelengths(weights_chroma_AC);

  /*******************************/

  ofstream output(output_file_name);
  if (output.fail()) {
    return -6;
  }

  output.write("LFIF-2D\n", 8);

  uint64_t raw_width  = toBigEndian(width);
  uint64_t raw_height = toBigEndian(height);
  uint64_t raw_count = toBigEndian(image_count);

  output.write(reinterpret_cast<char *>(&raw_width),  sizeof(raw_width));
  output.write(reinterpret_cast<char *>(&raw_height), sizeof(raw_height));
  output.write(reinterpret_cast<char *>(&raw_count), sizeof(raw_count));

  output.write(reinterpret_cast<const char *>(quant_table.data()), quant_table.size());
  output.write(reinterpret_cast<const char *>(traversal_table.data()), traversal_table.size() * sizeof(traversal_table[0]));

  writeHuffmanTable(codelengths_luma_DC, output);
  writeHuffmanTable(codelengths_luma_AC, output);
  writeHuffmanTable(codelengths_chroma_DC, output);
  writeHuffmanTable(codelengths_chroma_AC, output);

  OBitstream bitstream(output);

  HuffmanMap huffmap_luma_DC   = generateHuffmanMap(codelengths_luma_DC);
  HuffmanMap huffmap_luma_AC   = generateHuffmanMap(codelengths_luma_AC);
  HuffmanMap huffmap_chroma_DC = generateHuffmanMap(codelengths_chroma_DC);
  HuffmanMap huffmap_chroma_AC = generateHuffmanMap(codelengths_chroma_AC);

  for (size_t i = 0; i < blocks_cnt * image_count; i++) {
    encodeOnePair(runlength_Y[i][0], huffmap_luma_DC, bitstream);
    for (size_t j = 1; j < runlength_Y[i].size(); j++) {
      encodeOnePair(runlength_Y[i][j], huffmap_luma_AC, bitstream);
    }

    encodeOnePair(runlength_Cb[i][0], huffmap_chroma_DC, bitstream);
    for (size_t j = 1; j < runlength_Cb[i].size(); j++) {
      encodeOnePair(runlength_Cb[i][j], huffmap_chroma_AC, bitstream);
    }

    encodeOnePair(runlength_Cr[i][0], huffmap_chroma_DC, bitstream);
    for (size_t j = 1; j < runlength_Cr[i].size(); j++) {
      encodeOnePair(runlength_Cr[i][j], huffmap_chroma_AC, bitstream);
    }
  }

  bitstream.flush();

  return 0;
}
