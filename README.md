# x-fastscreenshot
node.js module for fast screenshots on the X window system via XShm 📷⚡

Most of the code is from [sylvain121/node-X11](https://github.com/sylvain121/node-X11) + proper rgb conversion. There is only Image Buffer output for it be as simple as possible so for writing to a file you will need to add another module.



# Example with file output

`npm i --save x-fastscreenshot`

```JS
const fastscreenshot = require('x-fastscreenshot');
const jimp = require('jimp');
fastscreenshot.init();

fastscreenshot.getImage((err, image){
  if(err) throw err;
  jimp.read({width: image.width, height: image.height, data: image.data}).then(img => {
    return img.write('output.png');
  });
});
```

# Speed

Taking a picture on a single 1080p screen setup takes around 16ms according to my benchmark

```JS
const fastscreenshot = require("./node_modules/x-fastscreenshot/index.js");
fastscreenshot.init();

console.time("img");
fastscreenshot.getImage((err, image) => {
	console.timeEnd("img");
});
```

```
XShm extention version 1.2 with shared pixmaps
X-Window-init: dimension: 1920x1080x24 @ 0/1
img: 15.996ms
```
