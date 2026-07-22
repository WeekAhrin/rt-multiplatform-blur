#include "../math.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include <cmath>

namespace {

constexpr int Channels = 4;

static inline uint8_t ClampU8(float v) {
    return static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f));
}

static void DownsampleAverage(std::span<const uint8_t> input, int width, int height,
                               int smallW, int smallH, std::span<uint8_t> downsampled) {
    float xRatio = static_cast<float>(width) / static_cast<float>(smallW);
    float yRatio = static_cast<float>(height) / static_cast<float>(smallH);
    for (int y = 0; y < smallH; y++) {
        int srcYStart = static_cast<int>(y * yRatio);
        int srcYEnd = static_cast<int>((y + 1) * yRatio);
        if (y == smallH - 1) srcYEnd = height;
        for (int x = 0; x < smallW; x++) {
            int srcXStart = static_cast<int>(x * xRatio);
            int srcXEnd = static_cast<int>((x + 1) * xRatio);
            if (x == smallW - 1) srcXEnd = width;

            int sum[Channels] = {0, 0, 0, 0};
            int count = 0;
            for (int sy = srcYStart; sy < srcYEnd; sy++) {
                for (int sx = srcXStart; sx < srcXEnd; sx++) {
                    int srcIdx = (sy * width + sx) * Channels;
                    for (int c = 0; c < Channels; ++c) {
                        sum[c] += input[srcIdx + c];
                    }
                    count++;
                }
            }

            int dstIdx = (y * smallW + x) * Channels;
            if (count > 0) {
                for (int c = 0; c < Channels; ++c) {
                    downsampled[dstIdx + c] = static_cast<uint8_t>(sum[c] / count);
                }
            } else {
                for (int c = 0; c < Channels; ++c) {
                    downsampled[dstIdx + c] = 0;
                }
            }
        }
    }
}

static void UpsampleBilinear(std::span<const uint8_t> small, int smallW, int smallH,
                              std::span<uint8_t> out, int width, int height) {
    float upX = static_cast<float>(smallW) / static_cast<float>(width);
    float upY = static_cast<float>(smallH) / static_cast<float>(height);
    for (int y = 0; y < height; y++) {
        float srcY = y * upY;
        int y1 = static_cast<int>(srcY);
        int y2 = std::min(smallH - 1, y1 + 1);
        float dy = srcY - y1;
        float inv_dy = 1.0f - dy;

        for (int x = 0; x < width; x++) {
            float srcX = x * upX;
            int x1 = static_cast<int>(srcX);
            int x2 = std::min(smallW - 1, x1 + 1);
            float dx = srcX - x1;
            float inv_dx = 1.0f - dx;

            int idx11 = (y1 * smallW + x1) * Channels;
            int idx12 = (y2 * smallW + x1) * Channels;
            int idx21 = (y1 * smallW + x2) * Channels;
            int idx22 = (y2 * smallW + x2) * Channels;

            int dstIdx = (y * width + x) * Channels;
            for (int c = 0; c < Channels; c++) {
                float val1 = small[idx11 + c] * inv_dx + small[idx21 + c] * dx;
                float val2 = small[idx12 + c] * inv_dx + small[idx22 + c] * dx;
                out[dstIdx + c] = ClampU8(val1 * inv_dy + val2 * dy);
            }
        }
    }
}

