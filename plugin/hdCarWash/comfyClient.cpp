// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// comfyClient.cpp — ComfyUI client implementation

#include "comfyClient.h"
#include "tokens.h"
#include "debugCodes.h"

#include "pxr/base/tf/diagnostic.h"

#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <ctime>
#include <cstdio>      // std::snprintf for frame filenames (#3)
#include <filesystem>  // write the downloaded frame sequence to disk (#3)

// stb_image for robust PNG decoding (handles compressed PNGs)
// See: https://github.com/nothings/stb
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG           // Only need PNG support
#define STBI_NO_STDIO           // We load from memory, not files
#include "stb_image.h"

// Windows HTTP and filesystem
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

PXR_NAMESPACE_OPEN_SCOPE

// =============================================================================
// Base64 Encoding (for image transfer)
// =============================================================================

namespace {

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string Base64Encode(const std::vector<uint8_t>& data) {
    std::string result;
    int i = 0;
    int j = 0;
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];
    size_t in_len = data.size();
    const uint8_t* bytes_to_encode = data.data();

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                result += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; j < i + 1; j++)
            result += base64_chars[char_array_4[j]];

        while (i++ < 3)
            result += '=';
    }

    return result;
}

std::vector<uint8_t> Base64Decode(const std::string& encoded) {
    std::vector<uint8_t> result;
    int in_len = static_cast<int>(encoded.size());
    int i = 0;
    int j = 0;
    int in_ = 0;
    uint8_t char_array_4[4], char_array_3[3];

    auto is_base64 = [](unsigned char c) {
        return (isalnum(c) || (c == '+') || (c == '/'));
    };

    auto find_char = [](char c) -> int {
        const char* p = strchr(base64_chars, c);
        return p ? static_cast<int>(p - base64_chars) : -1;
    };

    while (in_len-- && encoded[in_] != '=' && is_base64(encoded[in_])) {
        char_array_4[i++] = encoded[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = static_cast<uint8_t>(find_char(char_array_4[i]));

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++)
                result.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = 0; j < i; j++)
            char_array_4[j] = static_cast<uint8_t>(find_char(char_array_4[j]));

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; j < i - 1; j++)
            result.push_back(char_array_3[j]);
    }

    return result;
}

// Session-stable client ID generator (deterministic for reproducibility)
std::string GenerateClientId() {
    static std::string clientId;
    static std::once_flag idOnce;

    std::call_once(idOnce, []() {
        // Generate session-stable ID based on process ID + init timestamp
        // This is deterministic within a session but unique across sessions
#ifdef _WIN32
        DWORD pid = GetCurrentProcessId();
#else
        pid_t pid = getpid();
#endif
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

        std::ostringstream oss;
        oss << "hdcarwash-" << pid << "-" << (ms % 1000000);
        clientId = oss.str();
    });

    return clientId;
}

}  // anonymous namespace

// =============================================================================
// PNG Encoding (minimal implementation for ControlNet images)
// =============================================================================

