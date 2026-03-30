#include "Audio/FFTAnalyzer.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace dvds {

FFTAnalyzer::FFTAnalyzer() {
    computeWindowCoeffs();
}

FFTAnalyzer::~FFTAnalyzer() = default;

void FFTAnalyzer::setSampleRate(double sr) {
    sampleRate_ = sr;
}

void FFTAnalyzer::setWindowFunction(WindowFunction wf) {
    windowFunc_ = wf;
    computeWindowCoeffs();
}

void FFTAnalyzer::computeWindowCoeffs() {
    const int N = kFFTSize;
    for (int i = 0; i < N; ++i) {
        float x = static_cast<float>(i) / static_cast<float>(N - 1);
        switch (windowFunc_) {
            case WindowFunction::Hann:
                windowCoeffs_[i] = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * x));
                break;
            case WindowFunction::BlackmanHarris:
                windowCoeffs_[i] = 0.35875f
                    - 0.48829f * std::cos(2.0f * static_cast<float>(M_PI) * x)
                    + 0.14128f * std::cos(4.0f * static_cast<float>(M_PI) * x)
                    - 0.01168f * std::cos(6.0f * static_cast<float>(M_PI) * x);
                break;
            case WindowFunction::Hamming:
                windowCoeffs_[i] = 0.54f - 0.46f * std::cos(2.0f * static_cast<float>(M_PI) * x);
                break;
        }
    }
}

void FFTAnalyzer::pushSamples(const float* samples, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        inputBuffer_[writePos_] = samples[i];
        writePos_ = (writePos_ + 1) % kFFTSize;
    }
}

void FFTAnalyzer::processFFT() {
    std::lock_guard<std::mutex> lock(processMutex_);

    // Copy input buffer in correct order (oldest -> newest) starting from writePos_
    std::array<float, kFFTSize> ordered;
    for (int i = 0; i < kFFTSize; ++i)
        ordered[i] = inputBuffer_[(writePos_ + i) % kFFTSize];

    // Compute RMS and peak
    float sumSq = 0.0f;
    float peakVal = 0.0f;
    for (int i = 0; i < kFFTSize; ++i) {
        float v = ordered[i];
        sumSq += v * v;
        peakVal = std::max(peakVal, std::abs(v));
    }
    rms_.store(std::sqrt(sumSq / kFFTSize));
    peak_.store(peakVal);

    // Apply window
    for (int i = 0; i < kFFTSize; ++i)
        ordered[i] *= windowCoeffs_[i];

    // Pack for FFT: interleaved real/imag (simple DFT for now; JUCE dsp::FFT would be used in actual build)
    for (int i = 0; i < kFFTSize; ++i) {
        fftData_[i * 2] = ordered[i];
        fftData_[i * 2 + 1] = 0.0f;
    }

    // Cooley-Tukey radix-2 DIT FFT (self-contained, no JUCE dependency for testing)
    {
        const int N = kFFTSize;
        // Bit-reversal permutation
        for (int i = 1, j = 0; i < N; ++i) {
            int bit = N >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) {
                std::swap(fftData_[i * 2], fftData_[j * 2]);
                std::swap(fftData_[i * 2 + 1], fftData_[j * 2 + 1]);
            }
        }
        // Butterfly
        for (int len = 2; len <= N; len <<= 1) {
            float angle = -2.0f * static_cast<float>(M_PI) / len;
            float wRe = std::cos(angle), wIm = std::sin(angle);
            for (int i = 0; i < N; i += len) {
                float curRe = 1.0f, curIm = 0.0f;
                for (int j = 0; j < len / 2; ++j) {
                    int u = (i + j) * 2;
                    int v = (i + j + len / 2) * 2;
                    float tRe = curRe * fftData_[v] - curIm * fftData_[v + 1];
                    float tIm = curRe * fftData_[v + 1] + curIm * fftData_[v];
                    fftData_[v] = fftData_[u] - tRe;
                    fftData_[v + 1] = fftData_[u + 1] - tIm;
                    fftData_[u] += tRe;
                    fftData_[u + 1] += tIm;
                    float newRe = curRe * wRe - curIm * wIm;
                    curIm = curRe * wIm + curIm * wRe;
                    curRe = newRe;
                }
            }
        }
    }

    // Compute magnitude spectrum
    float scale = 2.0f / kFFTSize;
    for (int i = 0; i < kBinCount; ++i) {
        float re = fftData_[i * 2];
        float im = fftData_[i * 2 + 1];
        magnitude_[i] = std::sqrt(re * re + im * im) * scale;
        logMagnitude_[i] = 20.0f * std::log10(std::max(magnitude_[i], 1e-10f));
    }

    newDataAvailable_.store(true);
}

float FFTAnalyzer::getBinFrequency(int bin) const {
    return static_cast<float>(bin) * static_cast<float>(sampleRate_) / kFFTSize;
}

int FFTAnalyzer::frequencyToBin(float freq) const {
    return static_cast<int>(freq * kFFTSize / sampleRate_);
}

} // namespace dvds
