#pragma once
#include <array>
#include <vector>
#include <atomic>
#include <mutex>
#include <cstddef>

namespace dvds {

enum class WindowFunction {
    Hann,
    BlackmanHarris,
    Hamming
};

class FFTAnalyzer {
public:
    static constexpr int kFFTOrder = 11; // 2048 samples
    static constexpr int kFFTSize = 1 << kFFTOrder;
    static constexpr int kBinCount = kFFTSize / 2;

    FFTAnalyzer();
    ~FFTAnalyzer();

    void setSampleRate(double sampleRate);
    void setWindowFunction(WindowFunction wf);

    void pushSamples(const float* samples, int numSamples);
    void processFFT();

    const std::array<float, kBinCount>& getMagnitudeSpectrum() const { return magnitude_; }
    const std::array<float, kBinCount>& getLogMagnitudeSpectrum() const { return logMagnitude_; }
    float getRMS() const { return rms_.load(); }
    float getPeakLevel() const { return peak_.load(); }
    float getBinFrequency(int bin) const;
    int frequencyToBin(float freq) const;

    bool isNewDataAvailable() const { return newDataAvailable_.load(); }
    void clearNewDataFlag() { newDataAvailable_.store(false); }

private:
    void applyWindow();

    double sampleRate_ = 44100.0;
    WindowFunction windowFunc_ = WindowFunction::Hann;
    std::array<float, kFFTSize> windowCoeffs_;

    std::array<float, kFFTSize> inputBuffer_{};
    int writePos_ = 0;
    std::array<float, kFFTSize * 2> fftData_{};
    std::array<float, kBinCount> magnitude_{};
    std::array<float, kBinCount> logMagnitude_{};
    std::atomic<float> rms_{ 0.0f };
    std::atomic<float> peak_{ 0.0f };
    std::atomic<bool> newDataAvailable_{ false };
    std::mutex processMutex_;

    void computeWindowCoeffs();
};

} // namespace dvds
