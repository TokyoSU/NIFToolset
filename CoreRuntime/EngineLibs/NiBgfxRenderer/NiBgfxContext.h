#pragma once
#ifndef NIBGFXCONTEXT_H
#define NIBGFXCONTEXT_H

#include "NiBgfxRendererLibType.h"

#include <bgfx/platform.h>

#include <cstdint>
#include <memory>
#include <vector>

class NIBGFXRENDERER_ENTRY NiBgfxContext
{
public:
	NiBgfxContext() = default;
	~NiBgfxContext();

	NiBgfxContext(const NiBgfxContext&) = delete;
	NiBgfxContext& operator=(const NiBgfxContext&) = delete;

	bool Initialize(void* nativeWindowHandle, unsigned int width,
		unsigned int height, bool vsync);
	void Shutdown();
	void Reset(unsigned int width, unsigned int height, bool vsync);
	unsigned int Frame();

	// Captures a native-window back buffer through bgfx::CallbackI. Pass an
	// invalid framebuffer handle for the main swap chain. BGRA8 is bgfx's
	// screenshot contract; callers can convert/crop it.
	bool CaptureFrameBuffer(bgfx::FrameBufferHandle frameBuffer,
		std::vector<std::uint8_t>& pixels,
		unsigned int& width, unsigned int& height, unsigned int& pitch,
		bool& yFlip);

	bool IsInitialized() const;

private:
	bool m_initialized = false;
	std::unique_ptr<bgfx::CallbackI> m_callback;
};

#endif
