#pragma once
#ifndef NIBGFXMATH_H
#define NIBGFXMATH_H

#include <bx/math.h>

#include <cstddef>
#include <cstring>
#include <type_traits>

// Small POD wrappers around the representation expected by bx/bgfx.
// bx intentionally exposes Vec3/Quaternion as concrete types, while generic
// vec4 values and matrices are passed as contiguous float[4]/float[16] data.
// These wrappers retain the layout/indexing needed by the legacy Gamebryo
// constant-map code without depending on DirectXMath, D3DX, or DirectXTK.
namespace NiBgfxMath
{
    struct Vec4
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 0.0f;

        constexpr Vec4() noexcept = default;
        constexpr explicit Vec4(float value) noexcept
            : x(value), y(value), z(value), w(value)
        {
        }
        constexpr Vec4(float xValue, float yValue,
            float zValue, float wValue) noexcept
            : x(xValue), y(yValue), z(zValue), w(wValue)
        {
        }

        float* data() noexcept { return &x; }
        const float* data() const noexcept { return &x; }

        float& operator[](std::size_t index) noexcept { return data()[index]; }
        const float& operator[](std::size_t index) const noexcept
        {
            return data()[index];
        }

        operator float*() noexcept { return data(); }
        operator const float*() const noexcept { return data(); }
    };

    struct Mat4
    {
        // bx/bgfx matrices are contiguous float[16] values. Keeping that exact
        // representation also makes the data directly consumable by bgfx.
        float m[16];

        Mat4() noexcept
        {
            bx::mtxIdentity(m);
        }

        explicit Mat4(const float* values) noexcept
        {
            if (values)
                std::memcpy(m, values, sizeof(m));
            else
                bx::mtxIdentity(m);
        }

        float* data() noexcept { return m; }
        const float* data() const noexcept { return m; }

        float& operator[](std::size_t index) noexcept { return m[index]; }
        const float& operator[](std::size_t index) const noexcept
        {
            return m[index];
        }

        operator float*() noexcept { return m; }
        operator const float*() const noexcept { return m; }
    };

    struct alignas(16) Mat4A : public Mat4
    {
        Mat4A() noexcept = default;
        explicit Mat4A(const float* values) noexcept : Mat4(values) {}
        Mat4A(const Mat4& value) noexcept
        {
            std::memcpy(m, value.m, sizeof(m));
        }

        Mat4A& operator=(const Mat4& value) noexcept
        {
            if (this != &value)
                std::memcpy(m, value.m, sizeof(m));
            return *this;
        }
    };

    static_assert(sizeof(Vec4) == sizeof(float) * 4,
        "bgfx vec4 uniform data must remain four floats");
    static_assert(sizeof(Mat4) == sizeof(float) * 16,
        "bx matrix data must remain sixteen floats");
    static_assert(sizeof(Mat4A) == sizeof(float) * 16,
        "aligned bx matrix data must remain sixteen floats");
    static_assert(alignof(Mat4A) >= 16,
        "aligned bx matrix data must remain at least 16-byte aligned");
    static_assert(std::is_standard_layout_v<Vec4>);
    static_assert(std::is_standard_layout_v<Mat4>);
    static_assert(std::is_standard_layout_v<Mat4A>);
    static_assert(std::is_trivially_copyable_v<Vec4>);
    static_assert(std::is_trivially_copyable_v<Mat4>);
    static_assert(std::is_trivially_copyable_v<Mat4A>);

    inline Vec4 operator+(const Vec4& left, const Vec4& right) noexcept
    {
        return Vec4(left.x + right.x, left.y + right.y,
            left.z + right.z, left.w + right.w);
    }

    inline Vec4 operator-(const Vec4& left, const Vec4& right) noexcept
    {
        return Vec4(left.x - right.x, left.y - right.y,
            left.z - right.z, left.w - right.w);
    }

    inline Vec4 operator*(const Vec4& left, const Vec4& right) noexcept
    {
        return Vec4(left.x * right.x, left.y * right.y,
            left.z * right.z, left.w * right.w);
    }

    inline Vec4 operator*(const Vec4& vector, float scalar) noexcept
    {
        return Vec4(vector.x * scalar, vector.y * scalar,
            vector.z * scalar, vector.w * scalar);
    }

    inline Vec4 operator*(float scalar, const Vec4& vector) noexcept
    {
        return vector * scalar;
    }

    inline Vec4 operator/(const Vec4& left, const Vec4& right) noexcept
    {
        return Vec4(left.x / right.x, left.y / right.y,
            left.z / right.z, left.w / right.w);
    }

    inline Vec4 operator/(const Vec4& vector, float scalar) noexcept
    {
        return Vec4(vector.x / scalar, vector.y / scalar,
            vector.z / scalar, vector.w / scalar);
    }

    inline Vec4 operator/(float scalar, const Vec4& vector) noexcept
    {
        return Vec4(scalar / vector.x, scalar / vector.y,
            scalar / vector.z, scalar / vector.w);
    }

    template <typename MatrixT>
    inline MatrixT Add(const MatrixT& left, const MatrixT& right) noexcept
    {
        MatrixT result;
        for (std::size_t i = 0; i < 16; ++i)
            result[i] = left[i] + right[i];
        return result;
    }

    template <typename MatrixT>
    inline MatrixT Subtract(const MatrixT& left, const MatrixT& right) noexcept
    {
        MatrixT result;
        for (std::size_t i = 0; i < 16; ++i)
            result[i] = left[i] - right[i];
        return result;
    }

    template <typename MatrixT>
    inline MatrixT Scale(const MatrixT& matrix, float scalar) noexcept
    {
        MatrixT result;
        for (std::size_t i = 0; i < 16; ++i)
            result[i] = matrix[i] * scalar;
        return result;
    }

    template <typename MatrixT>
    inline MatrixT Divide(const MatrixT& matrix, float scalar) noexcept
    {
        MatrixT result;
        for (std::size_t i = 0; i < 16; ++i)
            result[i] = matrix[i] / scalar;
        return result;
    }

    template <typename MatrixT>
    inline MatrixT Divide(const MatrixT& left, const MatrixT& right) noexcept
    {
        MatrixT result;
        for (std::size_t i = 0; i < 16; ++i)
            result[i] = left[i] / right[i];
        return result;
    }

    inline Mat4 operator+(const Mat4& left, const Mat4& right) noexcept
    {
        return Add(left, right);
    }

    inline Mat4 operator-(const Mat4& left, const Mat4& right) noexcept
    {
        return Subtract(left, right);
    }

    inline Mat4 operator*(const Mat4& left, const Mat4& right) noexcept
    {
        Mat4 result;
        float temp[16];
        bx::mtxMul(temp, left.data(), right.data());
        std::memcpy(result.m, temp, sizeof(temp));
        return result;
    }

    inline Mat4 operator*(const Mat4& matrix, float scalar) noexcept
    {
        return Scale(matrix, scalar);
    }

    inline Mat4 operator*(float scalar, const Mat4& matrix) noexcept
    {
        return matrix * scalar;
    }

    inline Mat4 operator/(const Mat4& matrix, float scalar) noexcept
    {
        return Divide(matrix, scalar);
    }

    inline Mat4 operator/(const Mat4& left, const Mat4& right) noexcept
    {
        return Divide(left, right);
    }

    inline Mat4A operator+(const Mat4A& left, const Mat4A& right) noexcept
    {
        return Add(left, right);
    }

    inline Mat4A operator-(const Mat4A& left, const Mat4A& right) noexcept
    {
        return Subtract(left, right);
    }

    inline Mat4A operator*(const Mat4A& left, const Mat4A& right) noexcept
    {
        Mat4A result;
        float temp[16];
        bx::mtxMul(temp, left.data(), right.data());
        std::memcpy(result.m, temp, sizeof(temp));
        return result;
    }

    inline Mat4A operator*(const Mat4A& matrix, float scalar) noexcept
    {
        return Scale(matrix, scalar);
    }

    inline Mat4A operator*(float scalar, const Mat4A& matrix) noexcept
    {
        return matrix * scalar;
    }

    inline Mat4A operator/(const Mat4A& matrix, float scalar) noexcept
    {
        return Divide(matrix, scalar);
    }

    inline Mat4A operator/(const Mat4A& left, const Mat4A& right) noexcept
    {
        return Divide(left, right);
    }

    template <typename MatrixT>
    inline MatrixT* Identity(MatrixT* output) noexcept
    {
        if (!output)
            return nullptr;
        bx::mtxIdentity(output->data());
        return output;
    }

    template <typename OutMatrixT, typename InMatrixT>
    inline OutMatrixT* Transpose(OutMatrixT* output,
        const InMatrixT* input) noexcept
    {
        if (!output || !input)
            return nullptr;

        float temp[16];
        bx::mtxTranspose(temp, input->data());
        std::memcpy(output->data(), temp, sizeof(temp));
        return output;
    }

    template <typename OutMatrixT, typename LeftMatrixT, typename RightMatrixT>
    inline OutMatrixT* Multiply(OutMatrixT* output,
        const LeftMatrixT* left, const RightMatrixT* right) noexcept
    {
        if (!output || !left || !right)
            return nullptr;

        float temp[16];
        bx::mtxMul(temp, left->data(), right->data());
        std::memcpy(output->data(), temp, sizeof(temp));
        return output;
    }

    inline float Determinant(const Mat4& matrix) noexcept
    {
        const float* a = matrix.data();
        return
            a[0] * (a[5] * (a[10] * a[15] - a[11] * a[14])
                  - a[6] * (a[9] * a[15] - a[11] * a[13])
                  + a[7] * (a[9] * a[14] - a[10] * a[13]))
          - a[1] * (a[4] * (a[10] * a[15] - a[11] * a[14])
                  - a[6] * (a[8] * a[15] - a[11] * a[12])
                  + a[7] * (a[8] * a[14] - a[10] * a[12]))
          + a[2] * (a[4] * (a[9] * a[15] - a[11] * a[13])
                  - a[5] * (a[8] * a[15] - a[11] * a[12])
                  + a[7] * (a[8] * a[13] - a[9] * a[12]))
          - a[3] * (a[4] * (a[9] * a[14] - a[10] * a[13])
                  - a[5] * (a[8] * a[14] - a[10] * a[12])
                  + a[6] * (a[8] * a[13] - a[9] * a[12]));
    }

    template <typename MatrixT>
    inline MatrixT* Inverse(MatrixT* output, float* determinant,
        const MatrixT* input) noexcept
    {
        if (!output || !input)
            return nullptr;

        const float det = Determinant(static_cast<const Mat4&>(*input));
        if (determinant)
            *determinant = det;
        if (det == 0.0f)
            return nullptr;

        float temp[16];
        bx::mtxInverse(temp, input->data());
        std::memcpy(output->data(), temp, sizeof(temp));
        return output;
    }

    template <typename MatrixT>
    inline Vec4* Transform(Vec4* output, const Vec4* input,
        const MatrixT* matrix) noexcept
    {
        if (!output || !input || !matrix)
            return nullptr;

        float temp[4];
        bx::vec4MulMtx(temp, input->data(), matrix->data());
        std::memcpy(output->data(), temp, sizeof(temp));
        return output;
    }
}

#endif // NIBGFXMATH_H