static void ApplyBoxIntegral(std::span<const uint8_t> src, std::span<uint8_t> dst, int w, int h, int radius) {
    int sumStride = w + 1;
    size_t sumSize = static_cast<size_t>(w + 1) * static_cast<size_t>(h + 1) * Channels;
    std::vector<uint32_t> sumBuffer(sumSize, 0u);

    for (int y = 1; y <= h; y++) {
        int srcRow = (y - 1) * w * Channels;
        int sumRow = y * sumStride * Channels;
        int sumRowPrev = (y - 1) * sumStride * Channels;
        for (int x = 1; x <= w; x++) {
            int srcIdx = srcRow + (x - 1) * Channels;
            int sumIdx = sumRow + x * Channels;
            int sumLeft = sumRow + (x - 1) * Channels;
            int sumUp = sumRowPrev + x * Channels;
            int sumUpLeft = sumRowPrev + (x - 1) * Channels;
            for (int c = 0; c < Channels; ++c) {
                sumBuffer[sumIdx + c] = src[srcIdx + c] + sumBuffer[sumLeft + c] + sumBuffer[sumUp + c] - sumBuffer[sumUpLeft + c];
            }
        }
    }

    int r = std::max(1, radius);
    for (int y = 0; y < h; y++) {
        int y1 = std::max(0, y - r);
        int y2 = std::min(h - 1, y + r);
        int y1p = y1;
        int y2p = y2 + 1;

        for (int x = 0; x < w; x++) {
            int x1 = std::max(0, x - r);
            int x2 = std::min(w - 1, x + r);
            int x1p = x1;
            int x2p = x2 + 1;

            int idxA = (y1p * sumStride + x1p) * Channels;
            int idxB = (y1p * sumStride + x2p) * Channels;
            int idxC = (y2p * sumStride + x1p) * Channels;
            int idxD = (y2p * sumStride + x2p) * Channels;

            int area = (x2 - x1 + 1) * (y2 - y1 + 1);
            int dstIdx = (y * w + x) * Channels;
            for (int c = 0; c < Channels; ++c) {
                dst[dstIdx + c] = static_cast<uint8_t>((sumBuffer[idxD + c] - sumBuffer[idxB + c] - sumBuffer[idxC + c] + sumBuffer[idxA + c]) / area);
            }
        }
    }
}

static void BuildGaussianKernel(int radius, std::vector<float>& kernel) {
    int r = std::max(1, radius);
    float sigma = std::max(1.0f, r * 0.5f);
    kernel.resize(static_cast<size_t>(2 * r + 1));
    float sum = 0.0f;
    for (int i = -r; i <= r; ++i) {
        float w = std::exp(-(i * i) / (2.0f * sigma * sigma));
        kernel[static_cast<size_t>(i + r)] = w;
        sum += w;
    }
    if (sum > 0.0f) {
        for (float& w : kernel) {
            w /= sum;
        }
    }
}

static void ApplyGaussian(std::span<const uint8_t> src, std::span<uint8_t> tmp, std::span<uint8_t> dst, int w, int h, int radius) {
    std::vector<float> kernel;
    BuildGaussianKernel(radius, kernel);
    int r = std::max(1, radius);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float acc[Channels] = {0, 0, 0, 0};
            for (int k = -r; k <= r; ++k) {
                int sx = std::clamp(x + k, 0, w - 1);
                int idx = (y * w + sx) * Channels;
                float kw = kernel[static_cast<size_t>(k + r)];
                for (int c = 0; c < Channels; ++c) {
                    acc[c] += src[idx + c] * kw;
                }
            }
            int outIdx = (y * w + x) * Channels;
            for (int c = 0; c < Channels; ++c) {
                tmp[outIdx + c] = ClampU8(acc[c]);
            }
        }
    }

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float acc[Channels] = {0, 0, 0, 0};
            for (int k = -r; k <= r; ++k) {
                int sy = std::clamp(y + k, 0, h - 1);
                int idx = (sy * w + x) * Channels;
                float kw = kernel[static_cast<size_t>(k + r)];
                for (int c = 0; c < Channels; ++c) {
                    acc[c] += tmp[idx + c] * kw;
                }
            }
            int outIdx = (y * w + x) * Channels;
            for (int c = 0; c < Channels; ++c) {
                dst[outIdx + c] = ClampU8(acc[c]);
            }
        }
    }
}

static uint32_t Hash32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

