io_uring is a new I/O interface in the Linux 5.1 kernel that allows for efficient, async I/O. See intro at http://kernel.dk/io_uring.pdf.

This repo has proof-of-concept `read()`, `readFile()`, `write()` and `writeFile()` implementation replacements for Node.js. (They're not complete drop-ins but are sufficient for benchmarking.)

Note that this requires the kernel patch http://git.kernel.dk/cgit/linux-block/commit/?h=io_uring-next&id=62e22fe9c6f055d1424132bd1e1cb5a0d51649b7.

