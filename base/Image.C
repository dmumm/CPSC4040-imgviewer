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
    if (pRawData) {
        delete[] pRawData;
        pRawData = nullptr;
    }
    width = 0;
    height = 0;
    channelCount = 0;
    numElements = 0;
}

Image * Image::load(std::string const & filename)
{

    auto input = ImageInput::open(filename);
    if (! input) {
        return nullptr;
    }

    new Image();

    ImageSpec const & spec = input->spec();
    int newWidth = spec.width;
    int newHeight = spec.height;
    int newChannelCount = spec.nchannels;

    clear(newWidth, newHeight, newChannelCount);

    pRawData = new float[numElements];

    input->read_image(TypeDesc::FLOAT, pRawData);

    input->close();

    return this;
}

bool Image::write(std::string const & filename) const
{
    if (pRawData == nullptr) {
        return false;
    }

    auto output = ImageOutput::create(filename);
    if (! output) {
        return false;
    }

    ImageSpec spec(width, height, channelCount, TypeDesc::FLOAT);

    output->open(filename, spec);
    output->write_image(TypeDesc::FLOAT, pRawData);
    output->close();

    return true;
}

// clean, then set new dimensions TODO: change name
void Image::clear(int newWidth, int newHeight, int newChannelCount)
{
    clear();
    width = newWidth;
    height = newHeight;
    channelCount = newChannelCount;
    numElements = (long)width * (long)height * (long)channelCount;

    pRawData = new float[numElements];

#   pragma omp parallel for
    for (long i = 0; i < numElements; i++) {
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
    for (long i = 0; i < numElements; i++) {
        pRawData[i] = imageToCopy.pRawData[i];
    }
}

Image::~Image()
{
    clear();
}

Image & Image::operator=(Image const & imageToCopy)
{
    if (this == &imageToCopy) {
        return *this;
    }
    if (width != imageToCopy.width
        || height != imageToCopy.height
        || channelCount != imageToCopy.channelCount) {
        clear(imageToCopy.width, imageToCopy.height, imageToCopy.channelCount);
#       pragma omp parallel for
        for (long i = 0; i < numElements; i++) {
            pRawData[i] = imageToCopy.pRawData[i];
        }
    }
    return *this;
}

// returns requested value from image dimensions into inputted pixel vector
// TODO: why not just return the vector?
void Image::getValue(int iCol, int jRow, std::vector<float> & pixel) const
{
    pixel.clear();

    if (pRawData == nullptr
        || iCol < 0 || iCol >= width
        || jRow < 0 || jRow >= height) {
        return;
    }
    pixel.resize(channelCount);
    for (int channel = 0; channel < channelCount; channel++) {
        pixel[channel] = pRawData[index(iCol, jRow, channel)];
    }
    return;
}

// assigns requested set of values from given pixel indices into inputted pixel vector
void Image::setValue(int iCol, int jRow, std::vector<float> const & pixel)
{
    if (pRawData == nullptr
        || iCol < 0 || iCol >= width
        || jRow < 0 || jRow >= height
        || channelCount > (int)pixel.size()) {
        throw std::invalid_argument("Invalid row, col, or channel value");
    }

#pragma omp parallel for
    for (int channel = 0; channel < channelCount; channel++) {
        pRawData[index(iCol, jRow, channel)] = pixel[channel];
    }
    return;
}

// interleaved index
long Image::index(int iCol, int jRow, int channel) const
{
    if (pRawData == nullptr
        || iCol < 0 || iCol >= width
        || jRow < 0 || jRow >= height) {
        throw std::invalid_argument("Invalid row, col, or channel value");
    }

    long rowMultiplier = long(jRow)
                       * long(width);

    long pixelMultiplier = long(iCol)
                         + rowMultiplier;

    long targetIndex = pixelMultiplier
                     * long(channelCount);

    assert(targetIndex >= 0
           && targetIndex < numElements);

    return targetIndex;
}
