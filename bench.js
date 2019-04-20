// Config:
const N = 10; // num files/iterations
const P = 2; // parallelism, do not exceed 32 for now
const fileSize = 1024;

/////

const iou = require(".");
const fs = require("fs");
const async = require("async");
const crypto = require("crypto");

// Setup
const fds = [];
const buffs = [];
for (let i = 0; i < N; i++) {
	fs.writeFileSync(`./.bench/${i}`, crypto.randomBytes(fileSize));
	fds.push(fs.openSync(`./.bench/${i}`, "r", 0o666));
	buffs.push(Buffer.allocUnsafe(fileSize));
}

const methods = [{fn: fs.read, lbl: "fs"}, {fn: iou.read, lbl: "iouring"}];

// Run
while (true) {
	const {fn, lbl} = methods[Math.round(Math.random())];
	const start = process.hrtime.bigint();
	async.timesLimit(N, P, (i, done) => {
		fn(fds[i], buffers[i], 0, fileSize, 0, (err, data) => {
			if (err) return done(err);
			if (data.length !== fileSize) return done(new Error("Truncated"));
			done();
		});
	}, err => {
		if (err) throw err;
		const duration = Number(process.hrtime.bigint() - start);
		console.log(lbl, duration * 1e-6, "ms");
	});
}
