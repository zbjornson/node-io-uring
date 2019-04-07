{
	"targets": [{
		"target_name": "iou",
		"sources": ["iou.cc"],
		"include_dirs": [
			"<!(node -e \"require('nan')\")",
			"/home/zbjornson/liburing/src"
		],
		"cflags":[
			"-Wno-cast-function-type"
		]
	}]
}