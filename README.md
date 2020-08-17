<img alt="PeqBank" src="PeqBank.png" width="100%" max-width="888">

[![CircleCI](https://circleci.com/gh/spotify/PeqBank/tree/master.svg?style=svg)](https://circleci.com/gh/spotify/PeqBank/tree/master)
[![License](https://img.shields.io/github/license/spotify/PeqBank.svg)](LICENSE)
[![Spotify FOSS Slack](https://slackin.spotify.com/badge.svg)](https://slackin.spotify.com)
[![Readme Score](http://readme-score-api.herokuapp.com/score.svg?url=https://github.com/spotify/PeqBank)](http://clayallsopp.github.io/readme-score?url=https://github.com/spotify/PeqBank)

- [x] 📱 [iOS](https://www.apple.com/ios/) 9.0+
- [x] 💻 [OS X](https://www.apple.com/macos/) 10.11+
- [x] 🐧 [Ubuntu](https://www.ubuntu.com/) Trusty 14.04+
- [x] 🤖 [Android](https://developer.android.com/studio/) SDK r24+
- [x] 🖥️ [Microsoft UWP](https://developer.microsoft.com/en-us/windows/apps)

## Architecture :triangular_ruler:

`PeqBank` is a C-library designed to perform parametric equalization using analog-like parametric EQs. This implementation was documented in [Musical Applications of New Filter Extensions to Max/MSP](http://web.media.mit.edu/~tristan/Papers/ICMC99_MaxFilters.pdf).

## Usage example :eyes:

[`main.c`](source/main.c) provides an example program.

The following explains how to setup a low pass filter:

```c
#include "peqbank.h"

/* Assume arrays signal_in and signal_out and values
   num_frames, buffer_size and num_channels are declared
  $a. somewhere else
*/
t_filter **filters = new_filters(1);  // init number of desired filters
filters[0] = new_lowpass(4000, 1, 6);  // attach lowpass filter
peqbank_setup(x, filters);             // setup filters
for (int i = 0; i < num_frames; i += buffer_size) {
  if (num_frames - i < buffer_size) x->s_n = num_frames - i;
  frame += peqbank_callback_int16(
      x, &signal_in[frame * num_channels], &signal_out[frame * num_channels]);
}
x->s_n = buffer_size;  // reset buffer size...
free(filters[0]);
```

It is important to note that the peq filter bank must be allocated and free'd manually.
There is no RAII support as it is currently meant to be C-compatible.

## Installation :inbox_tray:

`PeqBank` is a [Cmake](https://cmake.org/) project. While you are free to download the prebuilt static libraries it is recommended to use Cmake to install this project into your wider project. In order to add this into a wider Cmake project, add the following line to your `CMakeLists.txt` file:
```
add_subdirectory(PeqBank)
```

Each script in [`ci/`](./ci) demonstrates how to build and the dependencies required per platform, although they do not account for integration into other build systems, like Gradle.

## Development

Generally the process is:

```sh
$ mkdir build
$ cd build
$ cmake .. -GXcode # or Ninja
$ open PeqBank.xcodeproj # MacOS Only
```

Check ths scripts in [`ci/`](./ci) for more platform-specific details.

## Contributing :mailbox_with_mail:
Contributions are welcomed, have a look at the [CONTRIBUTING.md](CONTRIBUTING.md) document for more information.

This project adheres to the [Open Code of Conduct][code-of-conduct]. By participating, you are expected to honor this code.

[code-of-conduct]: https://github.com/spotify/code-of-conduct/blob/master/code-of-conduct.md

## License :memo:
The project is available under the [Apache 2.0](http://www.apache.org/licenses/LICENSE-2.0) license.

### Acknowledgements

- Icon in readme banner is "[Bank by Adrien Coquet from the Noun Project](https://thenounproject.com/term/bank/867897/)".

## Contributors :woman_technologist: :man_technologist:

Primary contributors to this repository at the time of open sourcing:

* [Tristan Jehan](https://github.com/tjehan): Original Author
* [David Rubinstein](https://github.com/drubinstein)
* [Julia Cox](https://github.com/astrocox)
* [Will Sackfield](https://github.com/8W9aG)

