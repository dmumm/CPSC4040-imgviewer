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
    glutPostRedisplay();
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
    , title(string("Basic Viewer"))
    , mouse_x(0)
    , mouse_y(0)
    , frame(0)
{

    cout << "BasicViewer Loaded\n";
}

BasicViewer::~BasicViewer() {}

void BasicViewer::Init(StringVector const & args)
{

    int argc = (int)args.size();
    char ** argv = new char *[argc];
    for (int i = 0; i < argc; i++) {
        argv[i] = new char[args[i].length() + 1];
        strcpy(argv[i], args[i].c_str());
    }
    string window_title = title;
    glutInit(&argc, argv);
    glutInitDisplayMode(display_mode);
    glutInitWindowSize(width, height);
    glutCreateWindow(window_title.c_str());
    glClearColor(0.5, 0.5, 0.6, 0.0);

    glutDisplayFunc(&cbDisplayFunc);
    glutIdleFunc(&cbIdleFunc);
    glutKeyboardFunc(&cbKeyboardFunc);


    initialized = true;
    cout << "BasicViewer Initialized\n";
}

void BasicViewer::Init(StringVector const & args, UserInput const details)
{

    int argc = (int)args.size();
    char ** argv = new char *[argc];
    for (int i = 0; i < argc; i++) {
        argv[i] = new char[args[i].length() + 1];
        strcpy(argv[i], args[i].c_str());
    }
    string window_title = details.filename;
    glutInit(&argc, argv);
    glutInitDisplayMode(display_mode);
    glutInitWindowSize(details.pImage->getWidth(), details.pImage->getHeight());
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
    if (pImage->getChannelCount() == 3) {
        glDrawPixels(pImage->getWidth(), pImage->getHeight(), GL_RGB, GL_FLOAT, pImage->getRawData());
    }
    else {
        glDrawPixels(pImage->getWidth(), pImage->getHeight(), GL_RGBA, GL_FLOAT, pImage->getRawData());
    }
}

void BasicViewer::Keyboard(unsigned char key, int x, int y)
{

    switch (key) {

    case 'j':
        // save image as jpeg to file in same directory
        pImage->write("demowritetoafile.jpg");
        break;
    }
}

void BasicViewer::Idle() {}

void BasicViewer::Usage()
{

    cout << "--------------------------------------------------------------\n";
    cout << "BasicViewer usage:\n";
    cout << "--------------------------------------------------------------\n";
    cout << "j/J           writes image to demowritetoafile.jpg\n";
    cout << "--------------------------------------------------------------\n";
}

BasicViewer * CreateViewer()
{

    return BasicViewer::Instance();
}

} // namespace viewer
