// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// comfyClient.h — ComfyUI WebSocket/HTTP client for AI backend

#ifndef HD_CARWASH_COMFY_CLIENT_H
#define HD_CARWASH_COMFY_CLIENT_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/token.h"

#include "api.h"
#include "rasterizer.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <future>
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

/// \struct HdCarWashStyleParams
///
/// Parameters for AI stylization passed to ComfyUI
///
struct HDCARWASH_API HdCarWashStyleParams
{
    std::string prompt = "anime style, vibrant colors, detailed";
    std::string negativePrompt = "blurry, low quality, distorted";
    int inferenceSteps = 20;
    float guidanceScale = 7.5f;
    float controlNetStrength = 0.8f;       // Depth ControlNet strength
    float normalControlNetStrength = 0.6f; // Normal ControlNet strength
    int seed = 42;
    bool useDepthControl = true;
    bool useNormalControl = false;  // Disabled by default - requires canny ControlNet model
    bool useEdgeControl = false;
    // When true, the workflow seed is NOT time-jittered, so the same scene+seed
    // reproduces the same ComfyUI render (and cache-hits). Default false keeps
    // the existing cache-busting behavior; wire this from the deterministicMode
    // render setting to make the AI submission deterministic. (#3b)
    bool deterministic = false;
};

/// \struct HdCarWashRenderResult
///
/// Result from ComfyUI processing
///
struct HDCARWASH_API HdCarWashRenderResult
{
    bool success = false;
    std::string errorMessage;
    std::vector<GfVec4f> styledImage;  // RGBA output (frame 0 — viewport preview)
    unsigned int width = 0;
    unsigned int height = 0;
    std::string promptId;  // ComfyUI job ID for tracking
    float inferenceTimeMs = 0.0f;
    unsigned int frameCount = 0;   // number of frames written to disk (#3)
    std::string outputPath;        // per-generation directory holding the sequence + sidecar (#3)
};

/// \class HdCarWashComfyClient
///
/// Client for communicating with ComfyUI server.
/// Sends AOV buffers as ControlNet conditioning, receives stylized output.
///
/// Architecture:
/// 1. Encode AOV buffers to PNG images (base64)
/// 2. Submit workflow to ComfyUI via HTTP POST
/// 3. Poll for completion or use WebSocket for real-time updates
/// 4. Download result image and decode back to buffer
///
class HDCARWASH_API HdCarWashComfyClient
{
public:
    /// Construct client with server URL
    /// Construct client with server URL
    /// @param serverUrl ComfyUI HTTP server URL (default: http://127.0.0.1:8188)
    /// @param wsUrl WebSocket URL for progress updates (default: ws://localhost:9999)
    explicit HdCarWashComfyClient(
        const std::string& serverUrl = "http://127.0.0.1:8188",
        const std::string& wsUrl = "ws://localhost:9999");

    ~HdCarWashComfyClient();

    // Non-copyable
    HdCarWashComfyClient(const HdCarWashComfyClient&) = delete;
    HdCarWashComfyClient& operator=(const HdCarWashComfyClient&) = delete;

    /// Check if ComfyUI server is reachable
    bool IsServerAvailable() const;

    /// Get server URL
    const std::string& GetServerUrl() const { return _serverUrl; }

    /// Set server URL
    void SetServerUrl(const std::string& url) { _serverUrl = url; }

    /// Process a frame through ComfyUI
    /// @param framebuffer The AOV buffers from CPU rasterization
    /// @param params Style parameters for the AI
    /// @return Result containing stylized image or error
    HdCarWashRenderResult ProcessFrame(
        const HdCarWashFramebuffer& framebuffer,
        const HdCarWashStyleParams& params);

    /// Async version of ProcessFrame
    std::future<HdCarWashRenderResult> ProcessFrameAsync(
        const HdCarWashFramebuffer& framebuffer,
        const HdCarWashStyleParams& params);

    /// Cancel a pending async operation
    void CancelPending();

    /// Set the workflow template path
    void SetWorkflowPath(const std::string& path) { _workflowPath = path; }

    /// Set the base directory for downloaded render sequences. Each generation
    /// gets a `<outputDir>/<promptId>/` subfolder with frame_####.png + a
    /// carwash.json sidecar. Should be wired to the carwash:outputDirectory
    /// render setting (or $HIP) once that is plumbed. (#3)
    void SetOutputDir(const std::string& dir) { _outputDir = dir; }

    /// Set the backend type (ltx2, flux, cosmos)
    void SetBackend(const TfToken& backend) { _backend = backend; }

    /// Get current backend
    const TfToken& GetBackend() const { return _backend; }

    // =========================================================================
    // Buffer Encoding Utilities
    // =========================================================================

    /// Encode depth buffer to PNG (base64)
    static std::string EncodeDepthBuffer(
        const std::vector<float>& depth,
        unsigned int width, unsigned int height);

