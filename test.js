const iou = require(".");

iou.readFile("/home/zachb/node-io-uring/deps/liburing/man/io_uring_enter.2", (err, result) => {
	console.log(err, result && result.toString());
	iou.writeFile("./test", result, err => console.log("write", err));
});