static void AddFrostedNoise(std::span<uint8_t> buf, int w, int h, int amount) {
    int noise = std::max(1, amount);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint32_t h1 = Hash32(static_cast<uint32_t>(x * 73856093u) ^ static_cast<uint32_t>(y * 19349663u));
            int jitter = static_cast<int>(h1 % static_cast<uint32_t>(2 * noise + 1)) - noise;
            int idx = (y * w + x) * Channels;
            for (int c = 0; c < 3; ++c) {
                int v = static_cast<int>(buf[idx + c]) + jitter;
                buf[idx + c] = static_cast<uint8_t>(std::clamp(v, 0, 255));
            }
        }
    }
}

static void ApplyKawaseCPU(std::span<const uint8_t> src, std::span<uint8_t> tmp, std::span<uint8_t> dst, int w, int h, int radius) {
    std::copy(src.begin(), src.end(), tmp.begin());
    int passes = std::max(2, std::min(8, radius / 3 + 1));
    for (int p = 0; p < passes; ++p) {
        float t = (passes > 1) ? (static_cast<float>(p) / static_cast<float>(passes - 1)) : 0.0f;
        int off = std::max(1, static_cast<int>(1.0f + t * std::max(1, radius)));
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int x1 = std::clamp(x - off, 0, w - 1);
                int x2 = std::clamp(x + off, 0, w - 1);
                int y1 = std::clamp(y - off, 0, h - 1);
                int y2 = std::clamp(y + off, 0, h - 1);

                int i1 = (y1 * w + x1) * Channels;
                int i2 = (y1 * w + x2) * Channels;
                int i3 = (y2 * w + x1) * Channels;
                int i4 = (y2 * w + x2) * Channels;
                int o = (y * w + x) * Channels;
                for (int c = 0; c < Channels; ++c) {
                    int v = (tmp[i1 + c] + tmp[i2 + c] + tmp[i3 + c] + tmp[i4 + c]) / 4;
                    dst[o + c] = static_cast<uint8_t>((v * 3 + tmp[o + c]) / 4);
                }
            }
        }
        std::swap(tmp, dst);
    }
    std::copy(tmp.begin(), tmp.end(), dst.begin());
}

static void ApplyMotionCPU(std::span<const uint8_t> src, std::span<uint8_t> dst, int w, int h, int radius) {
    int r = std::max(1, radius);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float acc[Channels] = {0, 0, 0, 0};
            float wsum = 0.0f;
            for (int k = -r; k <= r; ++k) {
                int sx = std::clamp(x + k, 0, w - 1);
                int sy = std::clamp(y + k / 3, 0, h - 1);
                float kw = 1.0f - (std::abs(static_cast<float>(k)) / static_cast<float>(r + 1));
                int idx = (sy * w + sx) * Channels;
                for (int c = 0; c < Channels; ++c) {
                    acc[c] += src[idx + c] * kw;
                }
                wsum += kw;
            }
            int o = (y * w + x) * Channels;
            for (int c = 0; c < Channels; ++c) {
                dst[o + c] = ClampU8((wsum > 0.0f) ? (acc[c] / wsum) : src[o + c]);
            }
        }
    }
}

static void ApplyZoomCPU(std::span<const uint8_t> src, std::span<uint8_t> dst, int w, int h, int radius) {
    float cx = (w - 1) * 0.5f;
    float cy = (h - 1) * 0.5f;
    float strength = std::max(0.5f, radius * 0.08f);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float dx = cx - static_cast<float>(x);
            float dy = cy - static_cast<float>(y);
            int sx1 = std::clamp(static_cast<int>(x + dx * 0.04f * strength), 0, w - 1);
            int sy1 = std::clamp(static_cast<int>(y + dy * 0.04f * strength), 0, h - 1);
            int sx2 = std::clamp(static_cast<int>(x - dx * 0.04f * strength), 0, w - 1);
            int sy2 = std::clamp(static_cast<int>(y - dy * 0.04f * strength), 0, h - 1);
            int o = (y * w + x) * Channels;
            int i0 = o;
            int i1 = (sy1 * w + sx1) * Channels;
            int i2 = (sy2 * w + sx2) * Channels;
            for (int c = 0; c < Channels; ++c) {
                dst[o + c] = static_cast<uint8_t>((src[i0 + c] * 2 + src[i1 + c] + src[i2 + c]) / 4);
            }
        }
    }
}

