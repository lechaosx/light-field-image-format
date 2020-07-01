/**
* @file lfif_decoder.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions for decoding an image.
*/

#ifndef LFIF_DECODER_H
#define LFIF_DECODER_H

#include "block_compress_chain.h"
#include "block_decompress_chain.h"
#include "bitstream.h"
#include "colorspace.h"

#include <cstdint>
#include <istream>
#include <sstream>

/**
* @brief Base structure for decoding an image.
*/
template<size_t D>
struct LfifDecoder {
  uint8_t color_depth;    /**< @brief Number of bits per sample used by each decoded channel.*/

  std::array<uint64_t, D>     block_size;
  std::array<uint64_t, D + 1> img_dims; /**< @brief Dimensions of a decoded image + image count.*/

  std::array<uint64_t, D> img_dims_unaligned;
  std::array<uint64_t, D + 1> img_stride_unaligned;

  std::array<size_t, D>     block_dims; /**< @brief Dimensions of an encoded image in blocks. The multiple of all values should be equal to number of blocks in encoded image.*/
  std::array<size_t, D + 1> block_stride;

  bool use_huffman; /**< @brief Huffman Encoding was used when this is true.*/
  bool use_prediction;
  bool shift;

  std::array<int64_t, 2> shift_param;

  QuantTable<D>      quant_table         [2];    /**< @brief Quantization matrices for luma and chroma.*/
  TraversalTable<D>  traversal_table     [2];    /**< @brief Traversal matrices for luma and chroma.*/
  HuffmanDecoder     huffman_decoder     [2][2]; /**< @brief Huffman decoders for luma and chroma and also for the DC and AC coefficients.*/

  HuffmanDecoder    *huffman_decoders_ptr[3]; /**< @brief Pointer to Huffman decoders used by each channel.*/
  TraversalTable<D> *traversal_table_ptr [3]; /**< @brief Pointer to traversal matrix used by each channel.*/
  QuantTable<D>     *quant_table_ptr     [3]; /**< @brief Pointer to quantization matrix used by each channel.*/

  size_t amp_bits;   /**< @brief Number of bits sufficient to contain maximum DCT coefficient.*/
  size_t class_bits; /**< @brief Number of bits sufficient to contain number of bits of maximum DCT coefficient.*/

  DynamicBlock<INPUTTRIPLET,  D> current_block;   /**< @brief Buffer for caching the block of pixels before returning to client.*/
  DynamicBlock<RunLengthPair, D> runlength;       /**< @brief Buffer for caching the block of run-length pairs when decoding.*/
  DynamicBlock<QDATAUNIT,     D> quantized_block; /**< @brief Buffer for caching the block of quantized coefficients.*/
  DynamicBlock<DCTDATAUNIT,   D> dct_block;       /**< @brief Buffer for caching the block of DCT coefficients.*/
  DynamicBlock<INPUTUNIT,     D> output_block;    /**< @brief Buffer for caching the block of samples.*/
};

/**
* @brief Function which reads the image header from stream.
* @param dec The decoder structure.
* @param input The input stream.
* @return Zero if the header was succesfully read, nonzero else.
*/
template<size_t D>
int readHeader(LfifDecoder<D> &dec, std::istream &input) {
  std::string       magic_number     {};
  std::stringstream magic_number_cmp {};

  magic_number_cmp << "LFIF-" << D << "D";

  input >> magic_number;
  input.ignore();

  if (magic_number != magic_number_cmp.str()) {
    return -1;
  }

  std::string block_size {};
  for (size_t i = 0; i < D; i++) {
    input >> dec.block_size[i];
    input.ignore();
  }
  input.ignore();

  dec.color_depth = readValueFromStream<uint8_t>(input);

  for (size_t i = 0; i < D + 1; i++) {
    dec.img_dims[i] = readValueFromStream<uint64_t>(input);
  }

  for (size_t i = 0; i < 2; i++) {
    dec.quant_table[i] = readQuantFromStream<D>(dec.block_size, input);
  }

  dec.use_huffman    = readValueFromStream<uint8_t>(input);
  dec.use_prediction = readValueFromStream<uint8_t>(input);
  dec.shift          = readValueFromStream<uint8_t>(input);

  if (dec.shift) {
    for (size_t i = 0; i < 2; i++) {
      dec.shift_param[i] = readValueFromStream<int64_t>(input);
    }
  }

  if (dec.use_huffman) {
    for (size_t i = 0; i < 2; i++) {
      dec.traversal_table[i] = readTraversalFromStream(dec.block_size, input);
    }

    for (size_t y = 0; y < 2; y++) {
      for (size_t x = 0; x < 2; x++) {
        dec.huffman_decoder[y][x]
        . readFromStream(input);
      }
    }
  }

  return 0;
}

