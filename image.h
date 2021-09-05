#ifndef IMAGE_H_
#define IMAGE_H_

#include "common.h"

// RGB24 Image Format

class Image {
  public:
    // Allocs and de-allocs in ctor and dtor.
    Image(int cols, int rows);
    ~Image();

    uint8_t* Row(int row);

    // Returns a pixel channel value that can be changed.
    uint8_t& At(int col, int row, int channel);
    uint8_t& operator()(int col, int row, int channel) { return At(col, row, channel); }

    struct Pixel { uint8_t r; uint8_t g; uint8_t b; };
    void SetPixel(int col, int row, const Pixel& pix);
    void SetAll(uint8_t value);

    // Gets a cropped set of rows. Useful for overscan.
    //  GetCroppedView(0, 240) will give you data for 240 rows.
    struct CropInfo {
      uint8_t* data;
      size_t length;     // length of data
      size_t row_width;  // unchanged from Image::RowWidth
    };
    CropInfo GetCroppedView(int first_row, int num_rows);

    int Cols() { return cols_; }
    int Rows() { return rows_; }
    // Row width in bytes.
    size_t RowWidth() { return cols_ * 3 * sizeof(uint8_t); }

    // The size of the output buffer is exactly Rows() * RowWidth().
    uint8_t* Data() { return data_; }

  private:
    int cols_;
    int rows_;

    uint8_t* data_;
};

#endif 