namespace {

// CRC32 for PNG chunks (thread-safe initialization)
uint32_t Crc32(const uint8_t* data, size_t length) {
    static uint32_t table[256];
    static std::once_flag crcOnce;

    std::call_once(crcOnce, []() {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) {
                c = (c & 1) ? (0xedb88320 ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
    });

    uint32_t crc = 0xffffffff;
    for (size_t i = 0; i < length; i++) {
        crc = table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
    }
    return crc ^ 0xffffffff;
}

// Adler32 for zlib
uint32_t Adler32(const uint8_t* data, size_t length) {
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < length; i++) {
        a = (a + data[i]) % 65521;
        b = (b + a) % 65521;
    }
    return (b << 16) | a;
}

// Simple uncompressed zlib stream (for PNG)
std::vector<uint8_t> ZlibCompress(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> result;

    // Zlib header (no compression)
    result.push_back(0x78);  // CMF
    result.push_back(0x01);  // FLG

    // Split into blocks of max 65535 bytes
    size_t offset = 0;
    while (offset < data.size()) {
        size_t blockSize = std::min(data.size() - offset, (size_t)65535);
        bool lastBlock = (offset + blockSize >= data.size());

        result.push_back(lastBlock ? 0x01 : 0x00);  // BFINAL, BTYPE=00 (no compression)
        result.push_back(blockSize & 0xff);
        result.push_back((blockSize >> 8) & 0xff);
        result.push_back(~blockSize & 0xff);
        result.push_back((~blockSize >> 8) & 0xff);

        result.insert(result.end(), data.begin() + offset, data.begin() + offset + blockSize);
        offset += blockSize;
    }

    // Adler32 checksum
    uint32_t adler = Adler32(data.data(), data.size());
    result.push_back((adler >> 24) & 0xff);
    result.push_back((adler >> 16) & 0xff);
    result.push_back((adler >> 8) & 0xff);
    result.push_back(adler & 0xff);

    return result;
}

void WriteChunk(std::vector<uint8_t>& png, const char* type, const std::vector<uint8_t>& data) {
    // Length (big-endian)
    uint32_t len = static_cast<uint32_t>(data.size());
    png.push_back((len >> 24) & 0xff);
    png.push_back((len >> 16) & 0xff);
    png.push_back((len >> 8) & 0xff);
    png.push_back(len & 0xff);

    // Type
    size_t typeStart = png.size();
    for (int i = 0; i < 4; i++) {
        png.push_back(type[i]);
    }

    // Data
    png.insert(png.end(), data.begin(), data.end());

    // CRC (over type + data)
    std::vector<uint8_t> crcData(png.begin() + typeStart, png.end());
    uint32_t crc = Crc32(crcData.data(), crcData.size());
    png.push_back((crc >> 24) & 0xff);
    png.push_back((crc >> 16) & 0xff);
    png.push_back((crc >> 8) & 0xff);
    png.push_back(crc & 0xff);
}

std::vector<uint8_t> EncodePNG(const uint8_t* pixels, unsigned int width, unsigned int height, int channels) {
    std::vector<uint8_t> png;

    // PNG signature
    const uint8_t signature[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
    png.insert(png.end(), signature, signature + 8);

    // IHDR chunk
    std::vector<uint8_t> ihdr;
    ihdr.push_back((width >> 24) & 0xff);
    ihdr.push_back((width >> 16) & 0xff);
    ihdr.push_back((width >> 8) & 0xff);
    ihdr.push_back(width & 0xff);
    ihdr.push_back((height >> 24) & 0xff);
    ihdr.push_back((height >> 16) & 0xff);
    ihdr.push_back((height >> 8) & 0xff);
    ihdr.push_back(height & 0xff);
    ihdr.push_back(8);  // bit depth
    ihdr.push_back(channels == 4 ? 6 : (channels == 3 ? 2 : 0));  // color type
    ihdr.push_back(0);  // compression
    ihdr.push_back(0);  // filter
    ihdr.push_back(0);  // interlace
    WriteChunk(png, "IHDR", ihdr);

    // Prepare raw image data with filter bytes
    std::vector<uint8_t> rawData;
    for (unsigned int y = 0; y < height; y++) {
        rawData.push_back(0);  // filter type: none
        for (unsigned int x = 0; x < width; x++) {
            for (int c = 0; c < channels; c++) {
                rawData.push_back(pixels[(y * width + x) * channels + c]);
            }
        }
    }

    // IDAT chunk (compressed data)
    std::vector<uint8_t> compressed = ZlibCompress(rawData);
    WriteChunk(png, "IDAT", compressed);

    // IEND chunk
    WriteChunk(png, "IEND", {});

    return png;
}

}  // anonymous namespace

// =============================================================================
// HdCarWashComfyClient Implementation
// =============================================================================

HdCarWashComfyClient::HdCarWashComfyClient(const std::string& serverUrl,
                                         const std::string& wsUrl)
    : _serverUrl(serverUrl)
    , _wsUrl(wsUrl)
    , _workflowPath("")
    , _backend(HdCarWashSettingsTokens->backendLTX2)
    , _cancelRequested(false)
    , _useWebSocket(true)
    , _clientId(GenerateClientId())
    , _wsConnected(false)
{
    // Derive the WebSocket progress URL from the server URL unless one was
    // explicitly provided — the old ws://localhost:9999 default pointed at the
    // Synapse automation server, not ComfyUI's :8188/ws progress socket. (#5)
    if (wsUrl.empty()) {
        _wsUrl = _DeriveWsUrl(serverUrl);
    }

#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    _wsSocket = nullptr;
#endif

    TF_DEBUG_MSG(HD_CARWASH, "ComfyClient initialized: %s, ws: %s (client: %s)\n",
                 serverUrl.c_str(), _wsUrl.c_str(), _clientId.c_str());
}

HdCarWashComfyClient::~HdCarWashComfyClient()
{
    _WebSocketDisconnect();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool
HdCarWashComfyClient::IsServerAvailable() const
{
    // Try to connect to server
    std::string response = const_cast<HdCarWashComfyClient*>(this)->_HttpGet("/system_stats");
    bool available = !response.empty();
    TF_DEBUG_MSG(HD_CARWASH, "IsServerAvailable: %s (response size=%zu)\n",
                 available ? "YES" : "NO", response.size());
    return available;
}

HdCarWashRenderResult
HdCarWashComfyClient::ProcessFrame(
    const HdCarWashFramebuffer& framebuffer,
    const HdCarWashStyleParams& params)
{
    HdCarWashRenderResult result;
    // NOTE: the cancel flag is reset in ProcessFrameAsync BEFORE launch, not
    // here — resetting here would clobber a cancel issued (e.g. by the
    // render-pass destructor) between launch and the worker starting. (#5)
    auto startTime = std::chrono::high_resolution_clock::now();

    TF_DEBUG_MSG(HD_CARWASH, "ProcessFrame: %dx%d, prompt='%s'\n",
                 framebuffer.width, framebuffer.height, params.prompt.c_str());

    // Check server availability
    if (!IsServerAvailable()) {
        result.success = false;
        result.errorMessage = "ComfyUI server not available at " + _serverUrl;
        TF_WARN("ComfyUI server not available");
        return result;
    }

    // Build workflow JSON
    std::string workflow = _BuildWorkflow(framebuffer, params);
    if (workflow.empty()) {
        result.success = false;
        result.errorMessage = "Failed to build workflow";
        return result;
    }

    if (_cancelRequested) { result.errorMessage = "Render cancelled"; return result; }

    // Submit to ComfyUI
    std::string promptId = _SubmitWorkflow(workflow);
    if (promptId.empty()) {
        result.success = false;
        result.errorMessage = "ComfyUI rejected the workflow (check that the LTX-2 models and nodes are installed)";
        return result;
    }
    result.promptId = promptId;

    if (_cancelRequested) { _Interrupt(); result.errorMessage = "Render cancelled"; return result; }

    // Wait for completion. Timeout is configurable (default 300s) — the old
    // hardcoded 60s was unrealistic for 19B video. Distinguish the failure mode
    // so the artist gets an actionable message, and interrupt abandoned jobs so
    // they stop pinning the GPU. (#5)
    HdCarWashWaitResult waitResult = _WaitForCompletion(promptId, _completionTimeoutSeconds);
    if (waitResult != HdCarWashWaitResult::Success) {
        result.success = false;
        switch (waitResult) {
            case HdCarWashWaitResult::Timeout:
                result.errorMessage = "ComfyUI did not finish within "
                    + std::to_string(static_cast<int>(_completionTimeoutSeconds))
                    + "s (raise carwash:comfyui:timeoutSeconds, or check GPU/VRAM)";
                _Interrupt();
                break;
            case HdCarWashWaitResult::ExecutionError:
                result.errorMessage = "ComfyUI reported an execution error "
                    "(check model files, node availability, and VRAM)";
                break;
            case HdCarWashWaitResult::Cancelled:
                result.errorMessage = "Render cancelled";
                _Interrupt();
                break;
            default:
                result.errorMessage = "Workflow did not complete";
                break;
        }
        return result;
    }

    if (_cancelRequested) { result.errorMessage = "Render cancelled"; return result; }

    // Download result
    result.styledImage = _DownloadResult(promptId, params, result.width, result.height,
                                         result.frameCount, result.outputPath);
    if (result.styledImage.empty()) {
        result.success = false;
        result.errorMessage = "Failed to download result image";
        return result;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.inferenceTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    result.success = true;

    TF_DEBUG_MSG(HD_CARWASH, "ProcessFrame complete: %.1fms\n", result.inferenceTimeMs);

    return result;
}

std::future<HdCarWashRenderResult>
HdCarWashComfyClient::ProcessFrameAsync(
    const HdCarWashFramebuffer& framebuffer,
    const HdCarWashStyleParams& params)
{
    // Reset the cancel flag HERE, before launching the worker, so a cancel
    // issued after this point (e.g. from the render-pass destructor) sticks
    // instead of being clobbered by a reset inside the worker. (#5)
    _cancelRequested = false;

    // Clear progress so a new job starts at 0% rather than showing the previous
    // job's final value. (#6)
    _progressValue = 0;
    _progressMax = 0;

    // CRITICAL: Use value/move capture to ensure data lifetime
    // Reference capture would be unsafe as framebuffer/params may be destroyed
    // before the async task completes
    return std::async(std::launch::async,
        [this, fb = framebuffer, p = params]() {
            return ProcessFrame(fb, p);
        });
}

void
HdCarWashComfyClient::CancelPending()
{
    _cancelRequested = true;
    _Interrupt();  // also stop the in-flight server job, not just our wait loop (#5)
}

void
HdCarWashComfyClient::_Interrupt()
{
    // Best-effort: tell ComfyUI to stop the currently running prompt so an
    // abandoned (timed-out/cancelled) job stops pinning the GPU. Uses the
    // timeout-hardened _HttpPost, so it never blocks teardown for long. (#5)
    _HttpPost("/interrupt", "");
    TF_DEBUG_MSG(HD_CARWASH, "Posted /interrupt to ComfyUI\n");
}

std::string
HdCarWashComfyClient::_DeriveWsUrl(const std::string& serverUrl) const
{
    // http(s)://host:port[/...]  ->  ws://host:port/ws (ComfyUI progress socket)
    std::string hostPort = serverUrl;
    if (hostPort.rfind("https://", 0) == 0)     hostPort = hostPort.substr(8);
    else if (hostPort.rfind("http://", 0) == 0) hostPort = hostPort.substr(7);
    size_t slash = hostPort.find('/');
    if (slash != std::string::npos) hostPort = hostPort.substr(0, slash);
    if (hostPort.empty()) hostPort = "127.0.0.1:8188";
    return "ws://" + hostPort + "/ws";
}

// =============================================================================
// Buffer Encoding
// =============================================================================

std::string
HdCarWashComfyClient::EncodeDepthBuffer(
    const std::vector<float>& depth,
    unsigned int width, unsigned int height)
{
    // Convert depth to grayscale PNG
    std::vector<uint8_t> pixels(width * height);

    // Find depth range for normalization
    float minDepth = 1.0f, maxDepth = 0.0f;
    for (float d : depth) {
        if (d < 1.0f) {  // Ignore background (1.0)
            minDepth = std::min(minDepth, d);
            maxDepth = std::max(maxDepth, d);
        }
    }

    float range = maxDepth - minDepth;
    if (range < 0.001f) range = 1.0f;

    // Clamp to the allocation: writes index pixels[i], which is sized width*height.
    const size_t pixelCount = std::min(depth.size(), static_cast<size_t>(width) * height);
    for (size_t i = 0; i < pixelCount; i++) {
        float d = depth[i];
        if (d >= 1.0f) {
            pixels[i] = 255;  // Background = white (far)
        } else {
            // Near = dark, far = light (MiDaS convention)
            float normalized = (d - minDepth) / range;
            pixels[i] = static_cast<uint8_t>(normalized * 255.0f);
        }
    }

    std::vector<uint8_t> png = EncodePNG(pixels.data(), width, height, 1);
    return Base64Encode(png);
}

std::string
HdCarWashComfyClient::EncodeNormalBuffer(
    const std::vector<GfVec3f>& normals,
    unsigned int width, unsigned int height)
{
    // Convert normals to RGB PNG (standard normal map encoding)
    std::vector<uint8_t> pixels(width * height * 3);

    // Clamp to the allocation: writes pixels[i*3+0..2], sized width*height*3.
    const size_t pixelCount = std::min(normals.size(), static_cast<size_t>(width) * height);
    for (size_t i = 0; i < pixelCount; i++) {
        const GfVec3f& n = normals[i];
        // Normal map encoding: [-1,1] -> [0,255]
        pixels[i * 3 + 0] = static_cast<uint8_t>((n[0] * 0.5f + 0.5f) * 255.0f);
        pixels[i * 3 + 1] = static_cast<uint8_t>((n[1] * 0.5f + 0.5f) * 255.0f);
        pixels[i * 3 + 2] = static_cast<uint8_t>((n[2] * 0.5f + 0.5f) * 255.0f);
    }

    std::vector<uint8_t> png = EncodePNG(pixels.data(), width, height, 3);
    return Base64Encode(png);
}

std::string
HdCarWashComfyClient::EncodeColorBuffer(
    const std::vector<GfVec4f>& color,
    unsigned int width, unsigned int height)
{
    // Convert color to RGBA PNG
    std::vector<uint8_t> pixels(width * height * 4);

    // Clamp to the allocation: writes pixels[i*4+0..3], sized width*height*4.
    const size_t pixelCount = std::min(color.size(), static_cast<size_t>(width) * height);
    for (size_t i = 0; i < pixelCount; i++) {
        const GfVec4f& c = color[i];
        pixels[i * 4 + 0] = static_cast<uint8_t>(std::clamp(c[0], 0.0f, 1.0f) * 255.0f);
        pixels[i * 4 + 1] = static_cast<uint8_t>(std::clamp(c[1], 0.0f, 1.0f) * 255.0f);
        pixels[i * 4 + 2] = static_cast<uint8_t>(std::clamp(c[2], 0.0f, 1.0f) * 255.0f);
        pixels[i * 4 + 3] = static_cast<uint8_t>(std::clamp(c[3], 0.0f, 1.0f) * 255.0f);
    }

    std::vector<uint8_t> png = EncodePNG(pixels.data(), width, height, 4);
    return Base64Encode(png);
}

std::string
HdCarWashComfyClient::EncodeIdBuffer(
    const std::vector<int32_t>& ids,
    unsigned int width, unsigned int height)
{
    // Convert IDs to colored segmentation mask
    std::vector<uint8_t> pixels(width * height * 3);

    // Simple color palette for segmentation
    static const uint8_t palette[][3] = {
        {0, 0, 0},       // Background (-1)
        {255, 0, 0},     // ID 0
        {0, 255, 0},     // ID 1
        {0, 0, 255},     // ID 2
        {255, 255, 0},   // ID 3
        {255, 0, 255},   // ID 4
        {0, 255, 255},   // ID 5
        {128, 0, 0},     // ID 6
        {0, 128, 0},     // ID 7
        {0, 0, 128},     // etc
    };
    constexpr int paletteSize = sizeof(palette) / sizeof(palette[0]);

    // Clamp to the allocation: writes pixels[i*3+0..2], sized width*height*3.
    const size_t pixelCount = std::min(ids.size(), static_cast<size_t>(width) * height);
    for (size_t i = 0; i < pixelCount; i++) {
        int id = ids[i];
        int colorIdx = (id < 0) ? 0 : ((id + 1) % paletteSize);
        pixels[i * 3 + 0] = palette[colorIdx][0];
        pixels[i * 3 + 1] = palette[colorIdx][1];
        pixels[i * 3 + 2] = palette[colorIdx][2];
    }

    std::vector<uint8_t> png = EncodePNG(pixels.data(), width, height, 3);
    return Base64Encode(png);
}

std::vector<GfVec4f>
HdCarWashComfyClient::DecodeColorImage(
    const std::string& base64Png,
    unsigned int& outWidth, unsigned int& outHeight)
{
    // DEPRECATED: This function only handles uncompressed PNGs.
    // Use stb_image directly (as _DownloadResult does) for full PNG support.
    // Kept for API compatibility.

    // Decode base64 to PNG bytes
    std::vector<uint8_t> pngData = Base64Decode(base64Png);
    if (pngData.size() < 24) {
        TF_WARN("PNG data too small");
        return {};
    }

    // Verify PNG signature
    const uint8_t pngSig[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
    if (memcmp(pngData.data(), pngSig, 8) != 0) {
        TF_WARN("Invalid PNG signature");
        return {};
    }

    // Parse IHDR chunk (starts at offset 8)
    // Chunk: length(4) + type(4) + data + crc(4)
    size_t pos = 8;
    uint32_t ihdrLen = (pngData[pos] << 24) | (pngData[pos+1] << 16) |
                       (pngData[pos+2] << 8) | pngData[pos+3];
    pos += 4;

    // Verify IHDR type
    if (pngData[pos] != 'I' || pngData[pos+1] != 'H' ||
        pngData[pos+2] != 'D' || pngData[pos+3] != 'R') {
        TF_WARN("First chunk is not IHDR");
        return {};
    }
    pos += 4;

    // Read dimensions
    outWidth = (pngData[pos] << 24) | (pngData[pos+1] << 16) |
               (pngData[pos+2] << 8) | pngData[pos+3];
    pos += 4;
    outHeight = (pngData[pos] << 24) | (pngData[pos+1] << 16) |
                (pngData[pos+2] << 8) | pngData[pos+3];
    pos += 4;

    uint8_t bitDepth = pngData[pos++];
    uint8_t colorType = pngData[pos++];

    TF_DEBUG_MSG(HD_CARWASH, "PNG: %dx%d, depth=%d, colorType=%d\n",
                 outWidth, outHeight, bitDepth, colorType);

    // Skip rest of IHDR + CRC
    pos = 8 + 4 + 4 + ihdrLen + 4;

    // Collect all IDAT chunks
    std::vector<uint8_t> compressedData;
    while (pos + 8 < pngData.size()) {
        uint32_t chunkLen = (pngData[pos] << 24) | (pngData[pos+1] << 16) |
                            (pngData[pos+2] << 8) | pngData[pos+3];
        pos += 4;

        char chunkType[5] = {
            static_cast<char>(pngData[pos]),
            static_cast<char>(pngData[pos+1]),
            static_cast<char>(pngData[pos+2]),
            static_cast<char>(pngData[pos+3]),
            '\0'
        };
        pos += 4;

        if (strcmp(chunkType, "IDAT") == 0) {
            compressedData.insert(compressedData.end(),
                                  pngData.begin() + pos,
                                  pngData.begin() + pos + chunkLen);
        } else if (strcmp(chunkType, "IEND") == 0) {
            break;
        }

        pos += chunkLen + 4;  // Skip data + CRC
    }

    if (compressedData.empty()) {
        TF_WARN("No IDAT chunks found");
        return {};
    }

    // Decompress using simple inflate (zlib format)
    // Skip zlib header (2 bytes) and decompress
    std::vector<uint8_t> rawData;

    // Simple inflate for uncompressed blocks (BTYPE=00)
    // For production, should use proper zlib library
    size_t cPos = 2;  // Skip zlib header
    while (cPos < compressedData.size() - 4) {  // -4 for adler32
        uint8_t header = compressedData[cPos++];
        bool bfinal = header & 0x01;
        uint8_t btype = (header >> 1) & 0x03;

        if (btype == 0) {
            // Uncompressed block
            uint16_t len = compressedData[cPos] | (compressedData[cPos+1] << 8);
            cPos += 4;  // len + nlen
            rawData.insert(rawData.end(),
                          compressedData.begin() + cPos,
                          compressedData.begin() + cPos + len);
            cPos += len;
        } else {
            // Compressed blocks - need proper inflate
            // For now, return empty and log warning
            TF_WARN("PNG uses compressed data (btype=%d), full inflate not implemented", btype);
            TF_WARN("Consider using a simpler approach or adding zlib dependency");
            return {};
        }

        if (bfinal) break;
    }

    // Determine channels from color type
    int channels = 4;  // Default RGBA
    if (colorType == 0) channels = 1;       // Grayscale
    else if (colorType == 2) channels = 3;  // RGB
    else if (colorType == 4) channels = 2;  // Grayscale+Alpha
    else if (colorType == 6) channels = 4;  // RGBA

    // Remove filter bytes and reconstruct image
    size_t rowBytes = outWidth * channels + 1;  // +1 for filter byte
    if (rawData.size() < rowBytes * outHeight) {
        TF_WARN("Insufficient decompressed data: %zu < %zu",
                rawData.size(), rowBytes * outHeight);
        return {};
    }

    std::vector<GfVec4f> result(outWidth * outHeight);

    for (unsigned int y = 0; y < outHeight; y++) {
        uint8_t filter = rawData[y * rowBytes];
        // For now, only support filter type 0 (none)
        if (filter != 0) {
            TF_DEBUG_MSG(HD_CARWASH, "PNG filter type %d not fully supported\n", filter);
        }

        for (unsigned int x = 0; x < outWidth; x++) {
            size_t srcIdx = y * rowBytes + 1 + x * channels;
            float r = 0, g = 0, b = 0, a = 1.0f;

            if (channels == 4) {
                r = rawData[srcIdx] / 255.0f;
                g = rawData[srcIdx + 1] / 255.0f;
                b = rawData[srcIdx + 2] / 255.0f;
                a = rawData[srcIdx + 3] / 255.0f;
            } else if (channels == 3) {
                r = rawData[srcIdx] / 255.0f;
                g = rawData[srcIdx + 1] / 255.0f;
                b = rawData[srcIdx + 2] / 255.0f;
            } else if (channels == 1) {
                r = g = b = rawData[srcIdx] / 255.0f;
            }

            result[y * outWidth + x] = GfVec4f(r, g, b, a);
        }
    }

    return result;
}

// =============================================================================
// Private Methods
// =============================================================================

std::string
HdCarWashComfyClient::_BuildWorkflow(
    const HdCarWashFramebuffer& framebuffer,
    const HdCarWashStyleParams& params)
{
    // Generate unique cache-buster to prevent ComfyUI from skipping execution
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::string cacheBuster = std::to_string(ms);

    // Escape special characters in prompts for JSON
    auto escapeJson = [](const std::string& s) {
        std::string result;
        for (char c : s) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c;
            }
        }
        return result;
    };

    std::string positivePrompt = escapeJson(params.prompt);
    std::string negativePrompt = escapeJson(params.negativePrompt);

    // Check which backend is selected
    TF_DEBUG_MSG(HD_CARWASH, "_BuildWorkflow: current backend = '%s'\n", _backend.GetText());

    // Check if LTX2 backend is selected
    if (_backend == HdCarWashSettingsTokens->backendLTX2) {
        TF_DEBUG_MSG(HD_CARWASH, "_BuildWorkflow: Using LTX2 backend path\n");
        // LTX2 workflow - always uses image conditioning
        std::string controlSubfolder = _SaveControlImages(framebuffer, params);
        if (!controlSubfolder.empty()) {
            TF_DEBUG_MSG(HD_CARWASH, "_BuildWorkflow: Building LTX2 workflow with subfolder '%s'\n",
                        controlSubfolder.c_str());
            return _BuildWorkflowLTX2(framebuffer, params, controlSubfolder);
        }
        TF_WARN("LTX2 backend selected but failed to save control images, falling back to SDXL");
    } else {
        TF_DEBUG_MSG(HD_CARWASH, "_BuildWorkflow: NOT using LTX2 (backend='%s', expected='%s')\n",
                    _backend.GetText(), HdCarWashSettingsTokens->backendLTX2.GetText());
    }

    // Check if ControlNet is requested (SDXL path)
    if (params.useDepthControl || params.useNormalControl) {
        // Save control images and build ControlNet workflow
        std::string controlSubfolder = _SaveControlImages(framebuffer, params);
        if (!controlSubfolder.empty()) {
            return _BuildWorkflowControlNet(framebuffer, params, controlSubfolder);
        }
        // Fall back to txt2img if control image save failed
        TF_WARN("ControlNet requested but failed to save control images, falling back to txt2img");
    }

    // Phase 1: Simple txt2img workflow (no ControlNet dependencies)
    // This uses only standard ComfyUI nodes to verify the pipeline works

    std::ostringstream json;
    json << "{\n";
    json << "  \"client_id\": \"" << _clientId << "\",\n";
    json << "  \"prompt\": {\n";

    // Node 1: Checkpoint loader (SDXL)
    json << "    \"1\": {\n";
    json << "      \"class_type\": \"CheckpointLoaderSimple\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"ckpt_name\": \"sdxl_v10VAEFix.safetensors\"\n";
    json << "      }\n";
    json << "    },\n";

    // Node 2: CLIP Text Encode (positive)
    json << "    \"2\": {\n";
    json << "      \"class_type\": \"CLIPTextEncode\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"text\": \"" << positivePrompt << "\",\n";
    json << "        \"clip\": [\"1\", 1]\n";
    json << "      }\n";
    json << "    },\n";

    // Node 3: CLIP Text Encode (negative)
    json << "    \"3\": {\n";
    json << "      \"class_type\": \"CLIPTextEncode\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"text\": \"" << negativePrompt << "\",\n";
    json << "        \"clip\": [\"1\", 1]\n";
    json << "      }\n";
    json << "    },\n";

    // Node 4: Empty Latent Image
    json << "    \"4\": {\n";
    json << "      \"class_type\": \"EmptyLatentImage\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"width\": " << framebuffer.width << ",\n";
    json << "        \"height\": " << framebuffer.height << ",\n";
    json << "        \"batch_size\": 1\n";
    json << "      }\n";
    json << "    },\n";

    // Node 5: KSampler
    json << "    \"5\": {\n";
    json << "      \"class_type\": \"KSampler\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"model\": [\"1\", 0],\n";
    json << "        \"positive\": [\"2\", 0],\n";
    json << "        \"negative\": [\"3\", 0],\n";
    json << "        \"latent_image\": [\"4\", 0],\n";
    json << "        \"seed\": " << (params.deterministic ? params.seed : params.seed + ms % 1000000) << ",\n";  // Vary seed to prevent caching
    json << "        \"steps\": " << params.inferenceSteps << ",\n";
    json << "        \"cfg\": " << params.guidanceScale << ",\n";
    json << "        \"sampler_name\": \"euler\",\n";
    json << "        \"scheduler\": \"normal\",\n";
    json << "        \"denoise\": 1.0\n";
    json << "      }\n";
    json << "    },\n";

    // Node 6: VAE Decode
    json << "    \"6\": {\n";
    json << "      \"class_type\": \"VAEDecode\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"samples\": [\"5\", 0],\n";
    json << "        \"vae\": [\"1\", 2]\n";
    json << "      }\n";
    json << "    },\n";

    // Node 7: Save Image
    json << "    \"7\": {\n";
    json << "      \"class_type\": \"SaveImage\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"images\": [\"6\", 0],\n";
    json << "        \"filename_prefix\": \"hdcarwash/frame_" << cacheBuster << "\"\n";
    json << "      }\n";
    json << "    }\n";

    json << "  }\n";
    json << "}\n";

    return json.str();
}

std::string
HdCarWashComfyClient::_SaveControlImages(
    const HdCarWashFramebuffer& framebuffer,
    const HdCarWashStyleParams& params)
{
    // Create unique subfolder for this frame
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::string subfolder = "hdcarwash_ctrl_" + std::to_string(ms % 1000000);

    std::ofstream debugLog;
    if (TfDebug::IsEnabled(HD_CARWASH)) {
        debugLog.open("C:/Temp/hdcarwash_debug.txt", std::ios::app);  // gated: no hot-path I/O / workflow-JSON leak unless TF_DEBUG=HD_CARWASH (#5)
    }
    debugLog << "[_SaveControlImages] Uploading to ComfyUI subfolder: " << subfolder << std::endl;

    bool success = true;

    // Upload the shaded color (beauty) buffer as the LTX-2 first-frame
    // conditioning image. LTXVImgToVideo animates this image, so it must be the
    // scene's actual appearance — lighting, composition, color — not the
    // grayscale depth map. Uploaded unconditionally (independent of the depth/
    // normal control toggles) because it is the primary conditioning input; it
    // also guarantees the subfolder is non-empty so the LTX-2 path doesn't fall
    // back to SDXL. (#4)
    if (!framebuffer.color.empty()) {
        std::vector<uint8_t> colorPixels(framebuffer.width * framebuffer.height * 3);
        auto to8 = [](float v) {
            float clamped = std::min(1.0f, std::max(0.0f, v));
            return static_cast<uint8_t>(clamped * 255.0f + 0.5f);
        };
        for (size_t i = 0; i < framebuffer.color.size(); i++) {
            const GfVec4f& c = framebuffer.color[i];
            colorPixels[i * 3 + 0] = to8(c[0]);
            colorPixels[i * 3 + 1] = to8(c[1]);
            colorPixels[i * 3 + 2] = to8(c[2]);
        }

        std::vector<uint8_t> png = EncodePNG(colorPixels.data(),
                                             framebuffer.width, framebuffer.height, 3);

        std::string uploadResponse = _UploadImageToComfyUI(png, "color.png", subfolder);

        debugLog << "[_SaveControlImages] Color upload response: " << uploadResponse << std::endl;

        if (uploadResponse.find("\"name\"") != std::string::npos) {
            debugLog << "[_SaveControlImages] Uploaded color.png: " << png.size() << " bytes" << std::endl;
        } else {
            debugLog << "[_SaveControlImages] ERROR: Failed to upload color.png" << std::endl;
            success = false;
        }
    }

    // Upload depth image via ComfyUI API
    if (params.useDepthControl && !framebuffer.depth.empty()) {
        // Convert depth to grayscale PNG bytes
        std::vector<uint8_t> depthPixels(framebuffer.width * framebuffer.height);

        // Find depth range for normalization
        float minDepth = 1.0f, maxDepth = 0.0f;
        for (float d : framebuffer.depth) {
            if (d < 1.0f) {
                minDepth = std::min(minDepth, d);
                maxDepth = std::max(maxDepth, d);
            }
        }
        float range = maxDepth - minDepth;
        if (range < 0.001f) range = 1.0f;

        for (size_t i = 0; i < framebuffer.depth.size(); i++) {
            float d = framebuffer.depth[i];
            if (d >= 1.0f) {
                depthPixels[i] = 255;  // Background = white
            } else {
                float normalized = (d - minDepth) / range;
                depthPixels[i] = static_cast<uint8_t>(normalized * 255.0f);
            }
        }

        std::vector<uint8_t> png = EncodePNG(depthPixels.data(),
                                             framebuffer.width, framebuffer.height, 1);

        // Upload via ComfyUI API instead of writing to disk
        std::string uploadResponse = _UploadImageToComfyUI(png, "depth.png", subfolder);

        debugLog << "[_SaveControlImages] Depth upload response: " << uploadResponse << std::endl;

        // Check if upload succeeded (response contains "name")
        if (uploadResponse.find("\"name\"") != std::string::npos) {
            debugLog << "[_SaveControlImages] Uploaded depth.png: " << png.size() << " bytes" << std::endl;
        } else {
            debugLog << "[_SaveControlImages] ERROR: Failed to upload depth.png" << std::endl;
            success = false;
        }
    }

    // Upload normal image via ComfyUI API
    if (params.useNormalControl && !framebuffer.normal.empty()) {
        // Convert normals to RGB PNG bytes
        std::vector<uint8_t> normalPixels(framebuffer.width * framebuffer.height * 3);

        for (size_t i = 0; i < framebuffer.normal.size(); i++) {
            const GfVec3f& n = framebuffer.normal[i];
            normalPixels[i * 3 + 0] = static_cast<uint8_t>((n[0] * 0.5f + 0.5f) * 255.0f);
            normalPixels[i * 3 + 1] = static_cast<uint8_t>((n[1] * 0.5f + 0.5f) * 255.0f);
            normalPixels[i * 3 + 2] = static_cast<uint8_t>((n[2] * 0.5f + 0.5f) * 255.0f);
        }

        std::vector<uint8_t> png = EncodePNG(normalPixels.data(),
                                             framebuffer.width, framebuffer.height, 3);

        // Upload via ComfyUI API instead of writing to disk
        std::string uploadResponse = _UploadImageToComfyUI(png, "normal.png", subfolder);

        debugLog << "[_SaveControlImages] Normal upload response: " << uploadResponse << std::endl;

        if (uploadResponse.find("\"name\"") != std::string::npos) {
            debugLog << "[_SaveControlImages] Uploaded normal.png: " << png.size() << " bytes" << std::endl;
        } else {
            debugLog << "[_SaveControlImages] ERROR: Failed to upload normal.png" << std::endl;
            success = false;
        }
    }

    debugLog.close();

    return success ? subfolder : "";
}

std::string
HdCarWashComfyClient::_BuildWorkflowControlNet(
    const HdCarWashFramebuffer& framebuffer,
    const HdCarWashStyleParams& params,
    const std::string& controlImageSubfolder)
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::string cacheBuster = std::to_string(ms);

    auto escapeJson = [](const std::string& s) {
        std::string result;
        for (char c : s) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c;
            }
        }
        return result;
    };

    std::string positivePrompt = escapeJson(params.prompt);
    std::string negativePrompt = escapeJson(params.negativePrompt);

    std::ofstream debugLog;
    if (TfDebug::IsEnabled(HD_CARWASH)) {
        debugLog.open("C:/Temp/hdcarwash_debug.txt", std::ios::app);  // gated: no hot-path I/O / workflow-JSON leak unless TF_DEBUG=HD_CARWASH (#5)
    }
    debugLog << "[_BuildWorkflowControlNet] Building ControlNet workflow" << std::endl;
    debugLog << "  useDepthControl: " << params.useDepthControl << std::endl;
    debugLog << "  useNormalControl: " << params.useNormalControl << std::endl;
    debugLog << "  controlImageSubfolder: " << controlImageSubfolder << std::endl;

    std::ostringstream json;
    json << "{\n";
    json << "  \"client_id\": \"" << _clientId << "\",\n";
    json << "  \"prompt\": {\n";

    int nodeId = 1;

    // Node 1: Checkpoint loader (SDXL)
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"CheckpointLoaderSimple\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"ckpt_name\": \"sdxl_v10VAEFix.safetensors\"\n";
    json << "      }\n";
    json << "    },\n";
    int checkpointNode = nodeId++;

    // Node 2: CLIP Text Encode (positive)
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"CLIPTextEncode\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"text\": \"" << positivePrompt << "\",\n";
    json << "        \"clip\": [\"" << checkpointNode << "\", 1]\n";
    json << "      }\n";
    json << "    },\n";
    int positiveClipNode = nodeId++;

    // Node 3: CLIP Text Encode (negative)
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"CLIPTextEncode\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"text\": \"" << negativePrompt << "\",\n";
    json << "        \"clip\": [\"" << checkpointNode << "\", 1]\n";
    json << "      }\n";
    json << "    },\n";
    int negativeClipNode = nodeId++;

    // Node 4: Empty Latent Image
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"EmptyLatentImage\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"width\": " << framebuffer.width << ",\n";
    json << "        \"height\": " << framebuffer.height << ",\n";
    json << "        \"batch_size\": 1\n";
    json << "      }\n";
    json << "    },\n";
    int latentNode = nodeId++;

    // Track which conditioning node to use (may be modified by ControlNet)
    int finalPositive = positiveClipNode;
    int finalNegative = negativeClipNode;

    // Depth ControlNet (if enabled)
    if (params.useDepthControl) {
        // Node: ControlNet Loader (depth)
        json << "    \"" << nodeId << "\": {\n";
        json << "      \"class_type\": \"ControlNetLoader\",\n";
        json << "      \"inputs\": {\n";
        json << "        \"control_net_name\": \"controlnet-depth-sdxl-1.0.safetensors\"\n";
        json << "      }\n";
        json << "    },\n";
        int depthControlNetLoader = nodeId++;

        // Node: Load depth image
        json << "    \"" << nodeId << "\": {\n";
        json << "      \"class_type\": \"LoadImage\",\n";
        json << "      \"inputs\": {\n";
        json << "        \"image\": \"" << controlImageSubfolder << "/depth.png\"\n";
        json << "      }\n";
        json << "    },\n";
        int depthImageNode = nodeId++;

        // Node: Apply ControlNet (depth)
        json << "    \"" << nodeId << "\": {\n";
        json << "      \"class_type\": \"ControlNetApplyAdvanced\",\n";
        json << "      \"inputs\": {\n";
        json << "        \"positive\": [\"" << finalPositive << "\", 0],\n";
        json << "        \"negative\": [\"" << finalNegative << "\", 0],\n";
        json << "        \"control_net\": [\"" << depthControlNetLoader << "\", 0],\n";
        json << "        \"image\": [\"" << depthImageNode << "\", 0],\n";
        json << "        \"strength\": " << params.controlNetStrength << ",\n";
        json << "        \"start_percent\": 0.0,\n";
        json << "        \"end_percent\": 1.0\n";
        json << "      }\n";
        json << "    },\n";
        finalPositive = nodeId;
        finalNegative = nodeId;  // ControlNetApplyAdvanced outputs both
        nodeId++;

        debugLog << "  Added depth ControlNet nodes" << std::endl;
    }

    // Normal ControlNet (if enabled)
    if (params.useNormalControl) {
        // Node: ControlNet Loader (normal)
        json << "    \"" << nodeId << "\": {\n";
        json << "      \"class_type\": \"ControlNetLoader\",\n";
        json << "      \"inputs\": {\n";
        json << "        \"control_net_name\": \"control_v11p_sd15_normalbae.pth\"\n";
        json << "      }\n";
        json << "    },\n";
        int normalControlNetLoader = nodeId++;

        // Node: Load normal image
        json << "    \"" << nodeId << "\": {\n";
        json << "      \"class_type\": \"LoadImage\",\n";
        json << "      \"inputs\": {\n";
        json << "        \"image\": \"" << controlImageSubfolder << "/normal.png\"\n";
        json << "      }\n";
        json << "    },\n";
        int normalImageNode = nodeId++;

        // Node: Apply ControlNet (normal)
        json << "    \"" << nodeId << "\": {\n";
        json << "      \"class_type\": \"ControlNetApplyAdvanced\",\n";
        json << "      \"inputs\": {\n";
        json << "        \"positive\": [\"" << finalPositive << "\", 0],\n";
        json << "        \"negative\": [\"" << finalNegative << "\", 1],\n";
        json << "        \"control_net\": [\"" << normalControlNetLoader << "\", 0],\n";
        json << "        \"image\": [\"" << normalImageNode << "\", 0],\n";
        json << "        \"strength\": " << params.normalControlNetStrength << ",\n";
        json << "        \"start_percent\": 0.0,\n";
        json << "        \"end_percent\": 1.0\n";
        json << "      }\n";
        json << "    },\n";
        finalPositive = nodeId;
        finalNegative = nodeId;
        nodeId++;

        debugLog << "  Added normal ControlNet nodes" << std::endl;
    }

    // Node: KSampler
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"KSampler\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"model\": [\"" << checkpointNode << "\", 0],\n";
    json << "        \"positive\": [\"" << finalPositive << "\", 0],\n";
    json << "        \"negative\": [\"" << finalNegative << "\", 1],\n";
    json << "        \"latent_image\": [\"" << latentNode << "\", 0],\n";
    json << "        \"seed\": " << (params.deterministic ? params.seed : params.seed + ms % 1000000) << ",\n";
    json << "        \"steps\": " << params.inferenceSteps << ",\n";
    json << "        \"cfg\": " << params.guidanceScale << ",\n";
    json << "        \"sampler_name\": \"euler\",\n";
    json << "        \"scheduler\": \"normal\",\n";
    json << "        \"denoise\": 1.0\n";
    json << "      }\n";
    json << "    },\n";
    int samplerNode = nodeId++;

    // Node: VAE Decode
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"VAEDecode\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"samples\": [\"" << samplerNode << "\", 0],\n";
    json << "        \"vae\": [\"" << checkpointNode << "\", 2]\n";
    json << "      }\n";
    json << "    },\n";
    int vaeNode = nodeId++;

    // Node: Save Image
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"SaveImage\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"images\": [\"" << vaeNode << "\", 0],\n";
    json << "        \"filename_prefix\": \"hdcarwash/frame_" << cacheBuster << "\"\n";
    json << "      }\n";
    json << "    }\n";

    json << "  }\n";
    json << "}\n";

    debugLog << "[_BuildWorkflowControlNet] Workflow built with " << nodeId << " nodes" << std::endl;
    debugLog.close();

    return json.str();
}

