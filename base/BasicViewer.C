//-------------------------------------------------------
//
//  BasicViewer.C
//
//  This viewer is a wrapper of the Glut calls needed to
//  display opengl data.  Options for zooming,
//  labeling the window frame, etc are available for
//  derived classes to use.
//
//
//  Copyright (c) 2003,2017,2023 Jerry Tessendorf
//
//--------------------------------------------------------


#include "BasicViewer.h"

#include <GL/gl.h> // OpenGL itself.
#include <GL/glu.h> // GLU support library.
#include <GL/glut.h> // GLUT support library.
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>

using namespace std;
using namespace image;

namespace viewer {

void cbDisplayFunc()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	BasicViewer::Instance()->Display();
	glutSwapBuffers();
}

void cbIdleFunc()
{
	BasicViewer::Instance()->Idle();
}

void cbKeyboardFunc(unsigned char key, int x, int y)
{
	BasicViewer::Instance()->Keyboard(key, x, y);
}

BasicViewer * BasicViewer::pBasicViewer = nullptr;

BasicViewer::BasicViewer()
: initialized(false)
, width(512)
, height(512)
, display_mode(GLUT_DOUBLE | GLUT_RGBA)
, title(string("Image Viewer"))
, mouse_x(0)
, mouse_y(0)
{
	cout << "Display Window Loaded\n";
}

BasicViewer::~BasicViewer() {}

void BasicViewer::Init(StringVector const & args)
{

	int argc = (int)args.size();
	char ** argv = new char *[argc];
	for(int i = 0; i < argc; i++) {
		argv[i] = new char[args[i].length() + 1];
		strcpy(argv[i], args[i].c_str());
	}
	string window_title = title;
	glutInit(&argc, argv);
	glutInitDisplayMode(display_mode);
	glutInitWindowSize(width, height);
	glutCreateWindow(window_title.c_str());
	glClearColor(1.0, .82, 0.7, 0.0);

	glutDisplayFunc(&cbDisplayFunc);
	glutIdleFunc(&cbIdleFunc);
	glutKeyboardFunc(&cbKeyboardFunc);

	initialized = true;
	// cout << "BasicViewer Initialized\n";
}

void BasicViewer::Init(StringVector const & args, UserInput const details)
{

	int argc = (int)args.size();
	char ** argv = new char *[argc];
	for(int i = 0; i < argc; i++)
	{
		argv[i] = new char[args[i].length() + 1];
		strcpy(argv[i], args[i].c_str());
	}
	string window_title = details.filename;

	glutInit(&argc, argv);
	glutInitDisplayMode(display_mode);
	glutInitWindowSize(details.image.getWidth(), details.image.getHeight());
	glutCreateWindow(window_title.c_str());
	glClearColor(0.5, 0.5, 0.6, 0.0);

	glutDisplayFunc(&cbDisplayFunc);
	glutIdleFunc(&cbIdleFunc);
	glutKeyboardFunc(&cbKeyboardFunc);

	initialized = true;
	cout << "BasicViewer Initialized\n";
}

void BasicViewer::MainLoop()
{
	Usage();
	glutMainLoop();
}

void BasicViewer::Display()
{

	if(! initialized) {
		cerr << "ERROR: BasicViewer not initialized\n";
		exit(EXIT_FAILURE);
	}

	float * flippedData = displayedImage.getVerticallyFlippedData();

	switch(displayedImage.getChannelCount()) {

	case 1: {
		cerr << "WARNING: imgviewer does not support 2-channel images!\n";
		glDrawPixels(displayedImage.getWidth(), displayedImage.getHeight(), GL_LUMINANCE, GL_FLOAT, flippedData);
		return;
	}

	case 2: {
		cerr << "WARNING: imgviewer does not support 2-channel images!\n";
		glDrawPixels(displayedImage.getWidth(), displayedImage.getHeight(), GL_LUMINANCE_ALPHA, GL_FLOAT, flippedData);
		return;
	}

	case 3: {
		glDrawPixels(displayedImage.getWidth(), displayedImage.getHeight(), GL_RGB, GL_FLOAT, flippedData);
		return;
	}

	case 4: {
		cerr << "WARNING: imgviewer does not support 4-channel images!\n";
		glDrawPixels(displayedImage.getWidth(), displayedImage.getHeight(), GL_RGBA, GL_FLOAT, flippedData);
		return;
	}
	} // end switch
}

void BasicViewer::Keyboard(unsigned char key, int x, int y)
{

	switch(key) {

	case 'j': // Fall through to J
	case 'J': {
		string const base_name = GetTitle();

		string actual_filename;
		displayedImage.write(base_name, actual_filename);

		cout << "Wrote displayed image to file: " << actual_filename << "\n";
		break;
	}

	case 'g': {
		float const GAMMA_DECREASE = 0.9;

		Image tempImage(displayedImage);
		ImageProcessor::applyGamma(GAMMA_DECREASE, tempImage);
		displayedImage = tempImage;

		glutPostRedisplay();
		cout << "Gamma of " << GAMMA_DECREASE << " applied to image\n";
		break;
	}

	case 'G': {
		float const GAMMA_INCREASE = 1.111111;

		Image tempImage(displayedImage);
		ImageProcessor::applyGamma(GAMMA_INCREASE, tempImage);

		displayedImage = tempImage;
		glutPostRedisplay();

		cout << "Gamma of " << GAMMA_INCREASE << " applied to image\n";
		break;
	}

	case 's': {

		Stencil myStencil;

		Image tempImage;
		ImageProcessor::doBoundedLinearConvolution(myStencil, displayedImage, tempImage);

		displayedImage = tempImage;
		glutPostRedisplay();

		cout << "Bounded Linear Convolution applied\n";
		break;
	}

	case 'w': {

		Stencil myStencil;

		Image tempImage;
		ImageProcessor::doCircularLinearConvolution(myStencil, displayedImage, tempImage);

		displayedImage = tempImage;
		glutPostRedisplay();

		cout << "Circular Linear Convolution applied\n";
		break;
	}
	} // end switch
}

void BasicViewer::Idle()
{
}

void BasicViewer::Usage()
{
	cout << "--------------------------------------------------------------\n";
	cout << "BasicViewer usage:\n";
	cout << "--------------------------------------------------------------\n";
	cout << "j/J    writes image to demowritetoafile.jpg\n";
	cout << "g/G    decreases/increases gamma by 10%\n";
	cout << "s      applies stencil with bounded linear convolution\n";
	cout << "w      applies stencil with circular linear convolution\n";
	cout << "--------------------------------------------------------------\n";
}

BasicViewer * CreateViewer()
{
	return BasicViewer::Instance();
}

} // namespace viewer
