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

#include <cstdint>
#include <istream>
#include <sstream>

/**
* @brief Base structure for decoding an image.
*/
template<size_t BS, size_t D>
struct LfifDecoder {
  uint8_t color_depth;    /**< @brief Number of bits per sample used by each decoded channel.*/
  uint64_t img_dims[D+1]; /**< @brief Dimensions of a decoded image + image count.*/

  bool use_huffman; /**< @brief Huffman Encoding was used when this is true.*/

  QuantTable<BS, D>       quant_table         [2];    /**< @brief Quantization matrices for luma and chroma.*/
  TraversalTable<BS, D>   traversal_table     [2];    /**< @brief Traversal matrices for luma and chroma.*/
  HuffmanDecoder          huffman_decoder     [2][2]; /**< @brief Huffman decoders for luma and chroma and also for the DC and AC coefficients.*/

  HuffmanDecoder         *huffman_decoders_ptr[3]; /**< @brief Pointer to Huffman decoders used by each channel.*/
  TraversalTable<BS, D>  *traversal_table_ptr [3]; /**< @brief Pointer to traversal matrix used by each channel.*/
  QuantTable<BS, D>      *quant_table_ptr     [3]; /**< @brief Pointer to quantization matrix used by each channel.*/

  size_t block_dims[D]; /**< @brief Dimensions of an encoded image in blocks. The multiple of all values should be equal to number of blocks in encoded image.*/
  size_t blocks_cnt; /**< @brief Number of blocks in the decoded image.*/
  size_t pixels_cnt; /**< @brief Number of pixels in the decoded image.*/

  size_t amp_bits;   /**< @brief Number of bits sufficient to contain maximum DCT coefficient.*/
  size_t class_bits; /**< @brief Number of bits sufficient to contain number of bits of maximum DCT coefficient.*/

  Block<INPUTTRIPLET,  BS, D> current_block;   /**< @brief Buffer for caching the block of pixels before returning to client.*/
  Block<RunLengthPair, BS, D> runlength;       /**< @brief Buffer for caching the block of run-length pairs when decoding.*/
  Block<QDATAUNIT,     BS, D> quantized_block; /**< @brief Buffer for caching the block of quantized coefficients.*/
  Block<DCTDATAUNIT,   BS, D> dct_block;       /**< @brief Buffer for caching the block of DCT coefficients.*/
  Block<INPUTUNIT,     BS, D> output_block;    /**< @brief Buffer for caching the block of samples.*/
};

