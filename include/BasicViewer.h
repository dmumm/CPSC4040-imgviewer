//-------------------------------------------------------
//
//  BasicViewer.h
//
//  This viewer is a wrapper of the Glut calls needed to
//  display opengl data.  Options for zooming,
//  labeling the window frame, etc are available for
//  derived classes to use.
//
//
//
//  Copyright (c) 2003,2017,2023 Jerry Tessendorf
//
//
//--------------------------------------------------------


#ifndef BASIC_VIEWER_H
#define BASIC_VIEWER_H

#include "Image.h"
#include "FractalSet.h"
#include "ImageProcessor.h"

#include <string>
#include <unordered_map>
#include <vector>

using StringVector = std::vector<std::string>;
using StringMap = std::unordered_multimap<std::string, std::string>;

struct UserInput {
	std::string programCall;
	std::string flag;
	std::string imagePath;
	std::string filename;
	std::string imageTitle;
	std::string fileExtension;
	std::string imageFormat;
	image::Image image;
};

namespace viewer {

class BasicViewer {
  public:

	//! The viewer is a singleton
	static BasicViewer * Instance()
	{
		if (pBasicViewer == nullptr) {
			pBasicViewer = new BasicViewer();
		}
		return pBasicViewer;
	}

	~BasicViewer();

	//! Initialization, including GLUT initialization.
	void Init(StringVector const & args);

	//! Initialization, including GLUT initialization with data.
	void Init(StringVector const & args, UserInput const details);

	//! Invokes the GLUT main loop.
	void MainLoop();

	//! Set the window width
	void SetWidth(int const w)
	{
		width = w;
	}

	//! Set the window height
	void SetHeight(int const h)
	{
		height = h;
	}

	//! Get the window width
	int const & GetWidth()
	{
		return width;
	}

	//! Get the window height
	int const & GetHeight()
	{
		return height;
	}

	//! Set the window title
	void SetTitle(std::string const & t)
	{
		title = t;
	}

	//! Set the window title
	void SetTitle(char const * t)
	{
		title = t;
	}

	//! Get the window title
	std::string const & GetTitle()
	{
		return title;
	}

	void setImage(image::Image & p)
	{
		displayedImage = p;
	}

	image::Image const & getImage()
	{
		return displayedImage;
	}

	// Callback functions
	//! Cascading callback for initiating a display event
	void Display();
	//! Cascading callback for a keyboard event
	void Keyboard(unsigned char key, int x, int y);
	//! Cascading callback for an idle  event
	void Idle();

	// //! Get the current frame
	// int GetFrame() const
	// {
	// 	return frame;
	// }

	//! Cascading callback for usage information
	void Usage();

  private:

	bool initialized;
	int width, height;
	unsigned int display_mode;

	std::string title;
	int mouse_x, mouse_y;
	int keystate, button;
	int mouse_state;
	float current_raster_pos[4]; // TODO: what does this do

	image::Image displayedImage;

	static BasicViewer * pBasicViewer;

	// Declared private to prevent additional instances
	BasicViewer();
	BasicViewer(BasicViewer const &);
	BasicViewer & operator=(BasicViewer const &);
};

BasicViewer * CreateViewer();


} // namespace viewer


#endif // BASIC_VIEWER_H
