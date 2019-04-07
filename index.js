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
			bindings.readFile(fd, stats.size, (err, data) => {
				if (err) return onerr(err);
				fs.close(fd, (err) => cb(err, data));
			});
		});
	});
}

module.exports = {
	readFile
};

/* TEST:
readFile("./README.md", (err, result) => {
	console.log(err, result && result.toString());
})
*/