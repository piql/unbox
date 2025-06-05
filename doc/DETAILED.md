# How to decode data from piqlFilm - detailed explanation

This explanation will focus mainly on usage of the unboxing library in order to
decode a full piqlFilm. In the future it may also cover building the library and
project setup, but for now it will only focus on usage in code.

All code snippets are in C99.

## Initialization for unboxing the control frame

In order to initialize an unboxer we first need to construct a
`boxing_unboxer_parameters` object. In order to create a
`boxing_unboxer_parameters` object, we first need to create a `boxing_config`
object. The `boxing_config` object is what defines the format we are unboxing
for. This means that unboxing a new format implies creating a new unboxer.
Examples of different formats include `4k-controlframe-v7`, `4kv8`, etc. Formats
are printed in plain text in the margins of each frame.

The only `boxing_config` values we need for unboxing a whole reel are the
control-frame-defining formats. They are stored in
[the unboxing repository](https://github.com/piql/unboxing/tree/master/tests/testutils/src)
(`config_source_4k_controlframe_vX.h`).

To initialize a `boxing_config` we first need to include the relevant
header-files (assuming the above directory has been added to the project include
search paths, as well as the
[`inc` directory](https://github.com/piql/unboxing/tree/master/inc)):

```c
#include <boxing/config.h>
#include <boxing/unboxer.h>
#include <config_source_4k_controlframe_v7.h>
```

Then we can initialize a `boxing_config` using the control frame data:

```c
boxing_config *config = boxing_config_create_from_structure(&config_source_v7);
```

For `boxing_unboxer_parameters` may also specify whether or not we want to
decode using "raw mode". For scanned images this will generally not be the case.
Using the `config` variable we can initialize a `boxing_unboxer_parameters`
value like so:

```c
boxing_unboxer_parameters parameters;
boxing_unboxer_parameters_init(&parameters);
parameters.format = config;
```

Finally we will be able to construct an unboxer object:

```c
boxing_unboxer *unboxer = boxing_unboxer_create(&parameters);
```

<!-- TODO: why are we creating this? -->

In order to unbox we also need to create a `boxing_metadata_list`:

```c
boxing_metadata_list *metadata_list = boxing_metadata_list_create();
```

Finally we'll be able to run the unboxer to decode a frame:

```c
boxing_image8 image = {
    .width = (width of the image),
    .height = (height of the image),
    .is_owning_data = DFALSE,
    .data = (image data),
};
gvector data = {
    .buffer = NULL,
    .size = 0,
    .item_size = 1,
    .element_free = NULL,
};
int extract_result;
int decode_result = boxing_unboxer_unbox(&data, metadata_list, &image, unboxer, &extract_result, NULL, BOXING_METADATA_CONTENT_TYPES_CONTROLFRAME);
```

- `image` is your image input data as an array of grayscale pixels, and width
  and height in pixels.
- `data` is the output vector that will be filled with the decoded frame data.
- `extract_result` is the result of extracting the inner data from the container
  (see
  [Format description on Wikipedia](https://en.wikipedia.org/wiki/Boxing_barcode#Format))
  and decoding the metadata bar.
- `decode_result` is the result of decoding the data.

(In progress...)
