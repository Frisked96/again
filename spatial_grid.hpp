#pragma once

#include <vector>
#include <cstddef>
#include <cassert>
#include <algorithm>
#include <type_traits>

template <typename T>
class SpatialGrid2D {
public:
    using value_type = T;
    using size_type  = std::size_t;

private:
    size_type width_  = 0;
    size_type height_ = 0;
    std::vector<T> data_;

    [[nodiscard]]
    size_type index(size_type x, size_type y) const noexcept {
        return y * width_ + x;
    }

public:
    SpatialGrid2D() = default;

    SpatialGrid2D(size_type width, size_type height)
        : width_(width),
          height_(height),
          data_(width * height) {}

    SpatialGrid2D(size_type width, size_type height, const T& value)
        : width_(width),
          height_(height),
          data_(width * height, value) {}

    // Dimensions
    [[nodiscard]]
    size_type width() const noexcept {
        return width_;
    }

    [[nodiscard]]
    size_type height() const noexcept {
        return height_;
    }

    [[nodiscard]]
    size_type size() const noexcept {
        return data_.size();
    }

    [[nodiscard]]
    bool empty() const noexcept {
        return data_.empty();
    }

    // Bounds check supporting both signed and unsigned integers safely
    template <typename IntT>
    [[nodiscard]]
    bool in_bounds(IntT x, IntT y) const noexcept {
        static_assert(std::is_integral_v<IntT>, "in_bounds coordinates must be integral");
        if constexpr (std::is_signed_v<IntT>) {
            return x >= 0 && y >= 0 &&
                   static_cast<size_type>(x) < width_ &&
                   static_cast<size_type>(y) < height_;
        } else {
            return static_cast<size_type>(x) < width_ &&
                   static_cast<size_type>(y) < height_;
        }
    }

    // Element access (non-const)
    template <typename IntT>
    T& operator()(IntT x, IntT y) noexcept {
        static_assert(std::is_integral_v<IntT>, "coordinates must be integral");
        assert(in_bounds(x, y));
        return data_[index(static_cast<size_type>(x), static_cast<size_type>(y))];
    }

    // Element access (const)
    template <typename IntT>
    const T& operator()(IntT x, IntT y) const noexcept {
        static_assert(std::is_integral_v<IntT>, "coordinates must be integral");
        assert(in_bounds(x, y));
        return data_[index(static_cast<size_type>(x), static_cast<size_type>(y))];
    }

    // Raw scanline row access for high-speed sequential processing
    [[nodiscard]]
    T* row_data(size_type y) noexcept {
        assert(y < height_);
        return data_.data() + (y * width_);
    }

    [[nodiscard]]
    const T* row_data(size_type y) const noexcept {
        assert(y < height_);
        return data_.data() + (y * width_);
    }

    // 1D Index to 2D Coordinates conversion
    void to_coords(size_type idx, size_type& out_x, size_type& out_y) const noexcept {
        assert(width_ > 0);
        out_x = idx % width_;
        out_y = idx / width_;
    }

    // Raw 1D vector
    std::vector<T>& data() noexcept {
        return data_;
    }

    const std::vector<T>& data() const noexcept {
        return data_;
    }

    // Clear / fill
    void fill(const T& value) {
        std::fill(data_.begin(), data_.end(), value);
    }

    void resize(size_type width, size_type height) {
        width_ = width;
        height_ = height;
        data_.resize(width * height);
    }

    void resize(size_type width, size_type height, const T& value) {
        width_ = width;
        height_ = height;
        data_.assign(width * height, value);
    }
};