    /// Encode normal buffer to PNG (base64)
    static std::string EncodeNormalBuffer(
        const std::vector<GfVec3f>& normals,
        unsigned int width, unsigned int height);

    /// Encode color buffer to PNG (base64)
    static std::string EncodeColorBuffer(
        const std::vector<GfVec4f>& color,
        unsigned int width, unsigned int height);

    /// Encode ID buffer to segmentation mask PNG (base64)
    static std::string EncodeIdBuffer(
        const std::vector<int32_t>& ids,
        unsigned int width, unsigned int height);

    /// Decode PNG (base64) back to color buffer
    static std::vector<GfVec4f> DecodeColorImage(
        const std::string& base64Png,
        unsigned int& outWidth, unsigned int& outHeight);

private:
    /// Build workflow JSON from template and parameters
    std::string _BuildWorkflow(
        const HdCarWashFramebuffer& framebuffer,
        const HdCarWashStyleParams& params);

    /// Submit workflow to ComfyUI and get prompt ID
    std::string _SubmitWorkflow(const std::string& workflowJson);

    /// Poll for workflow completion
    bool _WaitForCompletion(const std::string& promptId, float timeoutSeconds);

    /// Download the full result sequence from ComfyUI. Writes every frame to a
    /// per-generation directory (with a sidecar JSON) and returns the first
    /// frame decoded as RGBA for the viewport preview. Sets frameCount and
    /// outputDir to describe what was written. (#3)
    std::vector<GfVec4f> _DownloadResult(
        const std::string& promptId,
        const HdCarWashStyleParams& params,
        unsigned int& width, unsigned int& height,
        unsigned int& frameCount, std::string& outputDir);

    /// HTTP GET request
    std::string _HttpGet(const std::string& endpoint);

    /// HTTP POST request
    std::string _HttpPost(const std::string& endpoint, const std::string& body);

    /// Upload image to ComfyUI via /upload/image API (multipart/form-data)
    /// Returns JSON response with filename info, or empty on failure
    std::string _UploadImageToComfyUI(
        const std::vector<uint8_t>& pngData,
        const std::string& filename,
        const std::string& subfolder);

    /// Save control images (depth, normal) to ComfyUI input folder
    /// Returns the subfolder name where images were saved
    std::string _SaveControlImages(
        const HdCarWashFramebuffer& framebuffer,
        const HdCarWashStyleParams& params);

    /// Build workflow JSON with ControlNet (img2img style)
    std::string _BuildWorkflowControlNet(
        const HdCarWashFramebuffer& framebuffer,
        const HdCarWashStyleParams& params,
        const std::string& controlImageSubfolder);

    /// Build workflow JSON for LTX2 image-to-video generation
    std::string _BuildWorkflowLTX2(
        const HdCarWashFramebuffer& framebuffer,
        const HdCarWashStyleParams& params,
        const std::string& controlImageSubfolder);

    /// Connect to WebSocket server
    bool _WebSocketConnect();

    /// Disconnect from WebSocket
    void _WebSocketDisconnect();

    /// Send WebSocket message
    bool _WebSocketSend(const std::string& message);

    /// Receive WebSocket message (blocking with timeout)
    std::string _WebSocketReceive(int timeoutMs = 1000);

    /// Wait for completion using WebSocket (real-time progress)
    bool _WaitForCompletionWebSocket(const std::string& promptId, float timeoutSeconds);

    /// Parse WebSocket message and check for completion
    bool _ParseWebSocketMessage(const std::string& message, const std::string& promptId);

    std::string _serverUrl;
    std::string _wsUrl;
    std::string _workflowPath;
    TfToken _backend;
    std::atomic<bool> _cancelRequested{false};
    bool _useWebSocket = true;  // Prefer WebSocket over polling

    // Client ID for ComfyUI session
    std::string _clientId;

    // WebSocket connection state
#ifdef _WIN32
    void* _wsSocket = nullptr;  // SOCKET as void* to avoid header pollution
#endif
    std::atomic<bool> _wsConnected{false};
    // Teardown coordination (#5d): _WebSocketReceive runs on the ProcessFrameAsync
    // thread; _WebSocketDisconnect (destructor/cancel) must not closesocket() while
    // that thread is inside select()/recv(). _wsStop signals the receive loop to
    // exit; _wsReceiving counts threads currently in select/recv so disconnect can
    // wait for 0 before closing the handle (eliminates use-after-close).
    std::atomic<bool> _wsStop{false};
    std::atomic<int> _wsReceiving{0};
    std::mutex _wsMutex;

    // ComfyUI input directory for control images
    // Default: C:\ComfyUI\input (standard ComfyUI location)
    std::string _comfyInputDir = "C:/ComfyUI/input";

    // Base directory for downloaded render sequences (#3). Each generation
    // writes <_outputDir>/<promptId>/frame_####.png + carwash.json.
    std::string _outputDir = "C:/CarWashRenders";
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_CARWASH_COMFY_CLIENT_H
