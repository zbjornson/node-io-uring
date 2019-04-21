// Config:
const N = 1000; // num files/iterations
const P = Number.parseInt(process.argv[3], 10); // parallelism, do not exceed 32 for now
const fileSize = 1024;
let maxTimes = 100;
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
	{fn: fs.read, lbl: "fs     "},
	{fn: iou.read, lbl: "iouring"}
];

const method = methods[process.argv[2] === "fs" ? 0 : 1];
console.log("Testing", method.lbl, P, "parallelism");

// Run
let M = 0, S = 0, mPrev, n = 0;
function run() {
	const {fn, lbl} = method; //s[Math.round(Math.random())];
	const start = process.hrtime.bigint();
	async.timesLimit(N, P, (i, done) => {
		const start = process.hrtime.bigint();
		fn(fds[i], buffers[i], 0, fileSize, 0, (err, bytesRead) => {
			const dur = Number(process.hrtime.bigint() - start);
			if (err) return done(err);
			if (bytesRead !== fileSize) return done(new Error("Truncated"));
			mPrev = M;
			M += (dur - M) / (n + 1);
			S += (dur - M) * (dur - mPrev);
			n++;
			done();
		});
	}, err => {
		if (err) throw err;
		//const duration = Number(process.hrtime.bigint() - start);
//		console.log(lbl, duration * 1e-6, "ms");
		if (--maxTimes) run();
	});
}

process.on("SIGINT", () => {
	console.log(M, "+/-" , Math.sqrt(S / n));
	process.exit(0);
});

process.on("exit", () => {
	console.log(M, "+/-", Math.sqrt(S / n));
});

run();