static void ApplyPixelateCPU(std::span<const uint8_t> src, std::span<uint8_t> dst, int w, int h, int radius) {
    int block = std::max(2, radius / 2 + 1);
    for (int by = 0; by < h; by += block) {
        for (int bx = 0; bx < w; bx += block) {
            int ex = std::min(w, bx + block);
            int ey = std::min(h, by + block);
            int sum[Channels] = {0, 0, 0, 0};
            int cnt = 0;
            for (int y = by; y < ey; ++y) {
                for (int x = bx; x < ex; ++x) {
                    int i = (y * w + x) * Channels;
                    for (int c = 0; c < Channels; ++c) sum[c] += src[i + c];
                    cnt++;
                }
            }
            uint8_t avg[Channels];
            for (int c = 0; c < Channels; ++c) avg[c] = static_cast<uint8_t>(sum[c] / std::max(1, cnt));
            for (int y = by; y < ey; ++y) {
                for (int x = bx; x < ex; ++x) {
                    int i = (y * w + x) * Channels;
                    for (int c = 0; c < Channels; ++c) dst[i + c] = avg[c];
                }
            }
        }
    }
}

} // namespace

void CpuApply(std::span<const uint8_t> input, std::span<uint8_t> output, int widthVal, int heightVal, float radiusVal, int downscaleVal, int typeVal) {
    if (input.empty() || output.empty() || widthVal <= 0 || heightVal <= 0) {
        return;
    }

    int intRadius = std::max(1, static_cast<int>(radiusVal));
    int ds = std::max(1, downscaleVal);
    int smallW = std::max(1, widthVal / ds);
    int smallH = std::max(1, heightVal / ds);

    size_t smallSize = static_cast<size_t>(smallW) * static_cast<size_t>(smallH) * Channels;
    std::vector<uint8_t> downsampled(smallSize);
    std::vector<uint8_t> blurA(smallSize);
    std::vector<uint8_t> blurB(smallSize);

    DownsampleAverage(input, widthVal, heightVal, smallW, smallH, downsampled);

    int smallRadius = std::max(1, intRadius / ds);
    switch (typeVal) {
        case 1:
            ApplyGaussian(downsampled, blurB, blurA, smallW, smallH, smallRadius);
            break;
        case 2:
            ApplyBoxIntegral(downsampled, blurA, smallW, smallH, smallRadius);
            break;
        case 3:
            ApplyGaussian(downsampled, blurB, blurA, smallW, smallH, smallRadius);
            AddFrostedNoise(blurA, smallW, smallH, std::max(2, smallRadius));
            break;
        case 4:
            ApplyKawaseCPU(downsampled, blurA, blurB, smallW, smallH, smallRadius);
            std::copy(blurB.begin(), blurB.end(), blurA.begin());
            break;
        case 5:
            ApplyGaussian(downsampled, blurB, blurA, smallW, smallH, std::max(1, smallRadius));
            ApplyGaussian(blurA, blurB, blurA, smallW, smallH, std::max(1, smallRadius / 2));
            break;
        case 6:
            ApplyMotionCPU(downsampled, blurA, smallW, smallH, std::max(1, smallRadius));
            break;
        case 7:
            ApplyZoomCPU(downsampled, blurA, smallW, smallH, std::max(1, smallRadius));
            break;
        case 8:
            ApplyPixelateCPU(downsampled, blurA, smallW, smallH, std::max(1, smallRadius));
            break;
        case 0:
        default:
            ApplyGaussian(downsampled, blurB, blurA, smallW, smallH, std::max(1, smallRadius));
            break;
    }

    UpsampleBilinear(blurA, smallW, smallH, output, widthVal, heightVal);

    for (int i = 0; i < widthVal * heightVal; ++i) {
        output[i * Channels + 3] = 255;
    }
}