std::string
HdCarWashComfyClient::_BuildWorkflowLTX2(
    const HdCarWashFramebuffer& framebuffer,
    const HdCarWashStyleParams& params,
    const std::string& controlImageSubfolder)
{
    // LTX2 Image-to-Video workflow for HdCarWash
    // Uses depth/color from Houdini as conditioning image

    TF_DEBUG_MSG(HD_CARWASH, "_BuildWorkflowLTX2: ENTERED - building LTX2 workflow\n");
    TF_DEBUG_MSG(HD_CARWASH, "_BuildWorkflowLTX2: controlImageSubfolder='%s'\n",
                 controlImageSubfolder.c_str());
    TF_DEBUG_MSG(HD_CARWASH, "_BuildWorkflowLTX2: framebuffer size=%ux%u\n",
                 framebuffer.width, framebuffer.height);

    // Also write to debug log file
    std::ofstream debugLog;
    if (TfDebug::IsEnabled(HD_CARWASH)) {
        debugLog.open("C:/Temp/hdcarwash_debug.txt", std::ios::app);  // gated: no hot-path I/O / workflow-JSON leak unless TF_DEBUG=HD_CARWASH (#5)
    }
    debugLog << "\n[_BuildWorkflowLTX2] ENTERED - Building LTX2 workflow" << std::endl;
    debugLog << "[_BuildWorkflowLTX2] controlImageSubfolder: " << controlImageSubfolder << std::endl;
    debugLog << "[_BuildWorkflowLTX2] framebuffer: " << framebuffer.width << "x" << framebuffer.height << std::endl;

    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::string cacheBuster = std::to_string(ms);

    auto escapeJson = [](const std::string& s) {
        std::string result;
        for (char c : s) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c;
            }
        }
        return result;
    };

    std::string positivePrompt = escapeJson(params.prompt);
    std::string negativePrompt = escapeJson(params.negativePrompt);

    // Determine output dimensions (LTX2 works best with multiples of 32)
    unsigned int outWidth = (framebuffer.width / 32) * 32;
    unsigned int outHeight = (framebuffer.height / 32) * 32;
    if (outWidth < 64) outWidth = 768;
    if (outHeight < 64) outHeight = 512;

    // Video length: 9 frames minimum for LTX2, use 25 for ~1 second at 25fps
    // For single-frame mode, we still generate minimum frames but only use first
    int videoLength = 25;

    // Continue logging to already-open debug file
    debugLog << "[_BuildWorkflowLTX2] Building LTX2 workflow JSON" << std::endl;
    debugLog << "  Resolution: " << outWidth << "x" << outHeight << std::endl;
    debugLog << "  Video length: " << videoLength << " frames" << std::endl;
    debugLog << "  Control image: " << controlImageSubfolder << "/color.png" << std::endl;
    debugLog << "  UNET: ltx-2-19b-distilled-fp8 | CLIP: Gemma 3 12B (via LTXAVTextEncoderLoader) | VAE: taeltx_2" << std::endl;

    std::ostringstream json;
    json << "{\n";
    json << "  \"client_id\": \"" << _clientId << "\",\n";
    json << "  \"prompt\": {\n";

    int nodeId = 1;

    // Node 1: UNETLoader - LTXv 13B model (diffusion model only)
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"UNETLoader\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"unet_name\": \"LTX2\\\\ltx-2-19b-distilled-fp8_transformer_only.safetensors\",\n";
    json << "        \"weight_dtype\": \"default\"\n";
    json << "      }\n";
    json << "    },\n";
    int unetNode = nodeId++;

    // Node 2: LTXAVTextEncoderLoader - Gemma 3 12B text encoder (required for LTX2 distilled models)
    // Uses Gemma 3 12B fp4 with the dev checkpoint for config reference
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"LTXAVTextEncoderLoader\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"text_encoder\": \"gemma_3_12B_it_fp4_mixed.safetensors\",\n";
    json << "        \"ckpt_name\": \"ltx-2-19b-dev-fp8.safetensors\",\n";
    json << "        \"device\": \"default\"\n";
    json << "      }\n";
    json << "    },\n";
    int clipNode = nodeId++;

    // Node 3: VAELoader - LTX video VAE
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"VAELoader\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"vae_name\": \"LTX2\\\\taeltx_2.safetensors\"\n";
    json << "      }\n";
    json << "    },\n";
    int vaeNode = nodeId++;

    // Node 4: CLIP Text Encode (positive)
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"CLIPTextEncode\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"text\": \"" << positivePrompt << "\",\n";
    json << "        \"clip\": [\"" << clipNode << "\", 0]\n";
    json << "      }\n";
    json << "    },\n";
    int positiveClipNode = nodeId++;

    // Node 5: CLIP Text Encode (negative)
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"CLIPTextEncode\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"text\": \"" << negativePrompt << "\",\n";
    json << "        \"clip\": [\"" << clipNode << "\", 0]\n";
    json << "      }\n";
    json << "    },\n";
    int negativeClipNode = nodeId++;

    // Node 6: LTXVConditioning (adds frame rate info)
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"LTXVConditioning\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"positive\": [\"" << positiveClipNode << "\", 0],\n";
    json << "        \"negative\": [\"" << negativeClipNode << "\", 0],\n";
    json << "        \"frame_rate\": 25.0\n";
    json << "      }\n";
    json << "    },\n";
    int ltxvCondNode = nodeId++;

    // Node 7: Load Image (Houdini shaded beauty render — the scene's actual
    // appearance, which the video model animates as its first frame). (#4)
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"LoadImage\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"image\": \"" << controlImageSubfolder << "/color.png\"\n";
    json << "      }\n";
    json << "    },\n";
    int imageNode = nodeId++;

    // Node 8: LTXVImgToVideo (image-to-video conditioning)
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"LTXVImgToVideo\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"positive\": [\"" << ltxvCondNode << "\", 0],\n";
    json << "        \"negative\": [\"" << ltxvCondNode << "\", 1],\n";
    json << "        \"vae\": [\"" << vaeNode << "\", 0],\n";
    json << "        \"image\": [\"" << imageNode << "\", 0],\n";
    json << "        \"width\": " << outWidth << ",\n";
    json << "        \"height\": " << outHeight << ",\n";
    json << "        \"length\": " << videoLength << ",\n";
    json << "        \"batch_size\": 1,\n";
    json << "        \"strength\": " << params.controlNetStrength << "\n";
    json << "      }\n";
    json << "    },\n";
    int img2vidNode = nodeId++;

    // Node 9: LTXVScheduler
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"LTXVScheduler\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"steps\": " << params.inferenceSteps << ",\n";
    json << "        \"max_shift\": 2.05,\n";
    json << "        \"base_shift\": 0.95,\n";
    json << "        \"stretch\": true,\n";
    json << "        \"terminal\": 0.1,\n";
    json << "        \"latent\": [\"" << img2vidNode << "\", 2]\n";
    json << "      }\n";
    json << "    },\n";
    int schedulerNode = nodeId++;

    // Node 10: BasicGuider
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"BasicGuider\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"model\": [\"" << unetNode << "\", 0],\n";
    json << "        \"conditioning\": [\"" << img2vidNode << "\", 0]\n";
    json << "      }\n";
    json << "    },\n";
    int guiderNode = nodeId++;

    // Node 11: KSamplerSelect
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"KSamplerSelect\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"sampler_name\": \"euler\"\n";
    json << "      }\n";
    json << "    },\n";
    int samplerSelectNode = nodeId++;

    // Node 12: RandomNoise
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"RandomNoise\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"noise_seed\": " << (params.deterministic ? params.seed : params.seed + ms % 1000000) << "\n";
    json << "      }\n";
    json << "    },\n";
    int noiseNode = nodeId++;

    // Node 13: SamplerCustomAdvanced
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"SamplerCustomAdvanced\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"noise\": [\"" << noiseNode << "\", 0],\n";
    json << "        \"guider\": [\"" << guiderNode << "\", 0],\n";
    json << "        \"sampler\": [\"" << samplerSelectNode << "\", 0],\n";
    json << "        \"sigmas\": [\"" << schedulerNode << "\", 0],\n";
    json << "        \"latent_image\": [\"" << img2vidNode << "\", 2]\n";
    json << "      }\n";
    json << "    },\n";
    int samplerNode = nodeId++;

    // Node 14: VAEDecode
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"VAEDecode\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"samples\": [\"" << samplerNode << "\", 0],\n";
    json << "        \"vae\": [\"" << vaeNode << "\", 0]\n";
    json << "      }\n";
    json << "    },\n";
    int vaeDecodeNode = nodeId++;

    // Node 15: SaveImage (saves first frame for CarWash to retrieve)
    json << "    \"" << nodeId << "\": {\n";
    json << "      \"class_type\": \"SaveImage\",\n";
    json << "      \"inputs\": {\n";
    json << "        \"images\": [\"" << vaeDecodeNode << "\", 0],\n";
    json << "        \"filename_prefix\": \"hdcarwash/ltx2_" << cacheBuster << "\"\n";
    json << "      }\n";
    json << "    }\n";

    json << "  }\n";
    json << "}\n";

    debugLog << "[_BuildWorkflowLTX2] Built workflow with " << nodeId << " nodes" << std::endl;
    debugLog.close();

    return json.str();
}

