#include "Image.h"

#include <OpenImageIO/imageio.h>

using namespace OIIO;
using namespace image;

Image::Image()
: width(0)
, height(0)
, channelCount(0)
, numElements(0)
, pRawData(nullptr)
{
}

void Image::clear()
{
	if(pRawData)
	{
		delete[] pRawData;
		pRawData = nullptr;
	}
	width = 0;
	height = 0;
	channelCount = 0;
	numElements = 0;
}

bool Image::load(std::string const & filename)
{
	auto input = ImageInput::open(filename);
	if(! input) return false;

	ImageSpec const & spec = input->spec();
	this->width = spec.width;
	this->height = spec.height;
	this->channelCount = spec.nchannels;
	this->numElements = this->width * this->height * this->channelCount;

	this->pRawData = new float[this->numElements];

	input->read_image(TypeDesc::FLOAT, this->pRawData);

	input->close();

	return true;
}

bool file_exists(std::string const & name)
{
	std::ifstream file_stream(name.c_str());
	bool existence = file_stream.good();
	return existence;
}

bool Image::write(std::string const & baseName, std::string & outputName) const
{
	if(pRawData == nullptr) return false;

	std::string const EXTENSION = ".jpg";

	outputName = baseName + EXTENSION;
	while(file_exists(outputName))
		outputName.insert(outputName.size() - EXTENSION.size(), ".edited");


	auto output = ImageOutput::create(outputName);
	if(! output) return false;

	ImageSpec spec(width, height, channelCount, TypeDesc::FLOAT);
	output->open(outputName, spec);
	output->write_image(TypeDesc::FLOAT, pRawData);
	output->close();

	return true;
}

void Image::clear(int newWidth, int newHeight, int newChannelCount)
{
	clear();
	width = newWidth;
	height = newHeight;
	channelCount = newChannelCount;
	numElements = (long)width * (long)height * (long)channelCount;

	pRawData = new float[numElements];

#pragma omp parallel for
	for(long i = 0; i < numElements; i++)
	{
		pRawData[i] = 0.0;
	}
}

Image::Image(Image const & imageToCopy)
: width(imageToCopy.width)
, height(imageToCopy.height)
, channelCount(imageToCopy.channelCount)
, numElements(imageToCopy.numElements)
{
	pRawData = new float[numElements];

#pragma omp parallel for
	for(long i = 0; i < numElements; i++)
	{
		pRawData[i] = imageToCopy.pRawData[i];
	}
}

Image::~Image()
{
	clear();
}

Image & Image::operator= (Image const & rhsImage)
{
	if(this == &rhsImage)
	{
		return *this;
	}

	if(width != rhsImage.width
	|| height != rhsImage.height
	|| channelCount != rhsImage.channelCount)
	{
		clear(rhsImage.width, rhsImage.height, rhsImage.channelCount);
	}

#pragma omp parallel for
	for(long i = 0; i < numElements; i++)
	{
		pRawData[i] = rhsImage.pRawData[i];
	}

	return *this;
}

void Image::getValue(int iCol, int jRow, std::vector<float> & pixel) const
{
	pixel.clear();

	if(pRawData == nullptr
	|| iCol < 0 || iCol >= width
	|| jRow < 0 || jRow >= height)
	{
		std::cerr << "Could not get value at (" << iCol << ", " << jRow << ")\n";
		return;
	}

	pixel.resize(channelCount);
	for(int channel = 0; channel < channelCount; channel++)
	{
		pixel[channel] = pRawData[index(iCol, jRow, channel)];
	}
	return;
}

// assigns requested set of values from given pixel indices into inputted pixel
// vector
void Image::setValue(int iCol, int jRow, std::vector<float> const & pixel)
{
	if(pRawData == nullptr
	|| iCol < 0 || iCol >= width
	|| jRow < 0 || jRow >= height
	|| channelCount > (int)(pixel.size()))
	{
		std::cerr << "Could not set value at (" << iCol << ", " << jRow << ")\n";
		return;
	}

#pragma omp parallel for
	for(int channel = 0; channel < channelCount; channel++)
	{
		pRawData[index(iCol, jRow, channel)] = pixel[channel];
	}
	return;
}

// interleaved index
long Image::index(int iCol, int jRow, int channel) const
{
	if(pRawData == nullptr
	|| iCol < 0 || iCol >= width
	|| jRow < 0 || jRow >= height)
	{
		std::cerr << "Could not get element index for channel at (";
		std::cerr << iCol << ", " << jRow << ", " << channel << ")\n";
	}

	long channelMultiplier = long(channelCount);
	long rowMultiplier = long(jRow) * long(width);
	long pixelMultiplier = long(iCol) + rowMultiplier;

	long pixelIndex = pixelMultiplier * channelMultiplier;
	long channelIndex = pixelIndex + (long)channel;

	return channelIndex;
}

float * Image::getVerticallyFlippedData() const
{
	int const rowSize = width * channelCount;
	int const topIndex = height - 1;

	float * flippedData = new float[numElements];

	for(int rowIndex = 0; rowIndex < height; rowIndex++)
	{
		int flippedRowIndex = topIndex - rowIndex;
		for(int element = 0; element < rowSize; element++)
		{
                        int originalPosition =        rowIndex * rowSize + element;
                        int flippedPosition  = flippedRowIndex * rowSize + element;

			flippedData[flippedPosition] = pRawData[originalPosition];
		}
	}

	return flippedData;
}

// void Image::printPixelValues(std::vector<float> const & pixel) const
// {

// 	std::cout << "(";
// 	for(int channelIndex = 0; channelIndex < (int)(pixel.size()); channelIndex++)
// 	{
// 		std::cout << std::fixed << std::setprecision(4)
// 			  << pixel[channelIndex];

// 		if(channelIndex < channelCount - 1)
// 		{
// 			std::cout << ",";
// 		}
// 	}
// 	std::cout << ") ";

// 	return;
// }

// bool Image::isValid() const
// {

// 	if(! pRawData
// 	|| width <= 0
// 	|| height <= 0
// 	|| channelCount <= 0
// 	|| numElements <= 0)
// 	{
// 		return false;
// 	}

// 	return true;
// }
