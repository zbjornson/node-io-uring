{
	"targets": [{
		"target_name": "iou",
		"sources": ["iou.cc"],
		"include_dirs": [
			"<!(node -e \"require('nan')\")",
			"deps"
		],
		"libraries": [
			"<(module_root_dir)/deps/liburing/src/liburing.a"
		],
		"cflags":[
			"-Wno-cast-function-type"
		]
	}]
}