std::string
HdCarWashComfyClient::_SubmitWorkflow(const std::string& workflowJson)
{
    // DEBUG: Log workflow submission
    std::ofstream debugLog;
    if (TfDebug::IsEnabled(HD_CARWASH)) {
        debugLog.open("C:/Temp/hdcarwash_debug.txt", std::ios::app);  // gated: no hot-path I/O / workflow-JSON leak unless TF_DEBUG=HD_CARWASH (#5)
    }
    debugLog << "[_SubmitWorkflow] Workflow JSON size: " << workflowJson.size() << " bytes" << std::endl;
    if (workflowJson.size() < 2000) {
        debugLog << "[_SubmitWorkflow] Workflow: " << workflowJson << std::endl;
    } else {
        debugLog << "[_SubmitWorkflow] Workflow (first 1000): " << workflowJson.substr(0, 1000) << std::endl;
    }

    std::string response = _HttpPost("/prompt", workflowJson);

    debugLog << "[_SubmitWorkflow] Response size: " << response.size() << " bytes" << std::endl;
    if (response.size() < 500) {
        debugLog << "[_SubmitWorkflow] Response: " << response << std::endl;
    } else {
        debugLog << "[_SubmitWorkflow] Response (first 500): " << response.substr(0, 500) << std::endl;
    }

    // Parse prompt_id from response
    // Response format: {"prompt_id": "xxx-xxx-xxx"}
    size_t pos = response.find("\"prompt_id\"");
    if (pos == std::string::npos) {
        debugLog << "[_SubmitWorkflow] ERROR: No prompt_id in response" << std::endl;
        debugLog.close();
        TF_WARN("No prompt_id in ComfyUI response");
        return "";
    }

    pos = response.find("\"", pos + 12);  // Find opening quote of value
    if (pos == std::string::npos) {
        debugLog << "[_SubmitWorkflow] ERROR: Malformed prompt_id" << std::endl;
        debugLog.close();
        return "";
    }

    size_t endPos = response.find("\"", pos + 1);
    if (endPos == std::string::npos) {
        debugLog << "[_SubmitWorkflow] ERROR: Malformed prompt_id end" << std::endl;
        debugLog.close();
        return "";
    }

    std::string promptId = response.substr(pos + 1, endPos - pos - 1);
    debugLog << "[_SubmitWorkflow] SUCCESS: prompt_id=" << promptId << std::endl;
    debugLog.close();

    return promptId;
}