/**
* @brief Function which (re)initializes the decoder structure.
* @param dec The decoder structure.
*/
template<size_t D>
void initDecoder(LfifDecoder<D> &dec) {
  dec.current_block.resize(dec.block_size);
  dec.output_block.resize(dec.block_size);
  dec.dct_block.resize(dec.block_size);
  dec.quantized_block.resize(dec.block_size);
  dec.runlength.resize(dec.block_size);

  dec.img_stride_unaligned[0] = 1;
  dec.block_stride[0]         = 1;

  for (size_t i = 0; i < D; i++) {
    dec.block_dims[i]       = ceil(dec.img_dims[i] / static_cast<double>(dec.block_size[i]));
    dec.block_stride[i + 1] = dec.block_stride[i] * dec.block_dims[i];

    dec.img_dims_unaligned[i]       = dec.img_dims[i];
    dec.img_stride_unaligned[i + 1] = dec.img_stride_unaligned[i] * dec.img_dims_unaligned[i];
  }

  dec.huffman_decoders_ptr[0] = dec.huffman_decoder[0];
  dec.huffman_decoders_ptr[1] = dec.huffman_decoder[1];
  dec.huffman_decoders_ptr[2] = dec.huffman_decoder[1];

  dec.traversal_table_ptr[0]  = &dec.traversal_table[0];
  dec.traversal_table_ptr[1]  = &dec.traversal_table[1];
  dec.traversal_table_ptr[2]  = &dec.traversal_table[1];

  dec.quant_table_ptr[0]      = &dec.quant_table[0];
  dec.quant_table_ptr[1]      = &dec.quant_table[1];
  dec.quant_table_ptr[2]      = &dec.quant_table[1];

  dec.amp_bits = ceil(log2(get_stride<D>(dec.block_size))) + dec.color_depth - D - (D/2);
  dec.class_bits = RunLengthPair::classBits(dec.amp_bits);
}

/**
* @brief Function which decodes Huffman encoded image from stream.
* @param dec The decoder structure.
* @param input Input stream from which the image will be decoded.
* @param output Output callback function which will be returning pixels with signature void output(size_t index, INPUTTRIPLET value), where index is a pixel index in memory and value is said pixel.
*/
template<size_t D, typename F>
void decodeScanHuffman(LfifDecoder<D> &dec, std::istream &input, F &&pusher) {
  IBitstream bitstream       {};
  QDATAUNIT  previous_DC [3] {};

  bitstream.open(&input);

  for (size_t image = 0; image < dec.img_dims[D]; image++) {

    iterate_dimensions<D>(dec.block_dims, [&](const std::array<size_t, D> &block) {

      for (size_t channel = 0; channel < 3; channel++) {
        decodeHuffman_RUNLENGTH<D>(bitstream, dec.runlength, dec.huffman_decoders_ptr[channel], dec.class_bits);
        runLengthDecode<D>(dec.runlength, dec.quantized_block);
        detraverse<D>(dec.quantized_block, *dec.traversal_table_ptr[channel]);
        diffDecodeDC<D>(dec.quantized_block, previous_DC[channel]);
        dequantize<D>(dec.quantized_block, dec.dct_block, *dec.quant_table_ptr[channel]);
        inverseDiscreteCosineTransform<D>(dec.dct_block, dec.output_block);

        for (size_t i = 0; i < get_stride<D>(dec.block_size); i++) {
          dec.current_block[i][channel] = dec.output_block[i];
        }
      }

      auto outputF = [&](const auto &image_pos, const auto &value) {
        std::array<size_t, D + 1> whole_image_pos {};
        std::copy(std::begin(image_pos), std::end(image_pos), std::begin(whole_image_pos));
        whole_image_pos[D] = image;
        return pusher(whole_image_pos, value);
      };

      putBlock<D>(dec.block_size.data(), [&](const auto &pos) { return dec.current_block[pos]; }, block, dec.img_dims_unaligned, outputF);
    });
  }
}

