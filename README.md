io_uring is a new I/O interface in the Linux 5.1 kernel that allows for efficient, async I/O. See intro at http://kernel.dk/io_uring.pdf.

This repo has proof-of-concept `read()`, `readFile()`, `write()` and `writeFile()` implementation replacements for Node.js. (They're not complete drop-ins but are sufficient for benchmarking.)

Not sure if using libuv's idle check is the best way to poll.
