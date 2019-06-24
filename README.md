io_uring is a new I/O interface in the Linux 5.1 kernel that allows for efficient, async I/O. See intro at http://kernel.dk/io_uring.pdf.

This repo has proof-of-concept `read()`, `readFile()`, `write()` and `writeFile()` implementation replacements for Node.js. They're not complete drop-ins but are sufficient for benchmarking and testing.

I'm planning to polish up this module soon, and expand it to expose all io_uring functionality since it's unlikely that libuv or Node.js would be reasonably able to do so. I'm also not sure when basic support for io_uring will land in Node.js (https://github.com/libuv/libuv/pull/2322).

```js
const iouring = require("iouring");
iouring.read(fd, dest, offset, len, position, (err, bytesRead, dest) => { });
iouring.write(fd, src, offset, len, position, (err, bytesWritten, dest) => { });
iouring.readFile(path, (err, buff) => { });
iouring.writeFile(path, data, (err) => { })
```
The signatures are the same as Node.js' `fs` functions.

Current limitations:
* Only 32 read/write requests may be in-flight at a time. Easily fixed.
* Error checking is minimal, overall code quality is not great. Easily fixed.
* No `fsync` support.
* `position` (the file position) must be specified and must not be `-1`; unlike Node.js' `fs` methods, "current position" (seeking) operations are not supported. This can be mostly fixed by calling `lseek(2)` before and after the io_uring operation, but the fd cursor position update will not be atomic. (That is unlikely to be a problem for Node.js.)
* `fd` must support read_iter/write_iter -- it cannot be a pipe such as stdin, stdout, stderr or a fifo. This is a restriction imposed by io_uring that cannot be avoided.
