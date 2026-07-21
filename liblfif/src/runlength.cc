#include <components/runlength.h>
#include <components/bitstream.h>

void rl_encode(const RunLengthPair &p, const HuffmanEncoder &encoder, OBitstream &bitstream, std::ostream &stream, size_t class_bits) {
  huff_encode(encoder, rl_huffman_symbol(p, class_bits), bitstream, stream);

  RLAMPUNIT amp = p.amplitude;
  if (amp < 0) {
    amp = -amp;
    amp = ~amp;
  }

  HuffmanClass amp_class = rl_huffman_class(p);
  for (int16_t i = amp_class - 1; i >= 0; i--) {
    bitstream.writeBit(stream, (amp & (1 << i)) >> i);
  }
}

void rl_decode(RunLengthPair &p, const HuffmanDecoder &decoder, IBitstream &bitstream, std::istream &stream, size_t class_bits) {
  HuffmanSymbol huffman_symbol = huff_decode(decoder, bitstream, stream);
  HuffmanClass  amp_class      = huffman_symbol & (~(static_cast<HuffmanSymbol>(-1) << class_bits));
  p.zeroes    = huffman_symbol >> class_bits;
  p.amplitude = 0;

  if (amp_class != 0) {
    for (HuffmanClass i = 0; i < amp_class; i++) {
      p.amplitude <<= 1;
      p.amplitude |= bitstream.readBit(stream);
    }

    if (p.amplitude < (1 << (amp_class - 1))) {
      p.amplitude |= static_cast<uint64_t>(-1) << amp_class;
      p.amplitude = ~p.amplitude;
      p.amplitude = -p.amplitude;
    }
  }
}
