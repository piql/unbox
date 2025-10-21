# Data Decoding Description - How to decode data from piqlFilm - detailed explanation

This explanation will focus mainly on usage of the unboxing library in order to
decode a full piqlFilm. In the future it may also cover building the library and
project setup, but for now it will only focus on usage in code.

For simplicity and brevity, error checking and resource cleanup will be omitted.
For a more thorough deep-dive see the unbox command-line tool
[source code](../src/main.c).

All code snippets are in C99.

<!-- FUTURE: Section on build setup here? -->

## Prerequisites for using the unboxing library - logging functions

The unboxing library expects 2 predefined logging functions to exist
(`boxing_log` and `boxing_log_args`), so your program should define them and
make them available to the library (Meaning depending on your build setup you
may need to include `-rdynamic` or similar flags when compiling your
application).

The library will call these functions during operation and report back some
handy debugging information, so it's quite useful to look at logs if you are
having trouble decoding. For this guide we will provide a simple implementation
that just logs out everything to `stderr`.

A simple implementation of these logging functions could be the following:

```c
#include <stdarg.h>
#include <stdio.h>

static const char *const restrict log_level_str[] = { "INFO", "WARNING", "ERROR", "FATAL", "DEBUG" };

void boxing_log(const int log_level, const char *const restrict str) {
  fprintf(stderr, "%-7s: %s\n", log_level_str[log_level], str);
}

void boxing_log_args(const int log_level, const char *const restrict fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "%-7s: ", log_level_str[log_level]);
  vfprintf(stderr, fmt, args);
  fputc('\n', stderr);
  va_end(args);
}
```

## Preliminary information about data types

These are the major datatypes you need to know about to initialize the library
and perform unboxing (decoding) of scanned frames.

- `config_structure` is the format used to store boxing formats / configurations
  in code source files, meaning the machine-readable description of how a
  specific type of digital frame is encoded (including all error correction,
  synchronization points, striping, etc.).
- `boxing_config` is the structure used to store boxing formats in-memory.
- `boxing_unboxer_parameters`is the structure used to store the parameters
  (options) used to create an unboxer instance.
- `boxing_unboxer` is the handle to an instantiated unboxer object, which is
  used to decode frames. It is configured for a specific type of frames, exists
  in-memory, and holds state across decodings in the case of striped frames.