HdCarWashWaitResult
HdCarWashComfyClient::_WaitForCompletion(const std::string& promptId, float timeoutSeconds)
{
    // Try WebSocket first for real-time updates
    if (_useWebSocket) {
        bool wsOk = _WaitForCompletionWebSocket(promptId, timeoutSeconds);
        if (_cancelRequested) return HdCarWashWaitResult::Cancelled;
        if (wsOk) return HdCarWashWaitResult::Success;
        // WebSocket failed, fall back to polling
        TF_DEBUG_MSG(HD_CARWASH, "WebSocket unavailable, falling back to HTTP polling\n");
    }

    // Fallback: HTTP polling
    auto startTime = std::chrono::high_resolution_clock::now();

    while (!_cancelRequested) {
        // Check elapsed time
        auto now = std::chrono::high_resolution_clock::now();
        float elapsed = std::chrono::duration<float>(now - startTime).count();
        if (elapsed > timeoutSeconds) {
            TF_DEBUG_MSG(HD_CARWASH, "WaitForCompletion: timeout after %.1fs\n", elapsed);
            return HdCarWashWaitResult::Timeout;
        }

        // Poll history endpoint
        std::string response = _HttpGet("/history/" + promptId);

        // Check for completion - ComfyUI returns:
        // - "outputs": {...} when complete
        // - "status_str": "success" in status object
        // We check for "outputs" with actual content (non-empty outputs section)
        bool hasOutputs = response.find("\"outputs\"") != std::string::npos &&
                         response.find("\"outputs\": {}") == std::string::npos;
        bool hasSuccess = response.find("\"status_str\": \"success\"") != std::string::npos ||
                         response.find("\"status_str\":\"success\"") != std::string::npos;

        if (hasOutputs || hasSuccess) {
            TF_DEBUG_MSG(HD_CARWASH, "WaitForCompletion: success after %.1fs\n", elapsed);
            return HdCarWashWaitResult::Success;
        }

        // Check for error status (e.g. VRAM exhaustion, node failure) — report
        // it as an execution error, not a timeout, so the message is accurate.
        if (response.find("\"status_str\": \"error\"") != std::string::npos ||
            response.find("\"status_str\":\"error\"") != std::string::npos) {
            TF_DEBUG_MSG(HD_CARWASH, "WaitForCompletion: ComfyUI reported execution error\n");
            return HdCarWashWaitResult::ExecutionError;
        }

        // Wait before polling again
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    return HdCarWashWaitResult::Cancelled;
}

std::vector<GfVec4f>
HdCarWashComfyClient::_DownloadResult(
    const std::string& promptId,
    const HdCarWashStyleParams& params,
    unsigned int& width, unsigned int& height,
    unsigned int& frameCount, std::string& outputDir)
{
    // DEBUG: Write to file for diagnosis
    std::ofstream debugLog;
    if (TfDebug::IsEnabled(HD_CARWASH)) {
        debugLog.open("C:/Temp/hdcarwash_debug.txt", std::ios::app);  // gated: no hot-path I/O / workflow-JSON leak unless TF_DEBUG=HD_CARWASH (#5)
    }
    debugLog << "[_DownloadResult] promptId: " << promptId << std::endl;

    // Define the out-params up front so every early-return path is well-formed.
    frameCount = 0;
    outputDir.clear();

    // Get output info from history
    std::string history = _HttpGet("/history/" + promptId);

    debugLog << "[_DownloadResult] History response length: " << history.size() << std::endl;
    if (history.size() < 500) {
        debugLog << "[_DownloadResult] History content: " << history << std::endl;
    } else {
        debugLog << "[_DownloadResult] History (first 500): " << history.substr(0, 500) << std::endl;
    }

    TF_DEBUG_MSG(HD_CARWASH, "History response length: %zu\n", history.size());

    // Parse filename from history JSON
    // Looking for: "outputs": {..., "images": [{"filename": "xxx.png", "subfolder": "hdcarwash", ...}]}
    // IMPORTANT: Must search within "outputs" section to avoid matching "filename_prefix" in prompt inputs

    // First find the "outputs" section
    size_t outputsPos = history.find("\"outputs\"");
    if (outputsPos == std::string::npos) {
        debugLog << "[_DownloadResult] ERROR: No outputs section in history response" << std::endl;
        debugLog.close();
        TF_WARN("No outputs section in history response");
        return {};
    }

    debugLog << "[_DownloadResult] Found outputs section at position: " << outputsPos << std::endl;

    // Iterate EVERY image entry in the outputs section, not just the first.
    // The LTX-2 workflow generates a 25-frame video; previously 24 frames were
    // produced server-side and silently discarded here. We now download the
    // full sequence to a stable per-generation directory and keep the first
    // frame as the viewport preview. (#3)
    auto extractStringValue = [&history](size_t keyPos) -> std::string {
        size_t colon = history.find(':', keyPos);
        if (colon == std::string::npos) return "";
        size_t q1 = history.find('"', colon);
        if (q1 == std::string::npos) return "";
        size_t q2 = history.find('"', q1 + 1);
        if (q2 == std::string::npos) return "";
        return history.substr(q1 + 1, q2 - q1 - 1);
    };

    const std::string genDir = _outputDir + "/" + promptId;
    {
        std::error_code ec;
        std::filesystem::create_directories(genDir, ec);
        if (ec) {
            debugLog << "[_DownloadResult] WARNING: could not create output dir "
                     << genDir << ": " << ec.message() << std::endl;
        }
    }

    std::string firstPngData;
    std::string firstFilename;
    unsigned int savedFrames = 0;
    size_t searchPos = outputsPos;

    while (true) {
        size_t fnPos = history.find("\"filename\"", searchPos);
        if (fnPos == std::string::npos) break;

        std::string filename = extractStringValue(fnPos);
        size_t nextFnPos = history.find("\"filename\"", fnPos + 10);

        // The subfolder belongs to the same image entry, so it must appear
        // after this filename and before the next one.
        std::string subfolder;
        size_t subPos = history.find("\"subfolder\"", fnPos);
        if (subPos != std::string::npos &&
            (nextFnPos == std::string::npos || subPos < nextFnPos)) {
            subfolder = extractStringValue(subPos);
        }

        searchPos = fnPos + 10;
        if (filename.empty()) continue;

        std::string viewUrl = "/view?filename=" + filename;
        if (!subfolder.empty()) viewUrl += "&subfolder=" + subfolder;

        std::string framePng = _HttpGet(viewUrl);
        if (framePng.size() < 24) {
            debugLog << "[_DownloadResult] Skipping too-small frame '" << filename
                     << "' (" << framePng.size() << " bytes)" << std::endl;
            continue;
        }

        savedFrames++;

        // Write the PNG bytes straight to disk (already encoded — no re-encode).
        char frameName[32];
        std::snprintf(frameName, sizeof(frameName), "/frame_%04u.png", savedFrames);
        std::ofstream frameFile(genDir + frameName, std::ios::binary);
        if (frameFile) {
            frameFile.write(framePng.data(),
                            static_cast<std::streamsize>(framePng.size()));
            frameFile.close();
        } else {
            debugLog << "[_DownloadResult] WARNING: could not write "
                     << genDir << frameName << std::endl;
        }

        if (firstPngData.empty()) {
            firstPngData = framePng;
            firstFilename = filename;
        }
    }

    debugLog << "[_DownloadResult] Saved " << savedFrames << " frame(s) to " << genDir << std::endl;
    TF_DEBUG_MSG(HD_CARWASH, "Saved %u frame(s) to %s\n", savedFrames, genDir.c_str());

    frameCount = savedFrames;
    outputDir = (savedFrames > 0) ? genDir : std::string();

    if (firstPngData.empty()) {
        debugLog << "[_DownloadResult] ERROR: No usable frames downloaded" << std::endl;
        debugLog.close();
        TF_WARN("No usable frames downloaded from ComfyUI");
        return {};
    }

    // Decode the first frame for the viewport preview.
    std::string pngData = firstPngData;
    debugLog << "[_DownloadResult] Preview frame: " << firstFilename
             << " (" << pngData.size() << " bytes)" << std::endl;
    TF_DEBUG_MSG(HD_CARWASH, "Preview frame: %zu bytes\n", pngData.size());

    // Use stb_image to decode the PNG (handles all compression types)
    int imgWidth = 0, imgHeight = 0, imgChannels = 0;

    // stbi_load_from_memory returns RGBA pixels
    unsigned char* pixels = stbi_load_from_memory(
        reinterpret_cast<const unsigned char*>(pngData.data()),
        static_cast<int>(pngData.size()),
        &imgWidth, &imgHeight, &imgChannels,
        4  // Force RGBA output
    );

    if (!pixels) {
        const char* reason = stbi_failure_reason();
        debugLog << "[_DownloadResult] ERROR: stb_image failed: " << (reason ? reason : "unknown") << std::endl;
        debugLog.close();
        TF_WARN("stb_image failed to decode PNG: %s", reason);
        width = 0;
        height = 0;
        return {};
    }

    width = static_cast<unsigned int>(imgWidth);
    height = static_cast<unsigned int>(imgHeight);

    TF_DEBUG_MSG(HD_CARWASH, "stb_image decoded: %ux%u, %d channels\n",
                 width, height, imgChannels);

    // Sanity check dimensions
    const unsigned int MAX_DIM = 8192;
    if (width == 0 || height == 0 || width > MAX_DIM || height > MAX_DIM) {
        TF_WARN("Invalid PNG dimensions: %ux%u", width, height);
        stbi_image_free(pixels);
        width = 0;
        height = 0;
        return {};
    }

    // Convert uint8 RGBA to GfVec4f
    size_t totalPixels = static_cast<size_t>(width) * height;
    std::vector<GfVec4f> result(totalPixels);

    for (size_t i = 0; i < totalPixels; i++) {
        size_t srcIdx = i * 4;
        result[i] = GfVec4f(
            pixels[srcIdx + 0] / 255.0f,  // R
            pixels[srcIdx + 1] / 255.0f,  // G
            pixels[srcIdx + 2] / 255.0f,  // B
            pixels[srcIdx + 3] / 255.0f   // A
        );
    }

    // Free stb_image allocated memory
    stbi_image_free(pixels);

    // Sidecar metadata: makes each <promptId> directory a self-describing,
    // reproducible render (generation history/versioning). Note the recorded
    // seed is the BASE seed; when deterministic is false the workflow jitters it
    // at submission, so the exact noise_seed is not captured here yet. (#3)
    if (savedFrames > 0) {
        auto escapeJson = [](const std::string& s) {
            std::string r;
            for (char c : s) {
                switch (c) {
                    case '"':  r += "\\\""; break;
                    case '\\': r += "\\\\"; break;
                    case '\n': r += "\\n";  break;
                    case '\r': r += "\\r";  break;
                    case '\t': r += "\\t";  break;
                    default:   r += c;
                }
            }
            return r;
        };

        std::ostringstream sidecar;
        sidecar << "{\n";
        sidecar << "  \"promptId\": \"" << escapeJson(promptId) << "\",\n";
        sidecar << "  \"backend\": \"" << escapeJson(_backend.GetString()) << "\",\n";
        sidecar << "  \"prompt\": \"" << escapeJson(params.prompt) << "\",\n";
        sidecar << "  \"negativePrompt\": \"" << escapeJson(params.negativePrompt) << "\",\n";
        sidecar << "  \"seed\": " << params.seed << ",\n";
        sidecar << "  \"deterministic\": " << (params.deterministic ? "true" : "false") << ",\n";
        sidecar << "  \"inferenceSteps\": " << params.inferenceSteps << ",\n";
        sidecar << "  \"guidanceScale\": " << params.guidanceScale << ",\n";
        sidecar << "  \"controlNetStrength\": " << params.controlNetStrength << ",\n";
        sidecar << "  \"width\": " << width << ",\n";
        sidecar << "  \"height\": " << height << ",\n";
        sidecar << "  \"frameCount\": " << savedFrames << ",\n";
        sidecar << "  \"frameRate\": 25.0\n";
        sidecar << "}\n";

        std::ofstream sidecarFile(genDir + "/carwash.json", std::ios::binary);
        if (sidecarFile) {
            sidecarFile << sidecar.str();
            sidecarFile.close();
            debugLog << "[_DownloadResult] Wrote sidecar: " << genDir << "/carwash.json" << std::endl;
        } else {
            debugLog << "[_DownloadResult] WARNING: could not write sidecar in " << genDir << std::endl;
        }
    }

    debugLog << "[_DownloadResult] SUCCESS: Decoded " << totalPixels << " pixels (" << width << "x" << height << ")"
             << ", sequence of " << savedFrames << " frame(s) in " << genDir << std::endl;
    debugLog.close();

    TF_DEBUG_MSG(HD_CARWASH, "Successfully decoded %zu pixels (preview); %u frame(s) saved\n",
                 totalPixels, savedFrames);

    return result;
}

std::string
HdCarWashComfyClient::_HttpGet(const std::string& endpoint)
{
#ifdef _WIN32
    // Parse URL
    std::string host = _serverUrl;
    int port = 8188;

    // Remove http:// prefix
    if (host.substr(0, 7) == "http://") {
        host = host.substr(7);
    }

    // Extract port if present
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos) {
        try { port = std::stoi(host.substr(colonPos + 1)); } catch (...) { /* keep default 8188 on malformed/empty/IPv6 port */ }
        host = host.substr(0, colonPos);
    }

    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        TF_DEBUG_MSG(HD_CARWASH, "_HttpGet: socket() failed, WSA error=%d\n",
                     WSAGetLastError());
        return "";
    }

    // Set socket timeouts (3 seconds for send/recv)
    DWORD timeout = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    // Resolve hostname
    struct addrinfo hints = {}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
        TF_DEBUG_MSG(HD_CARWASH, "_HttpGet: getaddrinfo(%s:%d) failed, WSA error=%d\n",
                     host.c_str(), port, WSAGetLastError());
        closesocket(sock);
        return "";
    }

    // Non-blocking connect with timeout
    u_long nonBlocking = 1;
    ioctlsocket(sock, FIONBIO, &nonBlocking);

    int connectResult = connect(sock, result->ai_addr, (int)result->ai_addrlen);
    if (connectResult == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            // Connection in progress — wait with select()
            fd_set writeSet, exceptSet;
            FD_ZERO(&writeSet);
            FD_ZERO(&exceptSet);
            FD_SET(sock, &writeSet);
            FD_SET(sock, &exceptSet);

            struct timeval tv;
            tv.tv_sec = 3;
            tv.tv_usec = 0;

            int selectResult = select(0, nullptr, &writeSet, &exceptSet, &tv);
            if (selectResult <= 0 || FD_ISSET(sock, &exceptSet)) {
                TF_DEBUG_MSG(HD_CARWASH, "_HttpGet: connect(%s:%d) timed out or failed (select=%d, WSA=%d)\n",
                             host.c_str(), port, selectResult, WSAGetLastError());
                freeaddrinfo(result);
                closesocket(sock);
                return "";
            }
        } else {
            TF_DEBUG_MSG(HD_CARWASH, "_HttpGet: connect(%s:%d) failed immediately, WSA error=%d\n",
                         host.c_str(), port, err);
            freeaddrinfo(result);
            closesocket(sock);
            return "";
        }
    }
    freeaddrinfo(result);

    // Switch back to blocking mode for send/recv
    nonBlocking = 0;
    ioctlsocket(sock, FIONBIO, &nonBlocking);

    // Send HTTP request
    std::ostringstream request;
    request << "GET " << endpoint << " HTTP/1.1\r\n";
    request << "Host: " << host << ":" << port << "\r\n";
    request << "Connection: close\r\n";
    request << "\r\n";

    std::string reqStr = request.str();
    if (send(sock, reqStr.c_str(), (int)reqStr.size(), 0) == SOCKET_ERROR) {
        TF_DEBUG_MSG(HD_CARWASH, "_HttpGet: send() failed, WSA error=%d\n",
                     WSAGetLastError());
        closesocket(sock);
        return "";
    }

    // Receive response (handle binary data correctly - don't truncate at null bytes!)
    std::string response;
    char buffer[4096];
    int bytesReceived;
    while ((bytesReceived = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        // Use append with explicit length to preserve null bytes in binary data
        response.append(buffer, bytesReceived);
    }

    closesocket(sock);

    // Strip HTTP headers
    size_t headerEnd = response.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        return response.substr(headerEnd + 4);
    }

    return response;
