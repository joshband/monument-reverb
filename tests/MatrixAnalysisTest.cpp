#include <array>
#include <cmath>
#include <iostream>

// Duplicate matrix definitions from Chambers.cpp for analysis
constexpr float kInvSqrt8 = 0.3535533905932738f;
using Matrix8 = std::array<std::array<float, 8>, 8>;

constexpr float kHouseholderDiag = 0.75f;
constexpr float kHouseholderOff = -0.25f;

constexpr Matrix8 kMatrixHadamard{{
    {{ kInvSqrt8,  kInvSqrt8,  kInvSqrt8,  kInvSqrt8,  kInvSqrt8,  kInvSqrt8,  kInvSqrt8,  kInvSqrt8 }},
    {{ kInvSqrt8, -kInvSqrt8,  kInvSqrt8, -kInvSqrt8,  kInvSqrt8, -kInvSqrt8,  kInvSqrt8, -kInvSqrt8 }},
    {{ kInvSqrt8,  kInvSqrt8, -kInvSqrt8, -kInvSqrt8,  kInvSqrt8,  kInvSqrt8, -kInvSqrt8, -kInvSqrt8 }},
    {{ kInvSqrt8, -kInvSqrt8, -kInvSqrt8,  kInvSqrt8,  kInvSqrt8, -kInvSqrt8, -kInvSqrt8,  kInvSqrt8 }},
    {{ kInvSqrt8,  kInvSqrt8,  kInvSqrt8,  kInvSqrt8, -kInvSqrt8, -kInvSqrt8, -kInvSqrt8, -kInvSqrt8 }},
    {{ kInvSqrt8, -kInvSqrt8,  kInvSqrt8, -kInvSqrt8, -kInvSqrt8,  kInvSqrt8, -kInvSqrt8,  kInvSqrt8 }},
    {{ kInvSqrt8,  kInvSqrt8, -kInvSqrt8, -kInvSqrt8, -kInvSqrt8, -kInvSqrt8,  kInvSqrt8,  kInvSqrt8 }},
    {{ kInvSqrt8, -kInvSqrt8, -kInvSqrt8,  kInvSqrt8, -kInvSqrt8,  kInvSqrt8,  kInvSqrt8, -kInvSqrt8 }}
}};

constexpr Matrix8 kMatrixHouseholder{{
    {{ kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag }}
}};

void blendMatrices(const Matrix8& a, const Matrix8& b, float blend, Matrix8& dest)
{
    const float invBlend = 1.0f - blend;
    for (size_t row = 0; row < 8; ++row)
        for (size_t col = 0; col < 8; ++col)
            dest[row][col] = a[row][col] * invBlend + b[row][col] * blend;
}

void normalizeColumns(Matrix8& matrix)
{
    for (size_t col = 0; col < 8; ++col)
    {
        float norm = 0.0f;
        for (size_t row = 0; row < 8; ++row)
            norm += matrix[row][col] * matrix[row][col];
        if (norm > 1.0e-6f)
        {
            const float invNorm = 1.0f / std::sqrt(norm);
            for (size_t row = 0; row < 8; ++row)
                matrix[row][col] *= invNorm;
        }
    }
}

// Compute spectral radius (maximum absolute eigenvalue) using power iteration
float computeSpectralRadius(const Matrix8& matrix, int iterations = 1000)
{
    // Start with random vector
    std::array<float, 8> v = {1.0f, 0.5f, 0.3f, 0.7f, 0.2f, 0.9f, 0.4f, 0.6f};

    // Normalize
    float norm = 0.0f;
    for (auto val : v) norm += val * val;
    norm = std::sqrt(norm);
    for (auto& val : v) val /= norm;

    float eigenvalue = 0.0f;

    for (int iter = 0; iter < iterations; ++iter)
    {
        // Multiply matrix * vector
        std::array<float, 8> result = {0};
        for (size_t row = 0; row < 8; ++row)
            for (size_t col = 0; col < 8; ++col)
                result[row] += matrix[row][col] * v[col];

        // Compute norm of result
        norm = 0.0f;
        for (auto val : result) norm += val * val;
        eigenvalue = std::sqrt(norm);

        // Normalize for next iteration
        for (size_t i = 0; i < 8; ++i)
            v[i] = result[i] / eigenvalue;
    }

    return eigenvalue;
}

// Compute Frobenius norm of M^T * M - I (measure of non-orthogonality)
float computeOrthogonalityError(const Matrix8& matrix)
{
    // Compute M^T * M
    Matrix8 mtm = {};
    for (size_t i = 0; i < 8; ++i)
    {
        for (size_t j = 0; j < 8; ++j)
        {
            float sum = 0.0f;
            for (size_t k = 0; k < 8; ++k)
                sum += matrix[k][i] * matrix[k][j];  // M^T[i][k] * M[k][j]
            mtm[i][j] = sum;
        }
    }

    // Compute Frobenius norm of (M^T * M - I)
    float error = 0.0f;
    for (size_t i = 0; i < 8; ++i)
    {
        for (size_t j = 0; j < 8; ++j)
        {
            float target = (i == j) ? 1.0f : 0.0f;
            float diff = mtm[i][j] - target;
            error += diff * diff;
        }
    }

    return std::sqrt(error);
}

int main()
{
    std::cout << "=== FDN Matrix Orthogonality Analysis ===\n\n";

    // Test Hadamard matrix (should be perfectly orthogonal)
    std::cout << "Hadamard Matrix:\n";
    float hadamardRadius = computeSpectralRadius(kMatrixHadamard);
    float hadamardError = computeOrthogonalityError(kMatrixHadamard);
    std::cout << "  Spectral radius: " << hadamardRadius << " (should be ~1.0)\n";
    std::cout << "  Orthogonality error: " << hadamardError << " (should be ~0.0)\n\n";

    // Test Householder matrix (should be perfectly orthogonal)
    std::cout << "Householder Matrix:\n";
    float householderRadius = computeSpectralRadius(kMatrixHouseholder);
    float householderError = computeOrthogonalityError(kMatrixHouseholder);
    std::cout << "  Spectral radius: " << householderRadius << " (should be ~1.0)\n";
    std::cout << "  Orthogonality error: " << householderError << " (should be ~0.0)\n\n";

    // Test blended matrices at various warp values
    std::cout << "Blended Matrices (with column normalization):\n";
    for (float warp : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f})
    {
        Matrix8 blended;
        blendMatrices(kMatrixHadamard, kMatrixHouseholder, warp, blended);
        normalizeColumns(blended);

        float radius = computeSpectralRadius(blended);
        float error = computeOrthogonalityError(blended);

        std::cout << "  Warp = " << warp << ":\n";
        std::cout << "    Spectral radius: " << radius;
        if (radius > 1.01f)
            std::cout << " ⚠️  ENERGY AMPLIFICATION!";
        std::cout << "\n";
        std::cout << "    Orthogonality error: " << error << "\n";

        // Estimate energy growth over 30 seconds
        float feedbackCoeff = 0.85f;
        float effectiveGain = feedbackCoeff * radius;
        std::cout << "    Effective gain per iteration: " << effectiveGain;
        if (effectiveGain > 1.0f)
            std::cout << " ⚠️  UNSTABLE!";
        else if (effectiveGain > 0.95f)
            std::cout << " ⚠️  VERY SLOW DECAY";
        std::cout << "\n\n";
    }

    return 0;
}
