io_uring is a new I/O interface in the Linux 5.1 kernel that allows for efficient, async I/O. See intro at http://kernel.dk/io_uring.pdf.

This repo has proof-of-concept `read()`, `readFile()`, `write()` and `writeFile()` implementation replacements for Node.js. (They're not complete drop-ins but are sufficient for benchmarking.)

Note that this requires the kernel patch https://github.com/torvalds/linux/commit/9b402849e80c85eee10bbd341aab3f1a0f942d4f, which is on track for version 5.2.