#else
    // Non-Windows implementation placeholder
    return "";
#endif
}

std::string
HdCarWashComfyClient::_HttpPost(const std::string& endpoint, const std::string& body)
{
#ifdef _WIN32
    // Parse URL (same as _HttpGet)
    std::string host = _serverUrl;
    int port = 8188;

    if (host.substr(0, 7) == "http://") {
        host = host.substr(7);
    }

    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos) {
        try { port = std::stoi(host.substr(colonPos + 1)); } catch (...) { /* keep default 8188 on malformed/empty/IPv6 port */ }
        host = host.substr(0, colonPos);
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        TF_DEBUG_MSG(HD_CARWASH, "_HttpPost: socket() failed, WSA error=%d\n",
                     WSAGetLastError());
        return "";
    }

    // Set socket timeouts (3 seconds for send/recv)
    DWORD timeout = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    struct addrinfo hints = {}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
        TF_DEBUG_MSG(HD_CARWASH, "_HttpPost: getaddrinfo(%s:%d) failed, WSA error=%d\n",
                     host.c_str(), port, WSAGetLastError());
        closesocket(sock);
        return "";
    }

    // Non-blocking connect with timeout
    u_long nonBlocking = 1;
    ioctlsocket(sock, FIONBIO, &nonBlocking);

    int connectResult = connect(sock, result->ai_addr, (int)result->ai_addrlen);
    if (connectResult == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            fd_set writeSet, exceptSet;
            FD_ZERO(&writeSet);
            FD_ZERO(&exceptSet);
            FD_SET(sock, &writeSet);
            FD_SET(sock, &exceptSet);

            struct timeval tv;
            tv.tv_sec = 3;
            tv.tv_usec = 0;

            int selectResult = select(0, nullptr, &writeSet, &exceptSet, &tv);
            if (selectResult <= 0 || FD_ISSET(sock, &exceptSet)) {
                TF_DEBUG_MSG(HD_CARWASH, "_HttpPost: connect(%s:%d) timed out or failed (select=%d, WSA=%d)\n",
                             host.c_str(), port, selectResult, WSAGetLastError());
                freeaddrinfo(result);
                closesocket(sock);
                return "";
            }
        } else {
            TF_DEBUG_MSG(HD_CARWASH, "_HttpPost: connect(%s:%d) failed immediately, WSA error=%d\n",
                         host.c_str(), port, err);
            freeaddrinfo(result);
            closesocket(sock);
            return "";
        }
    }
    freeaddrinfo(result);

    // Switch back to blocking mode for send/recv
    nonBlocking = 0;
    ioctlsocket(sock, FIONBIO, &nonBlocking);

    // Send HTTP POST request
    std::ostringstream request;
    request << "POST " << endpoint << " HTTP/1.1\r\n";
    request << "Host: " << host << ":" << port << "\r\n";
    request << "Content-Type: application/json\r\n";
    request << "Content-Length: " << body.size() << "\r\n";
    request << "Connection: close\r\n";
    request << "\r\n";
    request << body;

    std::string reqStr = request.str();
    if (send(sock, reqStr.c_str(), (int)reqStr.size(), 0) == SOCKET_ERROR) {
        TF_DEBUG_MSG(HD_CARWASH, "_HttpPost: send() failed, WSA error=%d\n",
                     WSAGetLastError());
        closesocket(sock);
        return "";
    }

    // Receive response (handle binary data correctly - don't truncate at null bytes!)
    std::string response;
    char buffer[4096];
    int bytesReceived;
    while ((bytesReceived = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        // Use append with explicit length to preserve null bytes in binary data
        response.append(buffer, bytesReceived);
    }

    closesocket(sock);

    // Strip HTTP headers
    size_t headerEnd = response.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        return response.substr(headerEnd + 4);
    }

    return response;
