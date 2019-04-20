const bindings = require("./build/Release/iou.node");
const fs = require("fs");

function readFile(path, cb) {
	if (typeof cb !== "function") {
		throw new TypeError("Callback must be a function");
	}
	fs.open(path, "r", 0o666, (err, fd) => {
		if (err) return cb(err);
		const onerr = err => fs.close(fd, () => cb(err));
		fs.fstat(fd, (err, stats) => {
			if (err) return onerr(err);
			const dst = Buffer.allocUnsafe(stats.size);
			bindings.read(fd, dst, 0, stats.size, 0, (err, bytesRead, data) => {
				if (err) return onerr(err);
				fs.close(fd, err => cb(err, data));
			});
		});
	});
}

function writeFile(path, data, cb) {
	if (typeof cb !== "function") {
		throw new TypeError("Callback must be a function");
	}
	fs.open(path, "w", 0o666, (err, fd) => {
		if (err) return cb(err);
		const onerr = err => fs.close(fd, () => cb(err));
		bindings.writeBuffer(fd, data, 0, data.byteLength, 0, (err, bytesWritten, data) => {
			if (err) return onerr(err);
			fs.close(fd, err => cb(err));
		});
	});
}

module.exports = {
	readFile,
	writeFile,
	read: bindings.read,
	write: bindings.writeBuffer
};

/*
writeFile("./test", Buffer.from("hello!!!"), err => console.log("wrote", err));
*/
/*
readFile("/home/zachb/node-io-uring/deps/liburing/man/io_uring_enter.2", (err, result) => {
	console.log(err, result && result.toString());
	writeFile("./test", result, err => console.log("write", err));
});
*/

