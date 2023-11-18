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
			int originalPosition = rowIndex * rowSize + element;
			int flippedPosition = flippedRowIndex * rowSize + element;

			flippedData[flippedPosition] = pRawData[originalPosition];
		}
	}

	return flippedData;
}

std::vector<float> Image::getChannelAverages() const
{
	std::vector<float> pixelValue(getChannelCount(), 0.0f);
	std::vector<float> channelValueSums(getChannelCount(), 0.0f);

	// compute the sum of all pixel values in each channel
	for(int y = 0; y < getHeight(); y++)
	{
		for(int x = 0; x < getWidth(); x++)
		{
			getValue(x, y, pixelValue);
			for(size_t channel = 0; channel < getChannelCount(); channel++)
			{
				channelValueSums[channel] += pixelValue[channel];
			}
		}
	}

	// calculate the averages of each channel
	std::vector<float> channelAverages(getChannelCount(), 0.0f);
	for(size_t channel = 0; channel < getChannelCount(); channel++)
	{
		float average = channelValueSums[channel] / getPixelCount();
		channelAverages[channel] = average;
	}

	return channelAverages;
}

std::vector<float> Image::getChannelRMSs() const
{

	std::vector<float> const channelAverages = getChannelAverages();
	size_t const channelCount = getChannelCount();

	std::vector<float> pixelValue(channelCount, 0.0f);

	// Calculate the sum of squared differences for RMS
	std::vector<float> sumsOfSquares(channelCount, 0.0f);
	for(int y = 0; y < getHeight(); y++)
	{
		for(int x = 0; x < getWidth(); x++)
		{
			getValue(x, y, pixelValue);
			for(size_t channel = 0; channel < channelCount; channel++)
			{
				float pixelChannelDeviation = pixelValue[channel] - channelAverages[channel];
				sumsOfSquares[channel] += pixelChannelDeviation * pixelChannelDeviation;
			}
		}
	}

	// Calculate the RMS for each channel
	std::vector<float> channelRMSs(channelCount, 0.0f);
	for(size_t channel = 0; channel < channelCount; channel++)
	{
		channelRMSs[channel] = std::sqrt(sumsOfSquares[channel] / getPixelCount());
	}

	return channelRMSs;
}

void Image::calculateHistograms(std::vector<std::vector<int>> & histograms, std::vector<float> & minValues, std::vector<float> & maxValues, int numBins) const
{
	size_t const channelCount = getChannelCount();
	size_t const height = getHeight();
	size_t const width = getWidth();

	std::vector<float> pixelValue(channelCount, 0.0f);

	histograms.assign(channelCount, std::vector<int>(numBins, 0));
	minValues.assign(channelCount, std::numeric_limits<float>::max());
	maxValues.assign(channelCount, std::numeric_limits<float>::lowest());

	// Find min and max values for each channel and fill histograms
	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			getValue(x, y, pixelValue);
			for(size_t channel = 0; channel < channelCount; channel++)
			{
				minValues[channel] = std::min(minValues[channel], pixelValue[channel]);
				maxValues[channel] = std::max(maxValues[channel], pixelValue[channel]);
				int binIndex = static_cast<int>((pixelValue[channel] - minValues[channel]) / (maxValues[channel] - minValues[channel]) * (numBins - 1));
				histograms[channel][binIndex]++;
			}
		}
	}
}

void Image::normalizeHistograms(std::vector<std::vector<int>> const & histograms, std::vector<std::vector<float>> & normalizedHistograms, int totalPixels) const
{
	size_t channelCount = histograms.size();
	int numBins = histograms[0].size();
	normalizedHistograms.assign(channelCount, std::vector<float>(numBins, 0.0f));

	for(size_t channel = 0; channel < channelCount; channel++)
	{
		for(int bin = 0; bin < numBins; bin++)
		{
			normalizedHistograms[channel][bin] = static_cast<float>(histograms[channel][bin]) / totalPixels;
		}
	}
}

void Image::computeCDFs(std::vector<std::vector<float>> const & normalizedHistograms, std::vector<std::vector<float>>& CDFs) const
{
    size_t channelCount = normalizedHistograms.size();
    int numBins = normalizedHistograms[0].size();
    CDFs.assign(channelCount, std::vector<float>(numBins, 0.0f));

    for(size_t channel = 0; channel < channelCount; channel++)
    {
        CDFs[channel][0] = normalizedHistograms[channel][0];
        for(int bin = 1; bin < numBins; bin++)
        {
            CDFs[channel][bin] = CDFs[channel][bin-1] + normalizedHistograms[channel][bin];
        }
    }
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