/**
* @brief Function which decodes CABAC encoded image from stream.
* @param dec The decoder structure.
* @param input Input stream from which the image will be decoded.
* @param output Output callback function which will be returning pixels with signature void output(size_t index, INPUTTRIPLET value), where index is a pixel index in memory and value is said pixel.
*/
template<size_t D, typename IF, typename OF>
void decodeScanCABAC(LfifDecoder<D> &dec, std::istream &input, IF &&puller, OF &&pusher) {
  std::array<CABACContextsDIAGONAL<D>, 2> contexts         {CABACContextsDIAGONAL<D>(dec.block_size), CABACContextsDIAGONAL<D>(dec.block_size)};
  std::vector<std::vector<size_t>>        scan_table       {};
  IBitstream                              bitstream        {};
  CABACDecoder                            cabac            {};
  size_t                                  threshold        {};

  DynamicBlock<INPUTUNIT, D>              prediction_block {};

  if (dec.use_prediction) {
    prediction_block.resize(dec.block_size);
  }

  threshold = num_diagonals<D>(dec.block_size) / 2;

  scan_table.resize(num_diagonals<D>(dec.block_size));
  iterate_dimensions<D>(dec.block_size, [&](const auto &pos) {
    size_t diagonal {};
    for (size_t i = 0; i < D; i++) {
      diagonal += pos[i];
    }

    scan_table[diagonal].push_back(make_index(dec.block_size, pos));
  });

  bitstream.open(&input);
  cabac.init(bitstream);

  for (size_t image = 0; image < dec.img_dims[D]; image++) {
    auto inputF = [&](const auto &image_pos) {
      std::array<size_t, D + 1> whole_image_pos {};
      std::copy(std::begin(image_pos), std::end(image_pos), std::begin(whole_image_pos));
      whole_image_pos[D] = image;
      return puller(whole_image_pos);
    };

    auto outputF = [&](const auto &image_pos, const auto &value) {
      std::array<size_t, D + 1> whole_image_pos {};
      std::copy(std::begin(image_pos), std::end(image_pos), std::begin(whole_image_pos));
      whole_image_pos[D] = image;
      return pusher(whole_image_pos, value);
    };

    iterate_dimensions<D>(dec.block_dims, [&](const std::array<size_t, D> &block) {
      bool any_block_available {};
      bool previous_block_available[D] {};
      for (size_t i { 0 }; i < D; i++) {
        if (block[i]) {
          previous_block_available[i] = true;
          any_block_available = true;
        }
        else {
          previous_block_available[i] = false;
        }
      }

      auto predInputF = [&](std::array<int64_t, D> &block_pos, size_t channel) -> INPUTUNIT {
        if (!any_block_available) {
          return 0;
        }

        //resi pokud se prediktor chce divat na vzorky, ktere jeste nebyly komprimovane
        for (size_t i = 1; i <= D; i++) {
          size_t idx = D - i;

          if (block_pos[idx] < 0) {
            if (previous_block_available[idx]) {
              break;
            }
          }
          else if (block_pos[idx] >= static_cast<int64_t>(dec.block_size[idx])) {
            block_pos[idx] = dec.block_size[idx] - 1;
          }
        }

        //resi pokud se prediktor chce divat na vzorky mimo obrazek v zapornych souradnicich
        int64_t min_pos = std::numeric_limits<int64_t>::max();
        for (size_t i = 0; i < D; i++) {
          if (previous_block_available[i]) {
            if (block_pos[i] < min_pos) {
              min_pos = block_pos[i];
            }
          }
          else if (block_pos[i] < 0) {
            block_pos[i] = 0;
          }
        }

        for (size_t i = 0; i < D; i++) {
          if (previous_block_available[i]) {
            block_pos[i] -= min_pos + 1;
          }
        }

        std::array<size_t, D> image_pos {};
        for (size_t i = 0; i < D; i++) {
          image_pos[i] = block[i] * dec.block_size[i] + block_pos[i];

          //resi pokud se prediktor chce divat na vzorky mimo obrazek v kladnych souradnicich
          image_pos[i] = std::clamp<size_t>(image_pos[i], 0, dec.img_dims_unaligned[i] - 1);
        }

        return inputF(image_pos)[channel];
      };

      uint64_t prediction_type {};
      if (dec.use_prediction) {
        decodePredictionType<D>(prediction_type, cabac, contexts[0]);
      }

      for (size_t channel = 0; channel < 3; channel++) {
        decodeCABAC_DIAGONAL<D>(dec.quantized_block, cabac, contexts[channel != 0], threshold, scan_table);
        dequantize<D>(dec.quantized_block, dec.dct_block, *dec.quant_table_ptr[channel]);
        inverseDiscreteCosineTransform<D>(dec.dct_block, dec.output_block);

        if (dec.use_prediction) {
          predict<D>(prediction_block, prediction_type, [&](std::array<int64_t, D> &block_pos) { return predInputF(block_pos, channel); });
          disusePrediction<D>(dec.output_block, prediction_block);
        }

        for (size_t i = 0; i < dec.output_block.stride(D); i++) {
          dec.current_block[i][channel] = dec.output_block[i];
        }
      }

      putBlock<D>(dec.block_size.data(), [&](const auto &pos) { return dec.current_block[pos]; }, block, dec.img_dims_unaligned, outputF);
    });

  }

  cabac.terminate();
}
#endif