#else
    return "";
#endif
}

std::string
HdCarWashComfyClient::_UploadImageToComfyUI(
    const std::vector<uint8_t>& pngData,
    const std::string& filename,
    const std::string& subfolder)
{
    // Upload image to ComfyUI via /upload/image endpoint (multipart/form-data)
    // This ensures the image is placed in ComfyUI's actual input directory
#ifdef _WIN32
    std::string host = _serverUrl;
    int port = 8188;

    if (host.substr(0, 7) == "http://") {
        host = host.substr(7);
    }

    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos) {
        try { port = std::stoi(host.substr(colonPos + 1)); } catch (...) { /* keep default 8188 on malformed/empty/IPv6 port */ }
        host = host.substr(0, colonPos);
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return "";
    }

    // Socket timeouts so a stalled upload can't hang the AI worker (and thus
    // render-pass teardown) forever. This was the one path that could produce a
    // true permanent hang — it had a blocking connect and unbounded recv. (#5)
    DWORD timeout = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    struct addrinfo hints = {}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
        closesocket(sock);
        return "";
    }

    // Non-blocking connect with a 3s bound (same pattern as _HttpPost).
    u_long nonBlocking = 1;
    ioctlsocket(sock, FIONBIO, &nonBlocking);
    int connectResult = connect(sock, result->ai_addr, (int)result->ai_addrlen);
    if (connectResult == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            fd_set writeSet, exceptSet;
            FD_ZERO(&writeSet);
            FD_ZERO(&exceptSet);
            FD_SET(sock, &writeSet);
            FD_SET(sock, &exceptSet);
            struct timeval tv;
            tv.tv_sec = 3;
            tv.tv_usec = 0;
            int selectResult = select(0, nullptr, &writeSet, &exceptSet, &tv);
            if (selectResult <= 0 || FD_ISSET(sock, &exceptSet)) {
                freeaddrinfo(result);
                closesocket(sock);
                return "";
            }
        } else {
            freeaddrinfo(result);
            closesocket(sock);
            return "";
        }
    }
    freeaddrinfo(result);

    // Back to blocking mode for send/recv (now bounded by SO_*TIMEO above).
    nonBlocking = 0;
    ioctlsocket(sock, FIONBIO, &nonBlocking);

    // Build multipart/form-data body
    std::string boundary = "----HdCarWashBoundary" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count());

    std::ostringstream bodyStream;

    // Image field
    bodyStream << "--" << boundary << "\r\n";
    bodyStream << "Content-Disposition: form-data; name=\"image\"; filename=\"" << filename << "\"\r\n";
    bodyStream << "Content-Type: image/png\r\n\r\n";

    std::string bodyPrefix = bodyStream.str();

    std::ostringstream bodySuffix;

    // Subfolder field (if provided)
    if (!subfolder.empty()) {
        bodySuffix << "\r\n--" << boundary << "\r\n";
        bodySuffix << "Content-Disposition: form-data; name=\"subfolder\"\r\n\r\n";
        bodySuffix << subfolder;
    }

    // Overwrite field (always overwrite)
    bodySuffix << "\r\n--" << boundary << "\r\n";
    bodySuffix << "Content-Disposition: form-data; name=\"overwrite\"\r\n\r\n";
    bodySuffix << "true";

    bodySuffix << "\r\n--" << boundary << "--\r\n";

    std::string suffixStr = bodySuffix.str();

    // Calculate total content length
    size_t contentLength = bodyPrefix.size() + pngData.size() + suffixStr.size();

    // Build HTTP headers
    std::ostringstream headers;
    headers << "POST /upload/image HTTP/1.1\r\n";
    headers << "Host: " << host << ":" << port << "\r\n";
    headers << "Content-Type: multipart/form-data; boundary=" << boundary << "\r\n";
    headers << "Content-Length: " << contentLength << "\r\n";
    headers << "Connection: close\r\n";
    headers << "\r\n";

    std::string headerStr = headers.str();

    // Send request in parts, checking every send and handling partial sends of
    // the (potentially multi-MB) PNG body. A failed/timed-out send returns "".
    auto sendAll = [sock](const char* data, int len) -> bool {
        int sent = 0;
        while (sent < len) {
            int n = send(sock, data + sent, len - sent, 0);
            if (n == SOCKET_ERROR || n == 0) return false;
            sent += n;
        }
        return true;
    };
    if (!sendAll(headerStr.c_str(), (int)headerStr.size()) ||
        !sendAll(bodyPrefix.c_str(), (int)bodyPrefix.size()) ||
        !sendAll(reinterpret_cast<const char*>(pngData.data()), (int)pngData.size()) ||
        !sendAll(suffixStr.c_str(), (int)suffixStr.size())) {
        closesocket(sock);
        return "";
    }

    // Receive response
    std::string response;
    char buffer[4096];
    int bytesReceived;
    while ((bytesReceived = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        response.append(buffer, bytesReceived);
    }

    closesocket(sock);

    // Strip HTTP headers
    size_t headerEnd = response.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        return response.substr(headerEnd + 4);
    }

    return response;
#else
    return "";
#endif
}

// =============================================================================
// WebSocket Implementation
// =============================================================================

bool
HdCarWashComfyClient::_WebSocketConnect()
{
#ifdef _WIN32
    std::lock_guard<std::mutex> lock(_wsMutex);

    if (_wsConnected) {
        return true;
    }

    // Parse WebSocket URL: ws://host:port/path
    std::string url = _wsUrl;
    std::string host;
    int port = 9999;
    std::string path = "/ws";

    // Remove ws:// prefix
    if (url.substr(0, 5) == "ws://") {
        url = url.substr(5);
    }

    // Extract path
    size_t pathPos = url.find('/');
    if (pathPos != std::string::npos) {
        path = url.substr(pathPos);
        url = url.substr(0, pathPos);
    }

    // Extract port
    size_t colonPos = url.find(':');
    if (colonPos != std::string::npos) {
        try { port = std::stoi(url.substr(colonPos + 1)); } catch (...) { /* keep default port on malformed/empty/IPv6 port */ }
        host = url.substr(0, colonPos);
    } else {
        host = url;
    }

    // Add client_id to path
    std::string fullPath = path + "?clientId=" + _clientId;

    std::ofstream debugLog;
    if (TfDebug::IsEnabled(HD_CARWASH)) {
        debugLog.open("C:/Temp/hdcarwash_debug.txt", std::ios::app);  // gated: no hot-path I/O / workflow-JSON leak unless TF_DEBUG=HD_CARWASH (#5)
    }
    debugLog << "[WebSocket] Connecting to " << host << ":" << port << fullPath << std::endl;

    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        debugLog << "[WebSocket] ERROR: Failed to create socket" << std::endl;
        debugLog.close();
        return false;
    }

    // Resolve hostname
    struct addrinfo hints = {}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
        debugLog << "[WebSocket] ERROR: Failed to resolve host" << std::endl;
        debugLog.close();
        closesocket(sock);
        return false;
    }

    // Connect
    if (connect(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        debugLog << "[WebSocket] ERROR: Failed to connect" << std::endl;
        debugLog.close();
        freeaddrinfo(result);
        closesocket(sock);
        return false;
    }
    freeaddrinfo(result);

    // Generate WebSocket key (base64 of 16 random bytes)
    // For simplicity, use a fixed key (ComfyUI doesn't validate the key)
    std::string wsKey = "dGhlIHNhbXBsZSBub25jZQ==";

    // Send WebSocket handshake
    std::ostringstream handshake;
    handshake << "GET " << fullPath << " HTTP/1.1\r\n";
    handshake << "Host: " << host << ":" << port << "\r\n";
    handshake << "Upgrade: websocket\r\n";
    handshake << "Connection: Upgrade\r\n";
    handshake << "Sec-WebSocket-Key: " << wsKey << "\r\n";
    handshake << "Sec-WebSocket-Version: 13\r\n";
    handshake << "\r\n";

    std::string req = handshake.str();
    if (send(sock, req.c_str(), (int)req.size(), 0) == SOCKET_ERROR) {
        debugLog << "[WebSocket] ERROR: Failed to send handshake" << std::endl;
        debugLog.close();
        closesocket(sock);
        return false;
    }

    // Receive handshake response
    char buffer[1024];
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) {
        debugLog << "[WebSocket] ERROR: No handshake response" << std::endl;
        debugLog.close();
        closesocket(sock);
        return false;
    }
    buffer[bytesReceived] = '\0';

    // Check for 101 Switching Protocols
    std::string response(buffer);
    if (response.find("101") == std::string::npos) {
        debugLog << "[WebSocket] ERROR: Handshake failed: " << response.substr(0, 100) << std::endl;
        debugLog.close();
        closesocket(sock);
        return false;
    }

    // Set socket to non-blocking for timeouts
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    _wsSocket = reinterpret_cast<void*>(sock);
    _wsStop = false;  // fresh connection: clear any prior stop signal (#5d)
    _wsConnected = true;

    debugLog << "[WebSocket] Connected successfully" << std::endl;
    debugLog.close();

    TF_DEBUG_MSG(HD_CARWASH, "WebSocket connected to %s:%d%s\n",
                 host.c_str(), port, fullPath.c_str());
    return true;
