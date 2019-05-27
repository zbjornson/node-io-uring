// Config:
const N = 1000; // num files/iterations
const P = 32; // parallelism, do not exceed 32 for now
const fileSize = 1024;
let maxTimes = 5;
/////

const iou = require(".");
const fs = require("fs");
const async = require("async");
const crypto = require("crypto");

// Setup
const fds = [];
const buffers = [];
for (let i = 0; i < N; i++) {
	fs.writeFileSync(`./.bench/${i}`, crypto.randomBytes(fileSize));
	fds.push(fs.openSync(`./.bench/${i}`, "r", 0o666));
	buffers.push(Buffer.allocUnsafe(fileSize));
}

const methods = [
//	{fn: fs.read, lbl: "fs     "}
	{fn: iou.read, lbl: "iouring"}
];

// Run
const is = process.hrtime.bigint();
function run() {
	const {fn, lbl} = methods[0];//Math.round(Math.random())];
	async.timesLimit(N, P, (i, done) => {
		const start = process.hrtime.bigint();
		fn(fds[i], buffers[i], 0, fileSize, 0, (err, bytesRead) => {
			if (err) return done(err);
			if (bytesRead !== fileSize) return done(new Error("Truncated"));
			console.log(Number(start - is), Number(process.hrtime.bigint() - is));
			done();
		});
	}, err => {
		if (err) throw err;
//		const duration = Number(process.hrtime.bigint() - start);
//		console.log(lbl, duration * 1e-6, "ms");
		if (--maxTimes) run();
	});
}

run();

