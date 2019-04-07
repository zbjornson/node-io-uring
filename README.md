io_uring is a new I/O interface in the Linux 5.1 kernel that allows for efficient, async I/O. See intro at http://kernel.dk/io_uring.pdf.

This repo has a proof-of-concept `readFile()` implementation replacement for Node.js. (Not sure why I did `readFile()` instead of the more flexible `read()`; will change later.)

Requires liburing, available at http://git.kernel.dk/cgit/liburing/tree/. Modify binding.gyp to point to the header location. Also need to add `#ifdef __cplusplus \n extern "C" {` ... `}` to liburing.h.

TODO - fix the deprecated JS callback invocations.
