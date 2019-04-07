const iou = require(".");
const fs = require("fs");
const async = require("async");
const crypto = require("crypto");

const N = 10; // num files/iterations
const P = 2; // parallelism, do not exceed 32 for now

// Setup
for (let i = 0; i < N; i++) {
	fs.writeFileSync("./.bench/" + i, crypto.randomBytes(1000000));
}

// Non-partitioned readFile
function fsreadFile(path, cb) {
	if (typeof cb !== "function") {
		throw new TypeError("Callback must be a function");
	}
	fs.open(path, "r", 0o666, (err, fd) => {
		if (err) return cb(err);
		const onerr = err => fs.close(fd, () => cb(err));
		fs.fstat(fd, (err, stats) => {
			if (err) return onerr(err);
			const dst = Buffer.allocUnsafe(stats.size);
			fs.read(fd, dst, 0, stats.size, 0, (err, bytesRead, data) => {
				if (err) return onerr(err);
				fs.close(fd, err => cb(err, data));
			});
		});
	});
}

console.time("bench");
async.timesLimit(N, P, (n, next) => {
	fsreadFile(`./.bench/${n}`, (err, data) => {
		if (err) return next(err);
		console.assert(data.length, 1000000);
		next();
	});
}, err => {
	if (err) throw err;
	console.timeEnd("bench");
});
