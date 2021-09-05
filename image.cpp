#include "image.h"
#include "common.h"

Image::Image(int cols, int rows) {
  data_ = static_cast<uint8_t*>(std::malloc(cols * rows * 3 * sizeof(uint8_t)));
  cols_ = cols;
  rows_ = rows;
}

uint8_t* Image::Row(int row) {
  if (row < 0 || row >= rows_) {
    throw std::runtime_error("Row out of bounds.");
  }
  return &data_[row * RowWidth()];
}

uint8_t& Image::At(int col, int row, int channel) {
  if (col < 0 || col >= cols_) {
    throw std::runtime_error("Column out of bounds.");
  } else if (channel < 0 || channel > 2) {
    throw std::runtime_error("Channel out of bounds.");
  }
  return Row(row)[(col * 3) + channel];
}

void Image::SetPixel(int col, int row, const Pixel& pix) {
  At(col, row,  0) = pix.r;
  At(col, row,  1) = pix.g;
  At(col, row,  2) = pix.b;
}

void Image::SetAll(uint8_t value) {
  std::memset(data_, value, rows_ * RowWidth());
}

Image::~Image() {
  free(data_);
}