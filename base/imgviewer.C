#include "BasicViewer.h"
#include "Image.h"

#include <OpenImageIO/imageio.h>
#include <iostream>
#include <sstream>

using namespace OIIO;
using namespace viewer;
using namespace image;

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

UserInput processArgs(int argc, StringVector const args);
StringMap supportedFormats();
StringVector splitString(string const & input, char const & delimiter);

int main(int argc, char ** argv)
{
	StringVector args;
	int fixedArgc = argc;
	for(int i = 0; i < argc; i++) {
		if(*argv[i] == false) {
			fixedArgc--;
			continue;
		}
		args.push_back(argv[i]);
	}
	argc = fixedArgc;

	UserInput userRequest = processArgs(argc, args);

	Image inputImage;
	if(! inputImage.load(userRequest.imagePath)) {
		cerr << "ERROR: Image could not be processed.\n";
		exit(EXIT_FAILURE);
	}

	userRequest.image = inputImage;

	BasicViewer * viewer = CreateViewer();
	viewer->Init(args, userRequest);

	viewer->setImage(userRequest.image);
	viewer->SetTitle(userRequest.imageTitle);

	viewer->MainLoop();

	return 0;
}

UserInput processArgs(int const argc, StringVector const rawArgs)
{

	if(argc != 3) {
		cerr << "ERROR: Incorrect number of arguments: " << argc - 1 << "\n";
		cerr << "Usage: ./imgviewer -image <image_name>.<extension>\n";
		exit(EXIT_FAILURE);
	}

	if(rawArgs[1] != "-image") {
		cerr << "ERROR: -image flag not detected!\n";
		cerr << "Usage: ./imgviewer -image <image_name>.<extension>\n";
		exit(EXIT_FAILURE);
	}

	string const requestedImagePath = rawArgs[2];
	// ../images/trollface.edited.png

	StringVector directorySplit = splitString(requestedImagePath, '/');
	// ..
	// images
	// trollface.edited.png

	string const processedFilename = directorySplit.back();
	// trollface.edited.png

	StringVector filenameSplit = splitString(processedFilename, '.');
	// trollface
	// edited
	// png

	if(filenameSplit.size() == 1) {
		cerr << "ERROR: No image extension detected. Incorrect filename format.\n";
		cerr << "Usage: ./imgviewer -image <image_name>.<extension>\n";
		exit(EXIT_FAILURE);
	}

	string const processedExtension = filenameSplit.back();
	// png

	filenameSplit.pop_back();
	string processedTitle = filenameSplit[0];
	if(filenameSplit.size() > 1) {

		cerr << "Warning: Double-extension detected, assuming final extension is for image format\n";
		cerr << "Usage: ./imgviewer -image <image_name>.<extension>\n";

		for(size_t i = 1; i < filenameSplit.size(); i++) {
			processedTitle = processedTitle + "." + filenameSplit[i];
			// trollface.edited
		}
	}

	// Check if the extension is supported
	// Parses map of valid extensions for supported formats
	// If user's extension is found, returns processed arguments
	StringMap usableExtensions = supportedFormats();
	for(auto const & format : usableExtensions)
        {
		if(format.second == processedExtension)
                {
			cout << "Processed image title: " << processedTitle << "\n";
			cout << "Supported extension detected: " << format.second << "\n";
			cout << "Belongs to image format \"" << format.first << "\"\n";
			return UserInput{
				rawArgs[0], // programCall
				rawArgs[1], // flag
				rawArgs[2], // imagePath
				processedFilename, // filename
				processedTitle, // imageTitle
				format.first, // imageFormat
				format.second // fileExtension
			};
		}
	}

	// If user's extension is not found, exit
	cerr << "ERROR: Unsupported image extension detected.\n";
	cerr << "Usage: ./imgviewer -image <image_name>.<extension>\n";
	exit(EXIT_FAILURE);
}

StringMap supportedFormats()
{

	string formats = OIIO::get_string_attribute("extension_list");
	// bmp:bmp;cineon:cin;hdr:hdr,rgbe
	// The basic format is semicolon-separated lists of formats,
	// where each format consists of the format name,
	// a colon, and then a comma-separated list of extensions
	// that it assumes are associated with the formats.

	char const TYPE_DELIMITER = ';';
	char const NAME_DELIMITER = ':';
	char const EXTENSION_DELIMITER = ',';

	StringVector formatSplit = splitString(formats, TYPE_DELIMITER);
	// bmp:bmp
	// cineon:cin
	// dds:dds
	// dpx:dpx
	// hdr:hdr,rgbe

	StringMap formatMap;

	for(size_t i = 0; i < formatSplit.size(); i++) {

		StringVector nameAndExtensions = splitString(formatSplit[i], NAME_DELIMITER);
		// hdr
		// hdr,rgbe

		string formatName = nameAndExtensions[0], // "hdr"
		rawExtensions = nameAndExtensions[1]; // "hdr,rgbe"

		StringVector formatExtensions = splitString(rawExtensions, EXTENSION_DELIMITER);
		// hdr
		// rgbe

		for(size_t j = 0; j < formatExtensions.size(); j++) {
			formatMap.insert({ formatName, formatExtensions[j] });
			// formatMap["hdr"] = "rgbe"
		}
	}

	return formatMap;
	/*
	   "bmp" : "bmp"
	   "cin" : "cin"
	   "dds" : "dds"
	   "dpx" : "dpx"
	   "hdr" : "hdr" "rgbe"
	*/
}

StringVector splitString(string const & input, char const & delimiter)
{
	std::istringstream remaining(input);
	string token = input;
	vector<string> tokens;

	while(getline(remaining, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}
