# Unboxing Command-line tool

See [doc/SIMPLE.md](doc/SIMPLE.md) for a guide on how to decode frames. This project is a decoder for folders of image files containing scanned piqlFilm frames.

## Project organization

- `dep` - Dependencies (as git submodules):
  - `afs` - Archival File System Library, used to parse control frame and table
    of contents.
  - `ivm_testdata` - Not required by the project, but included for a sample
    folder of piqlFilm PNG images for testing.
  - `rpmalloc` - Only used for release builds to improve the performance of
    `malloc()` and friends (~3% from my tests).
  - `stb` - `stb_image.h` used by unbox for image file reading.
    `stb_image_write.h` used by raw_file_to_png for image file writing.
  - `unboxing` - Decoding library for the boxing barcode format.
- `dev` - Auxillary development tools, code and files.
  - `build` - Auxillary code for `build.c`.
- `out` - Output files of various kinds.
  - `exe` - Built binaries by the project.
- `src` - Project source code.
- `build.c` - Project build script.

## Building

```sh
./build.c # or
cc -o build build.c && ./build
./build.c -DRELEASE
```
<!--
## Preliminary plan for reading

1. CLI arg #1: folder input
2. Read folder, get all frame filenames
3. Start reading frame 1
4. Unbox as control frame v7
   1. if fails, unbox as control frame v6, then try v5, then try v4, ...
   2. return first succeeding parse
   3. If all fail, repeat step 4 with the last frame, counting down to
      second-last, third-last, etc if they fail
5. Get tocs
6. For each toc:
   1. Read all frames in toc
   2. If metadata frame ID is wrong, try adjusting by reading the frame at the
      relevant offset, if ok update global offsets
      1. If offsets fail, scan every frame and update ID's based on metadata
      2. Repeat this logic for every frame reading
   3. Parse toc
   4. If reading or parsing fails, continue, otherwise break.
7. For each file in toc:
   1. Read all frames in file and write out to arg #2 (folder) as you read
   2. If any read or write fails, skip the file and start next file

### notes

- Cache the last read frame between files, usually files overlap, and we don't
  want to unbox the same frame twice
-->