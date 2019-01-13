{
	"targets": [{
		"target_name": "fastscreenshot",
			"sources": ["index.cc"],
			"include_dirs": [
				"<!(node -e \"require('nan')\")"
			],
			"link_settings": {
				"libraries": [
					"-lXtst",
				"-lX11",
				"-lXfixes"
				]

			}
	}]
}
