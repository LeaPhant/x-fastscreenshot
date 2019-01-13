#include <nan.h>
#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/ipc.h>
#include <X11/X.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <sys/shm.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XTest.h>

#include "index.h"

using namespace v8;
using v8::FunctionTemplate;

char * pDisplay = std::getenv("DISPLAY");

int width = 0;
int height = 0;
int depth = 0;

static int screenNumber;
static Display *display = NULL;
static Window rootWindow;
static Screen *screen = NULL;
XColor color;
XImage * ximage = NULL;
static XShmSegmentInfo __xshminfo;

class GetImageWorker : public Nan::AsyncWorker {
	public:
		Image *image;

		GetImageWorker(Nan::Callback* callback)
		: Nan::AsyncWorker(callback) {  }

		void Execute() {
			image = ( Image* )malloc(sizeof(Image));
			image->data = (char*) malloc(sizeof(char)*width*height*4);

			if(XShmGetImage(display, rootWindow, ximage, 0, 0, XAllPlanes()) == 0) {
			  printf("FATAL: XShmGetImage failed.\n");
			  exit(-1);
			}

			image->width = ximage->width;
			image->height = ximage->height;
			image->size = image->width * image->height * 4;
			memcpy(image->data, ximage->data, image->size);
			image->depth = ximage->depth;
			image->bits_per_pixel = ximage->bits_per_pixel;
			image->bytes_per_line = ximage->bytes_per_line;

			unsigned char blue;
			for(int i = 0; i < image->size; i += 4){
			  blue = image->data[i];

			  image->data[i]     = image->data[i + 2];
			  image->data[i + 2] = blue;
			}

		}

		void HandleOKCallback() {
			Nan::HandleScope scope;
			uint32_t bufferSize = image->width * image->height * (image->bits_per_pixel / 8);
			Local<Object> buffer = Nan::NewBuffer((char*) image->data, bufferSize).ToLocalChecked();
			Local<Object> obj = Nan::New<Object>();
			Nan::Set(obj, Nan::New("width").ToLocalChecked(), Nan::New<Number>(image->width));
			Nan::Set(obj, Nan::New("height").ToLocalChecked(), Nan::New<Number>(image->height));
			Nan::Set(obj, Nan::New("depth").ToLocalChecked(), Nan::New<Number>(image->depth));
			Nan::Set(obj, Nan::New("bits_per_pixel").ToLocalChecked(), Nan::New<Number>(image->bits_per_pixel));
			Nan::Set(obj, Nan::New("bytes_per_line").ToLocalChecked(), Nan::New<Number>(image->bytes_per_line));
			Nan::Set(obj, Nan::New("data").ToLocalChecked(), buffer);

			v8::Local<v8::Value> argv[] = {
				Nan::Null(), // no error occured
				obj

			};
			callback->Call(2, argv);

		}

		void HandleErrorCallback() {
			Nan::HandleScope scope;
			v8::Local<v8::Value> argv[] = {
				Nan::New(this->ErrorMessage()).ToLocalChecked(), // return error message
				Nan::Null()
			};
			callback->Call(2, argv);

		}
};

NAN_METHOD(init)
{
	if(info.Length() == 1 && info[0]->IsString()){
		Nan::Utf8String dstr(info[0]);
		pDisplay  = *dstr;
	}

	int ignore = 0;
	bzero(&__xshminfo, sizeof(__xshminfo));

	// open display
	if((display = XOpenDisplay(pDisplay)) == NULL) {
	  printf("cannot open display \"%s\"\n", pDisplay ? pDisplay : "DEFAULT");
	}

	// check MIT extension
	if(XQueryExtension(display, "MIT-SHM", &ignore, &ignore, &ignore) ) {
	  int major, minor;
	  Bool pixmaps;
	  if(XShmQueryVersion(display, &major, &minor, &pixmaps) == True) {
	    printf("XShm extention version %d.%d %s shared pixmaps\n",
	        major, minor, (pixmaps==True) ? "with" : "without");
	  }else{
	    printf("XShm extension not supported.\n");
	  }
  }
  // get default screen
	screenNumber = XDefaultScreen(display);
	if((screen = XScreenOfDisplay(display, screenNumber)) == NULL) {
	  printf("cannot obtain screen #%d\n", screenNumber);
	}
	// get screen hight, width, depth
	width = XDisplayWidth(display, screenNumber);
	height = XDisplayHeight(display, screenNumber);
	depth = XDisplayPlanes(display, screenNumber);
	printf("X-Window-init: dimension: %dx%dx%d @ %d/%d\n",
	    width, height, depth,
	    screenNumber, XScreenCount(display));
	//create image context
	if((ximage = XShmCreateImage(display,
	        XDefaultVisual(display, screenNumber),
	        depth, ZPixmap, NULL, &__xshminfo,
	        width, height)) == NULL) {
	  printf("XShmCreateImage failed.\n");
	}


	//get shm info
	if((__xshminfo.shmid = shmget(IPC_PRIVATE,
	        ximage->bytes_per_line*ximage->height,
	        IPC_CREAT | 0777)) < 0) {
	  printf("shmget error");
	}

	__xshminfo.shmaddr = ximage->data = (char*) shmat(__xshminfo.shmid, 0, 0);
	__xshminfo.readOnly = False;
	if(XShmAttach(display, &__xshminfo) == 0) {
	  printf("XShmAttach failed.\n");
	}

	rootWindow = XRootWindow(display, screenNumber);
}

NAN_METHOD(getImage)
{
	Nan::AsyncQueueWorker(new GetImageWorker(
				new Nan::Callback(info[0].As<v8::Function>()
					)));
}

NAN_METHOD(close)
{
	if(display){
		XCloseDisplay(display);
		display = NULL;
	}
}

NAN_MODULE_INIT(Init)
{
	Nan::Set(target, Nan::New("init").ToLocalChecked(),
			Nan::GetFunction(Nan::New<FunctionTemplate>(init)).ToLocalChecked());
	Nan::Set(target, Nan::New("getImage").ToLocalChecked(),
			Nan::GetFunction(Nan::New<FunctionTemplate>(getImage)).ToLocalChecked());
	Nan::Set(target, Nan::New("close").ToLocalChecked(),
			Nan::GetFunction(Nan::New<FunctionTemplate>(close)).ToLocalChecked());
}

NODE_MODULE(fastscreenshot, Init)
