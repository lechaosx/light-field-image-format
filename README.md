# Light Field Image Format
Light Field Image Format (LFIF) is an implementation of a lossy light field image compression method which compresses the light field as a four-dimensional volume to exploit the correlation between adjacent views. The method is based on JPEG baseline encoder. The LFIF library can handle a variety of data and is able to compress a light field as a series of (two or three)-dimensional images or as one four-dimensional image. This project also contains a minimalistic PPM library for reading and writing PPM files and set of tools for light field image compression and decompression.
Evaluation of this encoder was performed in the paper [1]. The method is described in the Excel@FIT paper [2] and in the separate bachelor's thesis [3].

## Compile
The project uses C++20, CMake and Nix. Enter the reproducible development environment and build the release preset with:

    nix develop
    cmake --preset release
    cmake --build --preset release
    ctest --preset release

The ``full`` preset also builds and tests the codec comparison tools:

    cmake --preset full
    cmake --build --preset full
    ctest --preset full

The Nix flake builds the main package with ``nix build``; ``nix build .#full`` also builds and tests all comparison tools. It provides their dependencies directly: xvc is built from its pinned upstream repository, and ``mozbench`` links Mozilla mozjpeg.

## Usage
The tools are able to compress a light field image which exists as a set of ppm images representing individual views.
The image files must be sorted in row or column order and the view indices must be part of file names.
To compress a light field image, use the compression tool like this:

    ./build/cmake/release/tools/lfif compress path/to/input.ppm path/to/output.lfif

For a four-dimensional light field with a 15 by 15 view grid, specify the two view dimensions:

    ./build/cmake/release/tools/lfif compress 'path/to/input-###.ppm' path/to/output.lfif --shape 15x15 --block 8x8x4x4

The ``#`` characters are replaced with zero-padded sequential view numbers. The number of input images must match the product of ``--shape``, and all PPM properties must match. Omitting ``--shape`` compresses one two-dimensional PPM. One view dimension produces a three-dimensional image and two view dimensions produce a four-dimensional image.

Wavelet compression is the default. Use ``--transform dct`` to select DCT explicitly, ``--discarded-bits N`` for lossy transform-bit removal and ``--predict`` to enable intra prediction. If ``--block`` is omitted, each block extent is the smaller of 8 and the corresponding image extent.

The decompression command writes one PPM for a two-dimensional image, or expands a mask for the higher-dimensional view axes:

    ./build/cmake/release/tools/lfif decompress path/to/input.lfif 'path/to/output-###.ppm'

Container metadata can be inspected without decoding the payload:

    ./build/cmake/release/tools/lfif inspect path/to/input.lfif

The versioned binary container is documented in [docs/format.md](docs/format.md).

## License
See [LICENSE](https://github.com/xdlaba02/light-field-image-format/blob/master/LICENSE) for license and copyright information.

## References
[1] Barina, D.; Chlubna, T.; Solony, M.; aj.: Evaluation of 4D Light Field Compression Methods. In International Conference in Central Europe on Computer Graphics, Visualization and Computer Vision (WSCG), 2019.

[2] Dlabaja, D.: 4D-DCT Based Light Field Image Compression. In Excel\@FIT, 2019.

[3] Dlabaja, D.: Ztrátová komprese plenoptických fotografií. Bachelor's thesis, Brno University of Technology, Faculty of Information Technology, 2019. https://www.vut.cz/studenti/zav-prace/detail/122096
