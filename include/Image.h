#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <vector>
#include <fstream> // Required to check if file exists


namespace image {

class Image {

  public:

	// ImgProc();
	Image();

	// ~ImgProc();
	~Image();

	void clear();
	void clear(int width, int height, int channelCount);

	bool load(std::string const & filename);

	bool write(std::string const & baseName, std::string & outputName) const;

	int getWidth() const
	{
		return width;
	}

	int getHeight() const
	{
		return height;
	}

	int getChannelCount() const
	{
		return channelCount;
	}

	long getNumElements() const
	{
		return numElements;
	}

	size_t getPixelCount() const
	{
		return getWidth() * getHeight();
	}

	float * getRawData() const
	{
		return pRawData;
	} // img_data(), retrive pointer to the raw data

	void getValue(int iCol, int jRow, std::vector<float> & pixel) const;
	void setValue(int iCol, int jRow, std::vector<float> const & pixel);
	//  void interpolated_value( float iCol, float jRow, std::vector<float>& pixel) const;

	Image(Image const & imageTwo);
	Image & operator= (Image const & imageTwo);

	//  friend void swap(Image& imageOne, Image& imageTwo);

	//  void operator*=(float value);
	//  void operator/=(float value);
	//  void operator+=(float value);
	//  void operator-=(float value);
	//  void operator-=(const Image& imageTwo);
	//  void operator+=(const Image& imageTwo);

	//  void compliment();
	//  void negative();
	//  void pow(float gamma);
	//  void quantize(int levels);
	//  void grayscale();
	//  void rms_contrast();
	//  void demean();
	//  void log();
	//  void exp();

	//  void flip();
	//  void flop();

	//  void tanh( const std::vector<float> rms);
	//  void quench();
	//  void logquench();

	//  void rgb_to_hsv();
	//  void hsv_to_rgb();
	//  void rgb_to_cmyk();
	//  void cmyk_to_rgb(const float K);

	//  void edge_taper(int nb);

	long index(int iCol, int jRow, int channel) const; // i, j, c
	// long index(int iCol, int jRow) const; // i, j

	// void printPixelValues(std::vector<float> const &) const;

	// bool isValid() const;

	float * getVerticallyFlippedData() const;

    std::vector<float> getChannelAverages() const;
    std::vector<float> getChannelRMSs() const;

  private:

	int width, height, channelCount; // Nx, Ny, Nc
	long numElements; // Nsize (width * height * channelCount)
	float * pRawData; // img_data
};

//  void swap(Image& imageOne, Image& imageTwo);

} // namespace image
#endif // IMAGE_H