#else
    return false;
#endif
}

void
HdCarWashComfyClient::_WebSocketDisconnect()
{
#ifdef _WIN32
    std::lock_guard<std::mutex> lock(_wsMutex);

    // Signal the receive loop to stop and mark the connection down BEFORE closing
    // the socket, so a thread parked in _WebSocketReceive() observes it and exits.
    _wsStop = true;
    _wsConnected = false;

    // Wait until no thread is inside select()/recv() before closing the handle
    // (#5d use-after-close). The receive loop uses a bounded select timeout and a
    // non-blocking socket, so this clears quickly; cap the wait defensively. We do
    // not take any lock the receive path needs, so this cannot deadlock.
    for (int i = 0; i < 400 && _wsReceiving.load() > 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (_wsSocket) {
        SOCKET sock = reinterpret_cast<SOCKET>(_wsSocket);
        closesocket(sock);
        _wsSocket = nullptr;
    }
#endif
}

bool
HdCarWashComfyClient::_WebSocketSend(const std::string& message)
{
#ifdef _WIN32
    std::lock_guard<std::mutex> lock(_wsMutex);

    if (!_wsConnected || !_wsSocket) {
        return false;
    }

    SOCKET sock = reinterpret_cast<SOCKET>(_wsSocket);

    // Build WebSocket frame (text frame, masked)
    std::vector<uint8_t> frame;

    // Opcode: 0x81 = text frame, FIN bit set
    frame.push_back(0x81);

    // Payload length (masked)
    size_t len = message.size();
    if (len <= 125) {
        frame.push_back(static_cast<uint8_t>(len | 0x80));  // Mask bit set
    } else if (len <= 65535) {
        frame.push_back(126 | 0x80);
        frame.push_back((len >> 8) & 0xff);
        frame.push_back(len & 0xff);
    } else {
        frame.push_back(127 | 0x80);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((len >> (i * 8)) & 0xff);
        }
    }

    // Masking key (simple key for client frames)
    uint8_t mask[4] = {0x12, 0x34, 0x56, 0x78};
    frame.insert(frame.end(), mask, mask + 4);

    // Masked payload
    for (size_t i = 0; i < message.size(); i++) {
        frame.push_back(message[i] ^ mask[i % 4]);
    }

    int sent = send(sock, reinterpret_cast<const char*>(frame.data()),
                    static_cast<int>(frame.size()), 0);
    return sent == static_cast<int>(frame.size());
#else
    return false;
#endif
}

std::string
HdCarWashComfyClient::_WebSocketReceive(int timeoutMs)
{
#ifdef _WIN32
    // Mark this thread as actively reading; disconnect waits for this to hit 0
    // before closesocket(), so the handle can't be closed mid-select/recv (#5d).
    _wsReceiving.fetch_add(1);
    struct ReceivingGuard {
        std::atomic<int>& flag;
        ~ReceivingGuard() { flag.fetch_sub(1); }
    } receivingGuard{_wsReceiving};

    if (_wsStop || !_wsConnected || !_wsSocket) {
        return "";
    }

    SOCKET sock = reinterpret_cast<SOCKET>(_wsSocket);

    // Use select for timeout
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);

    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    int selectResult = select(0, &readSet, nullptr, nullptr, &tv);
    if (selectResult <= 0) {
        return "";  // Timeout or error
    }

    // Read WebSocket frame header
    uint8_t header[2];
    int received = recv(sock, reinterpret_cast<char*>(header), 2, 0);
    if (received != 2) {
        _wsConnected = false;
        return "";
    }

    // Parse header
    bool fin = (header[0] & 0x80) != 0;
    uint8_t opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    uint64_t payloadLen = header[1] & 0x7F;

    // Extended payload length
    if (payloadLen == 126) {
        uint8_t extLen[2];
        if (recv(sock, reinterpret_cast<char*>(extLen), 2, 0) != 2) {
            return "";
        }
        payloadLen = (extLen[0] << 8) | extLen[1];
    } else if (payloadLen == 127) {
        uint8_t extLen[8];
        if (recv(sock, reinterpret_cast<char*>(extLen), 8, 0) != 8) {
            return "";
        }
        payloadLen = 0;
        for (int i = 0; i < 8; i++) {
            payloadLen = (payloadLen << 8) | extLen[i];
        }
    }

    // Masking key (server frames are typically not masked)
    uint8_t mask[4] = {0, 0, 0, 0};
    if (masked) {
        if (recv(sock, reinterpret_cast<char*>(mask), 4, 0) != 4) {
            return "";
        }
    }

    // Payload (limit to reasonable size)
    if (payloadLen > 1024 * 1024) {
        return "";
    }

    std::string payload(payloadLen, '\0');
    size_t totalReceived = 0;
    while (totalReceived < payloadLen) {
        int chunk = recv(sock, &payload[totalReceived],
                        static_cast<int>(payloadLen - totalReceived), 0);
        if (chunk <= 0) {
            return "";
        }
        totalReceived += chunk;
    }

    // Unmask if needed
    if (masked) {
        for (size_t i = 0; i < payload.size(); i++) {
            payload[i] ^= mask[i % 4];
        }
    }

    // Handle different opcodes
    if (opcode == 0x08) {
        // Close frame
        _wsConnected = false;
        return "";
    } else if (opcode == 0x09) {
        // Ping - respond with pong
        std::vector<uint8_t> pong = {0x8A, 0x00};  // Pong, no payload
        send(sock, reinterpret_cast<const char*>(pong.data()), 2, 0);
        return _WebSocketReceive(timeoutMs);  // Continue receiving
    } else if (opcode == 0x0A) {
        // Pong - ignore
        return _WebSocketReceive(timeoutMs);
    }

    return payload;
#else
    return "";
#endif
}

bool
HdCarWashComfyClient::_WaitForCompletionWebSocket(const std::string& promptId, float timeoutSeconds)
{
    std::ofstream debugLog;
    if (TfDebug::IsEnabled(HD_CARWASH)) {
        debugLog.open("C:/Temp/hdcarwash_debug.txt", std::ios::app);  // gated: no hot-path I/O / workflow-JSON leak unless TF_DEBUG=HD_CARWASH (#5)
    }
    debugLog << "[WebSocket] Waiting for completion of prompt: " << promptId << std::endl;

    // Connect to WebSocket
    if (!_WebSocketConnect()) {
        debugLog << "[WebSocket] Failed to connect, falling back to polling" << std::endl;
        debugLog.close();
        return false;
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    bool completed = false;
    bool hasError = false;

    while (!_cancelRequested && !completed && !hasError) {
        // Check timeout
        auto now = std::chrono::high_resolution_clock::now();
        float elapsed = std::chrono::duration<float>(now - startTime).count();
        if (elapsed > timeoutSeconds) {
            debugLog << "[WebSocket] Timeout after " << elapsed << "s" << std::endl;
            break;
        }

        // Receive message
        std::string message = _WebSocketReceive(500);  // 500ms timeout per receive

        if (!_wsConnected) {
            debugLog << "[WebSocket] Connection lost" << std::endl;
            break;
        }

        if (message.empty()) {
            continue;  // Timeout, try again
        }

        // Parse message
        if (_ParseWebSocketMessage(message, promptId)) {
            completed = true;
            debugLog << "[WebSocket] Completion detected!" << std::endl;
        }

        // Check for errors
        if (message.find("\"type\": \"execution_error\"") != std::string::npos ||
            message.find("\"type\":\"execution_error\"") != std::string::npos) {
            hasError = true;
            debugLog << "[WebSocket] Execution error detected" << std::endl;
            TF_WARN("ComfyUI WebSocket reported execution error");
        }
    }

    debugLog.close();

    // Keep connection open for future use (will be closed in destructor)
    return completed && !hasError;
}

bool
HdCarWashComfyClient::_ParseWebSocketMessage(const std::string& message, const std::string& promptId)
{
    // ComfyUI WebSocket messages:
    // {"type": "status", "data": {"status": {"exec_info": {"queue_remaining": N}}}}
    // {"type": "executing", "data": {"node": "N", "prompt_id": "xxx"}}
    // {"type": "executed", "data": {"node": "N", "output": {...}, "prompt_id": "xxx"}}
    // {"type": "execution_cached", "data": {"nodes": [...], "prompt_id": "xxx"}}
    // {"type": "progress", "data": {"value": N, "max": M, "prompt_id": "xxx"}}

    // Progress updates: capture these BEFORE the relevance filter below — older
    // ComfyUI builds omit the prompt_id from progress messages. Only one job
    // runs at a time (the render pass gates on _aiProcessing), so any progress
    // we see is ours. (#6)
    if (message.find("\"type\": \"progress\"") != std::string::npos ||
        message.find("\"type\":\"progress\"") != std::string::npos) {
        auto extractInt = [&message](const char* key, int fallback) -> int {
            size_t k = message.find(key);
            if (k == std::string::npos) return fallback;
            size_t colon = message.find(':', k);
            if (colon == std::string::npos) return fallback;
            size_t p = colon + 1;
            while (p < message.size() && (message[p] == ' ' || message[p] == '\t')) ++p;
            try { return std::stoi(message.substr(p)); } catch (...) { return fallback; }
        };
        int v = extractInt("\"value\"", _progressValue.load());
        int m = extractInt("\"max\"", _progressMax.load());
        if (m > 0) {
            _progressMax.store(m);
            _progressValue.store(v);
        }
        return false;  // progress is not completion
    }

    // Check if this message is for our prompt
    if (message.find(promptId) == std::string::npos &&
        message.find("\"type\": \"status\"") == std::string::npos &&
        message.find("\"type\":\"status\"") == std::string::npos) {
        return false;  // Not relevant to us
    }

    // Check for execution_cached (workflow completed from cache)
    if (message.find("\"type\": \"execution_cached\"") != std::string::npos ||
        message.find("\"type\":\"execution_cached\"") != std::string::npos) {
        return true;
    }

    // Check for executed (final node completed)
    // The SaveImage node is typically the last one
    if ((message.find("\"type\": \"executed\"") != std::string::npos ||
         message.find("\"type\":\"executed\"") != std::string::npos) &&
        message.find("SaveImage") != std::string::npos) {
        return true;
    }

    // Check for queue empty (all work done)
    if (message.find("\"queue_remaining\": 0") != std::string::npos ||
        message.find("\"queue_remaining\":0") != std::string::npos) {
        // Queue is empty, but verify our prompt was in it
        // This is a fallback check
        return false;  // Don't complete just on queue empty, wait for executed
    }

    // Check for "executing": null (nothing being executed = done)
    if (message.find("\"executing\"") != std::string::npos &&
        message.find("\"node\": null") != std::string::npos) {
        return true;
    }

    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
