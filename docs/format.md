# LFIF container format

LFIF version 1 stores multidimensional RGB images compressed with either the
wavelet transform or DCT. Wavelet is the default transform used by encoders;
DCT is an explicit alternative. Multi-byte integers are big-endian.

## Header

The payload starts at `header_size`. The base header has the following layout:

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 8 | Magic: `4c 46 49 46 0d 0a 1a 0a` |
| 8 | 1 | Major version, currently 1 |
| 9 | 1 | Minor version, currently 0 |
| 10 | 2 | Total header size in bytes |
| 12 | 4 | Flags |
| 16 | 1 | Dimension count |
| 17 | 1 | Channel count |
| 18 | 1 | Sample depth in bits |
| 19 | 1 | Sample format |
| 20 | 1 | Transform |
| 21 | 1 | Entropy codec |
| 22 | 1 | Color space |
| 23 | 1 | Number of discarded transform bits |
| 24 | 8 | Payload size in bytes |
| 32 | 8 | Horizontal disparity-compensation shift, signed two's complement |
| 40 | 8 | Vertical disparity-compensation shift, signed two's complement |
| 48 | 8 × dimensions | Image extents, in dimension order |
| varies | 8 × dimensions | Block extents, in dimension order |
| varies | `header_size` remainder | Extension bytes |

Flag bit 0 enables block prediction. Bit 1 says disparity compensation was
applied and makes the two shift fields meaningful. All other flag bits are zero
in version 1.0. Shift fields must be zero when bit 1 is clear.

The defined identifiers are:

| Field | Value | Meaning |
| --- | ---: | --- |
| Sample format | 1 | Unsigned integer |
| Transform | 1 | Wavelet |
| Transform | 2 | DCT |
| Entropy codec | 1 | CABAC |
| Color space | 1 | RGB |

## Payload profile

The payload is one opaque codec bitstream selected by the transform and entropy
identifiers. It has no nested magic, dimensions or legacy LFIF/LFWF header; all
codec parameters come from the container header. A container parser can locate,
validate, skip or extract the payload without implementing the codec. The
container major version also versions this payload profile. Changing its binary
syntax incompatibly therefore requires a new container major version.

The arithmetic codec is currently defined by the reference implementation and
is not presented as an external standardized codec. Independent container
parsers need only treat exactly `payload_size` bytes as the payload.

Version 1 supports three unsigned RGB channels with sample depths from 1 to 16
bits. Dimension and block counts match, every extent is nonzero, and all extent
products and block alignment calculations fit the implementation's `size_t`.
The number of discarded transform bits does not exceed the sample depth. The
payload contains exactly `payload_size` bytes.

## Version and extension rules

A reader rejects an unknown major version, unknown identifiers, unknown flag
bits, invalid base fields, and a header smaller than the base size implied by
its dimension count. A version 1 reader may accept a newer minor version when
all base fields and flags are understood. It skips bytes between the required
base fields and `header_size`, allowing later minor versions to add optional
metadata without moving the payload.

The payload length frames concatenated or embedded data and detects truncation
when the payload is consumed. Version 1 has no checksum: the container does not
claim to detect intentional modification or act as a security boundary.

The current reference codec implementation encodes and decodes dimensions 2
through 4. The dimension field and container validation are not tied to that
implementation limit, so a later codec can extend the supported range without a
container-version change.

## Legacy files

Version 1 does not read the earlier `LFIF-2D\n` and `LFIF-4D\n` containers.
Readers recognize those prefixes and reject them as unsupported legacy LFIF
rather than interpreting them as a malformed version 1 header. The old 2D file
used the wavelet implementation named LFWF internally, but its on-disk prefix
was still `LFIF-2D`; there was no separate versioned LFWF container.

Use the software at the `v1.0.0` or `v2.0.0` tag to decode those files before
converting their PPM output to this format. New encoders always write the
versioned eight-byte magic documented above.
