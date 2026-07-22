#include "components/runlength.h"
#include "components/bitstream.h"

void RunLengthPair::huffmanEncodeToStream(const HuffmanEncoder &encoder, OBitstream &stream, size_t class_bits) const {
  HuffmanClass amp_class {};
  RLAMPUNIT    amp       {};

  encoder.encodeSymbolToStream(huffmanSymbol(class_bits), stream);

  amp = amplitude;
  if (amp < 0) {
    amp = -amp;
    amp = ~amp;
  }

  amp_class = huffmanClass();
  for (int16_t i = amp_class - 1; i >= 0; i--) {
    stream.writeBit((amp & (1 << i)) >> i);
  }
}

void RunLengthPair::huffmanDecodeFromStream(const HuffmanDecoder &decoder, IBitstream &stream, size_t class_bits) {
  HuffmanClass  amp_class      {};
  HuffmanSymbol huffman_symbol {};

  huffman_symbol  = decoder.decodeSymbolFromStream(stream);
  amp_class       = huffman_symbol & (~(static_cast<HuffmanSymbol>(-1) << class_bits));
  zeroes          = huffman_symbol >> class_bits;
  amplitude       = 0;

  if (amp_class != 0) {
    for (HuffmanClass i = 0; i < amp_class; i++) {
      amplitude <<= 1;
      amplitude |= stream.readBit();
    }

    if (amplitude < (1 << (amp_class - 1))) {
      amplitude |= static_cast<uint64_t>(-1) << amp_class;
      amplitude = ~amplitude;
      amplitude = -amplitude;
    }
  }
}
