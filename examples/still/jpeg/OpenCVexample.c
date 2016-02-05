/* 
Make sure you have installed and linked the OpenCV and OMXCam libraries.
*/

#include "omxcam.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>

using namespace cv;

int log_error() {
	omxcam_perror();
	return 1;
}


vector<uint8_t> buffer;


void on_data(omxcam_buffer_t omx_buffer) {
	for (uint ptr = 0; ptr < omx_buffer.length; ptr++) {
		buffer.push_back(omx_buffer.data[ptr]);
	}
}


int main() {

	//2592x1944 by default
	omxcam_still_settings_t settings;
	Mat builded_img;
	
	//Capture an image with default settings
	omxcam_still_init(&settings);

	settings.on_data = on_data;

	settings.camera.shutter_speed = 50000;
	settings.camera.iso = (omxcam_iso) 200;
	settings.jpeg.thumbnail.width = 0;
	settings.jpeg.thumbnail.height = 0;

	omxcam_still_start(&settings);
	builded_img = imdecode(buffer, CV_LOAD_IMAGE_COLOR); //needed for imwrite to work properly.
	printf(“Image captured\n”);
	imwrite("Default_test.jpg",builded_img); 
	buffer.clear();
	omxcam_still_stop();

	printf("ok\n");

	return 0;
}