/**
* @brief Function which reads the image header from stream.
* @param dec The decoder structure.
* @param input The input stream.
* @return Zero if the header was succesfully read, nonzero else.
*/
template<size_t BS, size_t D>
int readHeader(LfifDecoder<BS, D> &dec, std::istream &input) {
  std::string       magic_number     {};
  std::stringstream magic_number_cmp {};

  std::string       block_size     {};
  std::stringstream block_size_cmp {};

  magic_number_cmp << "LFIF-" << D << "D";
  block_size_cmp   << BS;

  input >> magic_number;
  input.ignore();
  input >> block_size;
  input.ignore();

  if (magic_number != magic_number_cmp.str()) {
    return -1;
  }

  if (block_size != block_size_cmp.str()) {
    return -2;
  }

  dec.color_depth = readValueFromStream<uint8_t>(input);

  for (size_t i = 0; i < D + 1; i++) {
    dec.img_dims[i] = readValueFromStream<uint64_t>(input);
  }

  for (size_t i = 0; i < 2; i++) {
    dec.quant_table[i] = readFromStream<BS, D>(input);
  }

  dec.use_huffman = readValueFromStream<uint8_t>(input);

  if (dec.use_huffman) {
    for (size_t i = 0; i < 2; i++) {
      dec.traversal_table[i]
      . readFromStream(input);
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
template<size_t BS, size_t D>
void initDecoder(LfifDecoder<BS, D> &dec) {
  dec.huffman_decoders_ptr[0] = dec.huffman_decoder[0];
  dec.huffman_decoders_ptr[1] = dec.huffman_decoder[1];
  dec.huffman_decoders_ptr[2] = dec.huffman_decoder[1];

  dec.traversal_table_ptr[0]  = &dec.traversal_table[0];
  dec.traversal_table_ptr[1]  = &dec.traversal_table[1];
  dec.traversal_table_ptr[2]  = &dec.traversal_table[1];

  dec.quant_table_ptr[0]      = &dec.quant_table[0];
  dec.quant_table_ptr[1]      = &dec.quant_table[1];
  dec.quant_table_ptr[2]      = &dec.quant_table[1];

  dec.blocks_cnt = 1;
  dec.pixels_cnt = 1;

  for (size_t i = 0; i < D; i++) {
    dec.block_dims[i] = ceil(dec.img_dims[i] / static_cast<double>(BS));
    dec.blocks_cnt   *= dec.block_dims[i];
    dec.pixels_cnt   *= dec.img_dims[i];
  }

  dec.amp_bits = ceil(log2(constpow(BS, D))) + dec.color_depth - D - (D/2);
  dec.class_bits = RunLengthPair::classBits(dec.amp_bits);
}

/**
* @brief Function which decodes Huffman encoded image from stream.
* @param dec The decoder structure.
* @param input Input stream from which the image will be decoded.
* @param output Output callback function which will be returning pixels with signature void output(size_t index, INPUTTRIPLET value), where index is a pixel index in memory and value is said pixel.
*/
template<size_t BS, size_t D, typename F>
void decodeScanHuffman(LfifDecoder<BS, D> &dec, std::istream &input, F &&output) {
  IBitstream bitstream       {};
  QDATAUNIT  previous_DC [3] {};

  bitstream.open(&input);

  auto inputF = [&](size_t index) -> const auto & {
    return dec.current_block[index];
  };

  for (size_t img = 0; img < dec.img_dims[D]; img++) {
    auto outputF = [&](size_t index, const auto &value) {
      output(img * dec.pixels_cnt + index, value);
    };

    for (size_t block = 0; block < dec.blocks_cnt; block++) {
      for (size_t channel = 0; channel < 3; channel++) {
               decodeHuffman_RUNLENGTH<BS, D>(bitstream,            dec.runlength, dec.huffman_decoders_ptr[channel], dec.class_bits);
                       runLengthDecode<BS, D>(dec.runlength,        dec.quantized_block);
                            detraverse<BS, D>(dec.quantized_block, *dec.traversal_table_ptr[channel]);
                          diffDecodeDC<BS, D>(dec.quantized_block,  previous_DC[channel]);
                            dequantize<BS, D>(dec.quantized_block,  dec.dct_block, *dec.quant_table_ptr[channel]);
        inverseDiscreteCosineTransform<BS, D>(dec.dct_block,        dec.output_block);

        for (size_t i = 0; i < constpow(BS, D); i++) {
          dec.current_block[i][channel] = dec.output_block[i];
        }
      }

      putBlock<BS, D>(inputF, block, dec.img_dims, outputF);
    }
  }
}

/**
* @brief Function which decodes CABAC encoded image from stream.
* @param dec The decoder structure.
* @param input Input stream from which the image will be decoded.
* @param output Output callback function which will be returning pixels with signature void output(size_t index, INPUTTRIPLET value), where index is a pixel index in memory and value is said pixel.
*/
template<size_t BS, size_t D, typename F>
void decodeScanCABAC(LfifDecoder<BS, D> &dec, std::istream &input, F &&output) {
  std::array<std::vector<size_t>,          D * (BS - 1) + 1> scan_table  {};
  std::array<CABACContextsDIAGONAL<BS, D>, 2>                contexts    {};
  std::array<std::vector<INPUTUNIT>, 3>                      decoded     {};
  IBitstream                                                 bitstream   {};
  CABACDecoder                                               cabac       {};
  size_t                                                     threshold   {};
  Block<INPUTUNIT,     BS, D>                                prediction_block {};

  threshold = (D * (BS - 1) + 1) / 2;

  for (size_t i = 0; i < constpow(BS, D); i++) {
    size_t diagonal { 0 };
    for (size_t j = i; j; j /= BS) {
      diagonal += j % BS;
    }

    scan_table[diagonal].push_back(i);
  }

  for (size_t i = 0; i < 3; i++) {
    decoded[i].resize(dec.blocks_cnt * constpow(BS, D));
  }

  size_t aligned_dims[D] {};

  for (size_t d = 0; d < D; d++) {
    aligned_dims[d] = ceil(dec.block_dims[d] * BS);
  }

  bitstream.open(&input);
  cabac.init(bitstream);

  auto inputF = [&](size_t index) -> const auto & {
    return dec.current_block[index];
  };

  auto inputFP = [&](size_t index) -> const auto & {
    return dec.output_block[index];
  };

  for (size_t img = 0; img < dec.img_dims[D]; img++) {
    auto outputF = [&](size_t index, const auto &value) {
      output(img * dec.pixels_cnt + index, value);
    };

    for (size_t block = 0; block < dec.blocks_cnt; block++) {
      uint64_t prediction_type {};

      for (size_t channel = 0; channel < 3; channel++) {
        auto outputFP = [&](size_t index, const auto &value) {
          decoded[channel][index] = value;
        };

        if (channel == 0) {
          decodePredictionType(prediction_type, cabac, contexts[0]);
        }
                               predict<BS, D>(prediction_block,    dec.block_dims,       decoded[channel], block, prediction_type);
                  decodeCABAC_DIAGONAL<BS, D>(dec.quantized_block, cabac, contexts[channel != 0], threshold, scan_table);
                            dequantize<BS, D>(dec.quantized_block,  dec.dct_block, *dec.quant_table_ptr[channel]);
        inverseDiscreteCosineTransform<BS, D>(dec.dct_block,        dec.output_block);
                      disusePrediction<BS, D>(dec.output_block,     prediction_block);
                              putBlock<BS, D>(inputFP,              block,                aligned_dims,           outputFP);

        for (size_t i = 0; i < constpow(BS, D); i++) {
          dec.current_block[i][channel] = dec.output_block[i];
        }
      }

      putBlock<BS, D>(inputF, block, dec.img_dims, outputF);
    }
  }

  cabac.terminate();
}

template<size_t BS, size_t D, typename F>
void decodeScanCABAC_NOPREDICT(LfifDecoder<BS, D> &dec, std::istream &input, F &&output) {
  std::array<std::vector<size_t>,          D * (BS - 1) + 1> scan_table  {};
  std::array<CABACContextsDIAGONAL<BS, D>, 2>                contexts    {};
  std::array<std::vector<INPUTUNIT>, 3>                      decoded     {};
  IBitstream                                                 bitstream   {};
  CABACDecoder                                               cabac       {};
  size_t                                                     threshold   {};

  threshold = (D * (BS - 1) + 1) / 2;

  for (size_t i = 0; i < constpow(BS, D); i++) {
    size_t diagonal { 0 };
    for (size_t j = i; j; j /= BS) {
      diagonal += j % BS;
    }

    scan_table[diagonal].push_back(i);
  }

  bitstream.open(&input);
  cabac.init(bitstream);

  auto inputF = [&](size_t index) -> const auto & {
    return dec.current_block[index];
  };

  for (size_t img = 0; img < dec.img_dims[D]; img++) {
    auto outputF = [&](size_t index, const auto &value) {
      output(img * dec.pixels_cnt + index, value);
    };

    for (size_t block = 0; block < dec.blocks_cnt; block++) {
      for (size_t channel = 0; channel < 3; channel++) {

                  decodeCABAC_DIAGONAL<BS, D>(dec.quantized_block, cabac, contexts[channel != 0], threshold, scan_table);
                            dequantize<BS, D>(dec.quantized_block,  dec.dct_block, *dec.quant_table_ptr[channel]);
        inverseDiscreteCosineTransform<BS, D>(dec.dct_block,        dec.output_block);

        for (size_t i = 0; i < constpow(BS, D); i++) {
          dec.current_block[i][channel] = dec.output_block[i];
        }
      }

      putBlock<BS, D>(inputF, block, dec.img_dims, outputF);
    }
  }

  cabac.terminate();
}

#endif