- `boxing_metadata_list` is a hashmap that holds a list of metadata entries from
  the structural metadata bar (see
  [Boxing barcode - Format](https://en.wikipedia.org/wiki/Boxing_barcode#Format)).
- `boxing_image8` is a basic image container which stores a width, height, and
  pixel data.
- `gvector` is a basic untyped dynamic array to store a growable list of items.
  For unboxing this will be used to store the resultant data from decoding
  frames.

## Initialization for unboxing the control frame

<!-- TODO: Rewrite this deranged paragraph -->

In order to initialize an unboxer we first need to construct a
`boxing_unboxer_parameters` object. In order to create a useful
`boxing_unboxer_parameters` object, we first need to create a `boxing_config`
object. The `boxing_config` object is what defines the format we are unboxing
for. This means that unboxing a new format implies creating a new unboxer.
Examples of different formats include `4k-controlframe-v7`, `4kv8`, etc. Formats
are printed in plain text in the margins of each frame.

The only `boxing_config` values we need for unboxing a whole reel are the
control-frame-defining formats. They are stored in C source form (as
`config_structure`s) in
[the unboxing repository](https://github.com/piql/unboxing/tree/master/tests/testutils/src)
(`config_source_4k_controlframe_vX.h`). For completeness several formats used in
data frames (such as 4kv8, 4kv11) are also stored here, but they are not
required for decoding a reel.

To initialize a `boxing_config` we first need to include the relevant
header-files (assuming the above directory has been added to the project include
search paths, as well as the
[`inc` directory](https://github.com/piql/unboxing/tree/master/inc)):

```c
#include <boxing/config.h>
#include <boxing/unboxer.h>
#include <config_source_4k_controlframe_v7.h>
```

<!--
```c
#include <assert.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../dep/stb/stb_image.h"
static boxing_image8 LOAD_IMAGE_SOMEHOW(int frame_id) {
  char path[256];
  snprintf(path, sizeof path, "dep/ivm_testdata/reel/png/%06d.png", frame_id);
  fprintf(stderr, "LOAD_IMAGE_SOMEHOW(%d): loading %s\n", frame_id, path);
  int width;
  int height;
  unsigned char *data = stbi_load(path, &width, &height, NULL, 1);
  return (boxing_image8){
    .width = width,
    .height = height,
    .is_owning_data = DFALSE,
    .data = data,
  };
}
int main(int argc, char *argv[]) {
(void)argc;
(void)argv;
```
-->

Then we can initialize a `boxing_config` using the control frame data:

```c
boxing_config *config = boxing_config_create_from_structure(&config_source_v7);
```

It is also possible to initialize a boxing_config by loading the configuration
from a configuration XML file using the
[`afs`](https://github.com/piql/afs/blob/35d6f06770e2586b092fa590e764df256c0c8ae2/inc/controlframe/boxingformat.h#L67)
library.

For `boxing_unboxer_parameters` may also specify whether or not we want to
decode using "raw mode". For scanned images this will generally not be the case.
Using the `config` variable we can initialize a `boxing_unboxer_parameters`
value like so:

```c
boxing_unboxer_parameters parameters;
boxing_unboxer_parameters_init(&parameters);
parameters.format = config;
```

If you wish to use "raw mode", for example if you are decoding digital frame
images that were not photographed / scanned from physical film, you should also
set the `is_raw` parameter like so:

```c
parameters.is_raw = 1;
```

Finally we will be able to construct an unboxer object:

```c
boxing_unboxer *unboxer = boxing_unboxer_create(&parameters);
```

In order to unbox we also need to create a `boxing_metadata_list`, which will be
populated with any decoded metadata from the frame we are decoding:

```c
boxing_metadata_list *metadata_list = boxing_metadata_list_create();
```

<!--
```c
unsigned int IMAGE_WIDTH;
unsigned int IMAGE_HEIGHT;
unsigned char *IMAGE_DATA;
{
  boxing_image8 image = LOAD_IMAGE_SOMEHOW(1);
  IMAGE_WIDTH = image.width;
  IMAGE_HEIGHT = image.height;
  IMAGE_DATA = image.data;
}
```
-->

### Loading image data

In order to provide the image data to the library we need to load the scanned
image into memory, and get the image width and height. The pixel format (in
`IMAGE_DATA`) is expected to be inverted, and a 1 byte per pixel grayscale
image, starting from the top left of the frame, and going row by row from left
to right. Once we have the image width and height (`IMAGE_WIDTH`,
`IMAGE_HEIGHT`), and the data correctly loaded, we can construct a
`boxing_image8`:

```c
boxing_image8 image = {
    .width = IMAGE_WIDTH,
    .height = IMAGE_HEIGHT,
    .is_owning_data = DFALSE,
    .data = IMAGE_DATA,
};
```

### Performing the decoding

We also need to construct a `gvector` value and `extract_result` integer to hold
the resulting data from decoding the frame, and the error code from extracting
the inner data from the container.

```c
gvector data = {
    .buffer = NULL,
    .size = 0,
    .item_size = 1,
    .element_free = NULL,
};
int extract_result;
```

`decode_result` will hold the result of decoding the data. Both `extract_result`
and `decode_result` should have the value `BOXING_UNBOXER_OK` after unboxing is
done if everything went successfully. See the enum `boxing_unboxer_result`
defined in
[`boxing/unboxer.h`](https://github.com/piql/unboxing/blob/master/inc/boxing/unboxer.h)
for more information about possible error codes, and remember to check the logs
for additional failure information.

Finally we perform the unboxing:

```c
int decode_result = boxing_unboxer_unbox(&data, metadata_list, &image, unboxer, &extract_result, NULL, BOXING_METADATA_CONTENT_TYPES_CONTROLFRAME);
```

For debugging purposes, we can now print the control frame data contents if
everything went well (You should check that `extract_result` and `decode_result`
are both equal to `BOXING_UNBOXER_OK` before accessing the data buffer):

<!--
```c
assert(extract_result == BOXING_UNBOXER_OK && decode_result == BOXING_UNBOXER_OK);
```
-->

```c
printf("%.*s\n", (int)data.size, (char *)data.buffer);
```

You should expect to see a minified XML file at this point.

After decoding the control frame, we can parse the contents, and decode the rest
of the reel.

## Parsing the control frame and unboxing the Table of Contents

There are many ways to parse the control frame data. Piql provides a "built-in"
and self-contained way which is printed in the representation information within
the reel along with the unboxing source code. This is the `afs` library.

First we'll need to add some more includes to our project:

```c
#include <string.h>
#include <controldata.h>
```

In order to parse the control frame data using `afs`, we will want to initialize
an `afs_control_data` object and load the XML string into it. We'll have to
zero-terminate the XML string first:

```c
data.buffer = realloc(data.buffer, data.size + 1);
((char *)data.buffer)[data.size] = '\0';
data.size++;

afs_control_data *ctl = afs_control_data_create();
afs_control_data_load_string(ctl, (const char *)data.buffer);
```

Once we have loaded the control data we can print some information that was
stored in the XML text:

```c
printf(
  "Loaded control frame of a piqlFilm titled '%s' created by '%s' at '%s'\n",
  ctl->administrative_metadata->title,
  ctl->administrative_metadata->creator,
  ctl->administrative_metadata->creation_date
);
```

Next we need to find the frame numbers corresponding to the Table of Contents:

```c
afs_toc_file *toc_file = afs_toc_files_get_toc(ctl->technical_metadata->afs_tocs, 0);
printf(
  "First Table of Contents file is %zu bytes, starts at frame %d byte offset %d, and ends at frame %d byte offset %d (inclusive)\n",
  (size_t)toc_file->size,
  toc_file->start_frame,
  toc_file->start_byte,
  toc_file->end_frame,
  toc_file->end_byte
);
```

<!--
```c
  {
```
-->

In order to actually decode the data frames containing table of contents and
other files, we actually need to initialize a second unboxer using the
configuration loaded from the control frame data, so first we'll need to
initialize parameters:

```c
boxing_unboxer_parameters parameters;
boxing_unboxer_parameters_init(&parameters);
parameters.format = ctl->technical_metadata->afs_content_boxing_format->config;
```

Like before, if we want to decode digital frames, we should set `is_raw`:

```c
parameters.is_raw = 1;
```

And then we create a new unboxer, using our parameters initialized using the
decoded boxing format from the control frame.

```c
boxing_unboxer *unboxer = boxing_unboxer_create(&parameters);
```

Following this newly created unboxer, we can start decoding table of contents by
iterating through the frames (keep in mind you also need to implement a method
of loading each scanned image file on demand) and concatenating all the data
results into one big buffer. Refer to the section on
[loading image data](#loading-image-data) for more information on how
`LOAD_IMAGE_SOMEHOW` could be implemented.

```c
void *table_of_contents = NULL;
size_t table_of_contents_size = 0;
boxing_metadata_list *metadata_list = boxing_metadata_list_create();
for (int current_frame = toc_file->start_frame; current_frame <= toc_file->end_frame; current_frame++) {
  boxing_image8 image = LOAD_IMAGE_SOMEHOW(current_frame);
  gvector data = {
    .buffer = NULL,
    .size = 0,
    .item_size = 1,
    .element_free = NULL,
  };
  int extract_result;
  int decode_result = boxing_unboxer_unbox(&data, metadata_list, &image, unboxer, &extract_result, NULL, BOXING_METADATA_CONTENT_TYPES_TOC);
  if (extract_result != BOXING_UNBOXER_OK || decode_result != BOXING_UNBOXER_OK) {
    return EXIT_FAILURE;
  }
  table_of_contents = realloc(table_of_contents, table_of_contents_size + data.size);
  memcpy((char *)table_of_contents + table_of_contents_size, data.buffer, data.size);
  table_of_contents_size += data.size;
  free(image.data);
  free(data.buffer);
}
```

Then we will want to offset the data we received by start_byte, and limit it by
file size:

```c
memmove(table_of_contents, (char *)table_of_contents + toc_file->start_byte, toc_file->size);
table_of_contents = realloc(table_of_contents, toc_file->size);
table_of_contents_size = toc_file->size;
```

After all this we can finally attempt to print our table of contents data:

```c
printf("Table of Contents (%zu): %.*s\n", table_of_contents_size, (int)table_of_contents_size, (char *)table_of_contents);
```

## Parsing the Table of Contents and decoding the rest of the files on the film

For this example we will continue using the `afs` library, so we'll need another
include directive:

```c
#include <tocdata_c.h>
```

Parsing the table of contents is very similar to parsing the control frame data:

```c
table_of_contents = realloc(table_of_contents, table_of_contents_size + 1);
((char *)table_of_contents)[table_of_contents_size] = '\0';
table_of_contents_size++;

afs_toc_data *toc = afs_toc_data_create();
afs_toc_data_load_string(toc, table_of_contents);
```

Once the Table of Contents is loaded, we can start looking at what files are
stored on the film:

```c
afs_toc_data_reel *reel = afs_toc_data_reels_get_reel(toc->reels, 0);
unsigned file_count = afs_toc_data_reel_file_count(reel);
for (unsigned i = 0; i < file_count; i++) {
  afs_toc_file *file = afs_toc_data_reel_get_file_by_index(reel, i);
  printf("Found file named '%s' with size %zu starting at frame %d with checksum: %s\n", file->name, (size_t)file->size, file->start_frame, file->checksum);
```

And to unbox a file, we use an essentially identical method to unboxing the
table of contents. We can reuse the same unboxer object we used for the table of
contents, but it's important to call `boxing_unboxer_reset` between each range
of frames to support striped reels (the unboxer retains some state in the case
of striped reels, and it should be flushed once you are done reading a full
file, so the striped decoding can restart from a stable starting state):

```c
boxing_unboxer_reset(unboxer);

void *file_data = NULL;
size_t file_size = 0;
for (int current_frame = file->start_frame; current_frame <= file->end_frame; current_frame++) {
  boxing_image8 image = LOAD_IMAGE_SOMEHOW(current_frame);
  gvector data = {
    .buffer = NULL,
    .size = 0,
    .item_size = 1,
    .element_free = NULL,
  };
  int extract_result;
  int decode_result = boxing_unboxer_unbox(&data, metadata_list, &image, unboxer, &extract_result, NULL, BOXING_METADATA_CONTENT_TYPES_DATA);

  if (extract_result != BOXING_UNBOXER_OK || decode_result != BOXING_UNBOXER_OK) {
    return EXIT_FAILURE;
  }

  file_data = realloc(file_data, file_size + data.size);
  memcpy((char *)file_data + file_size, data.buffer, data.size);
  file_size += data.size;
  free(image.data);
  free(data.buffer);
}
```

Again we offset by start_byte and limit by file size:

```c
memmove(file_data, (char *)file_data + file->start_byte, file->size);
file_data = realloc(file_data, file->size);
file_size = file->size;
```

Now we are ready to for example output our file to disk:

```c
printf("Writing file: '%s' (size: %zu)\n", file->name, (size_t)file->size);
FILE *f = fopen(file->name, "w");
fwrite(file_data, 1, file_size, f);
fclose(f);
free(file_data);
```

You may want additional code to create subdirectories before attempting to open
and write a file, as well as checking the return values of fopen / fwrite.

More optimal implementations of this algorithm may want to dump the file to disk
as it is being read, since the file may be too large to hold in entirely in
memory. Refer to the unbox command line tool [source code](../src/main.c) for
more information and an alternative implementation. This guide is meant to
provide the simplest possible implementation.

You will be able to verify the checksums of the file with a SHA1 command line
tool (like `sha1sum` on unix systems). [An implementation](../dev/sha1.c) of a
simple command-line tool is included in this project for completeness.

<!-- TODO: SHA1 implementation? -->

<!--
```c
      char always = 1;
      if (always) break;
    }
    afs_toc_data_free(toc);
    free(table_of_contents);
    boxing_metadata_list_free(metadata_list);
    boxing_unboxer_free(unboxer);
    boxing_unboxer_parameters_free(&parameters);
  }
  afs_control_data_free(ctl);
  if (data.buffer) free(data.buffer);
  if (IMAGE_DATA) free(IMAGE_DATA);
  boxing_metadata_list_free(metadata_list);
  boxing_unboxer_free(unboxer);
  boxing_unboxer_parameters_free(&parameters);
  boxing_config_free(config);
}
```
-->
