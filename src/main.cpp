/*  Copyright 2019 Ash Logan "quarktheawesome" <ash@heyquark.com>
    Copyright 2019 Maschell

    Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <wups.h>
#include <memory/mappedmemory.h>
#include <string.h>
#include <utils/logger.h>
#include <whb/log_udp.h>
#include <coreinit/filesystem.h>
#include <gx2/swap.h>
#include <gx2/enum.h>
#include <gx2/surface.h>
#include <gx2/mem.h>
#include <gx2/context.h>
#include <gx2/state.h>
#include <gx2/clear.h>
#include <gx2/event.h>
#include <coreinit/cache.h>
#include <coreinit/debug.h>

#include "shaders/Texture2DShader.h"

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("VideoSquoosher");
WUPS_PLUGIN_DESCRIPTION("Squooshes the Gamepad and TV video side-by-side, " \
    "displaying that on your TV");
WUPS_PLUGIN_VERSION("v1.0");
WUPS_PLUGIN_AUTHOR("Maschell & quarktheawesome");
WUPS_PLUGIN_LICENSE("ISC");

GX2ColorBuffer main_cbuf;
GX2Texture drcTex;
GX2Texture tvTex;
GX2Sampler sampler;
GX2ContextState* ownContextState;
GX2ContextState* originalContextSave = NULL;
bool inForeground = true;

/**
    Add this to one of your projects file to have access to SD/USB.
**/
//WUPS_FS_ACCESS()


INITIALIZE_PLUGIN() {
    memset(&main_cbuf, 0, sizeof(GX2ColorBuffer));
    WHBLogUdpInit();

    DEBUG_FUNCTION_LINE("VideoSquoosher: Hi!\n");
}

void freeUsedMemory(){
    if (main_cbuf.surface.image) {
        MEMFreeToMappedMemory(main_cbuf.surface.image);
        main_cbuf.surface.image = NULL;
    }

    if (drcTex.surface.image) {
        MEMFreeToMappedMemory(drcTex.surface.image);
        drcTex.surface.image = NULL;
    }

    if (tvTex.surface.image) {
        MEMFreeToMappedMemory(tvTex.surface.image);
        tvTex.surface.image = NULL;
    }

    if(ownContextState){
        MEMFreeToMappedMemory(ownContextState);
        ownContextState = NULL;
    }
}

ON_APPLICATION_ENDS() {
    DEBUG_FUNCTION_LINE("VideoSquoosher: shutting down...\n");

    freeUsedMemory();

    Texture2DShader::destroyInstance();

    WHBLogUdpDeinit();
}

void copyToTexture(GX2ColorBuffer* sourceBuffer, GX2Texture * target){
    if(sourceBuffer == NULL || target == NULL){
        return;
    }
    if (sourceBuffer->surface.aa == GX2_AA_MODE1X) {
        // If AA is disabled, we can simply use GX2CopySurface.
        GX2CopySurface(&sourceBuffer->surface,
            sourceBuffer->viewMip,
            sourceBuffer->viewFirstSlice,
            &target->surface, 0, 0);
    } else {
        // If AA is enabled, we need to resolve the AA buffer.

        // Allocate surface to resolve buffer onto
        GX2Surface tempSurface;
        tempSurface = sourceBuffer->surface;
        tempSurface.aa = GX2_AA_MODE1X;
        GX2CalcSurfaceSizeAndAlignment(&tempSurface);

        tempSurface.image = MEMAllocFromMappedMemoryForGX2Ex(
            tempSurface.imageSize,
            tempSurface.alignment
        );
        if(tempSurface.image == NULL) {
            DEBUG_FUNCTION_LINE("VideoSquoosher: failed to allocate AA surface\n");
            if(target->surface.image != NULL) {
                MEMFreeToMappedMemory(target->surface.image);
                target->surface.image = NULL;
            }
            return;
        }

        // Resolve, then copy result to target
        GX2ResolveAAColorBuffer(sourceBuffer,&tempSurface, 0, 0);
        GX2CopySurface(&tempSurface, 0, 0,&target->surface, 0, 0);

        if(tempSurface.image != NULL) {
            MEMFreeToMappedMemory(tempSurface.image);
            tempSurface.image = NULL;
        }
    }
}

void drawTexture(GX2Texture * texture, GX2Sampler* sampler, float x, float y, int32_t width, int32_t height){
    float widthScaleFactor = 1.0f / (float)1280;
    float heightScaleFactor = 1.0f / (float)720;

    glm::vec3 positionOffsets = glm::vec3(0.0f);

    positionOffsets[0] = (x-((1280)/2)+(width/2)) * widthScaleFactor * 2.0f;
    positionOffsets[1] = -(y-((720)/2)+(height/2)) * heightScaleFactor * 2.0f;

    glm::vec3 scale(width*widthScaleFactor,height*heightScaleFactor,1.0f);

    Texture2DShader::instance()->setShaders();
    Texture2DShader::instance()->setAttributeBuffer();
    Texture2DShader::instance()->setAngle(0.0f);
    Texture2DShader::instance()->setOffset(positionOffsets);
    Texture2DShader::instance()->setScale(scale);
    Texture2DShader::instance()->setColorIntensity(glm::vec4(1.0f));
    Texture2DShader::instance()->setBlurring(glm::vec3(0.0f));
    Texture2DShader::instance()->setTextureAndSampler(texture, sampler);
    Texture2DShader::instance()->draw();
}

DECL_FUNCTION(void, GX2SetContextState, GX2ContextState * curContext) {
    if(inForeground) {
        originalContextSave = curContext;
    }
    real_GX2SetContextState(curContext);
}

ON_RELEASE_FOREGROUND() {
  if (main_cbuf.surface.image) {
      MEMFreeToMappedMemory(main_cbuf.surface.image);
      main_cbuf.surface.image = NULL;
  }
  memset(&main_cbuf, 0, sizeof(GX2ColorBuffer));

  inForeground = false;
  DEBUG_FUNCTION_LINE("VideoSquoosher: Moving to background\n");
}
ON_ACQUIRED_FOREGROUND() {
  inForeground = true;
}

DECL_FUNCTION(void, GX2CopyColorBufferToScanBuffer, GX2ColorBuffer* cbuf, GX2ScanTarget target) {
    if (!inForeground) {
        real_GX2CopyColorBufferToScanBuffer(cbuf, target);
        return;
    }

    if (!main_cbuf.surface.image) {
        freeUsedMemory();

        GX2InitColorBuffer(&main_cbuf,
            GX2_SURFACE_DIM_TEXTURE_2D,
            1280, 720, 1,
            GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8,
            (GX2AAMode)GX2_AA_MODE1X
        );

        if (main_cbuf.surface.imageSize) {
            DEBUG_FUNCTION_LINE("allocating main_cbuf %08x, align %08x\n", main_cbuf.surface.imageSize, main_cbuf.surface.alignment);
            main_cbuf.surface.image = MEMAllocFromMappedMemoryForGX2Ex(
                main_cbuf.surface.imageSize,
                main_cbuf.surface.alignment
            );
            if(main_cbuf.surface.image == NULL){
                OSFatal("VideoSquoosher: Failed to alloc main_cbuf\n");
            }

            DEBUG_FUNCTION_LINE("VideoSquoosher: allocated %dx%d cbuf %08X\n",
                main_cbuf.surface.width,
                main_cbuf.surface.height,
                main_cbuf.surface.image);
        } else {
            DEBUG_FUNCTION_LINE("VideoSquoosher: GX2InitTexture failed for main_cbuf!\n");
        }

        GX2InitTexture(&drcTex,
            854, 480, 1, 0,
            GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8,
            GX2_SURFACE_DIM_TEXTURE_2D,
            GX2_TILE_MODE_LINEAR_ALIGNED
        );
        drcTex.surface.use = (GX2SurfaceUse)
            (GX2_SURFACE_USE_COLOR_BUFFER | GX2_SURFACE_USE_TEXTURE);

        if (drcTex.surface.imageSize) {
            DEBUG_FUNCTION_LINE("allocating drcTex %08x, align %08x\n", drcTex.surface.imageSize, drcTex.surface.alignment);
            drcTex.surface.image = MEMAllocFromMappedMemoryForGX2Ex(
                drcTex.surface.imageSize,
                drcTex.surface.alignment
            );
            if(drcTex.surface.image == NULL){
                OSFatal("VideoSquoosher: Failed to alloc drcTex\n");
            }

            DEBUG_FUNCTION_LINE("VideoSquoosher: allocated %dx%d drcTex %08X\n",
                drcTex.surface.width,
                drcTex.surface.height,
                drcTex.surface.image);
        } else {
            DEBUG_FUNCTION_LINE("VideoSquoosher: GX2InitTexture failed for drcTex!\n");
        }

        GX2InitTexture(&tvTex,
            1280, 720, 1, 0,
            GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8,
            GX2_SURFACE_DIM_TEXTURE_2D,
            GX2_TILE_MODE_LINEAR_ALIGNED
        );
        tvTex.surface.use = (GX2SurfaceUse)
            (GX2_SURFACE_USE_COLOR_BUFFER | GX2_SURFACE_USE_TEXTURE);

        if (tvTex.surface.imageSize) {
            DEBUG_FUNCTION_LINE("allocating tvTex %08x, align %08x\n", tvTex.surface.imageSize, tvTex.surface.alignment);
            tvTex.surface.image = MEMAllocFromMappedMemoryForGX2Ex(
                tvTex.surface.imageSize,
                tvTex.surface.alignment
            );
            if(tvTex.surface.image == NULL){
                OSFatal("VideoSquoosher: Failed to alloc tvTex\n");
            }

            DEBUG_FUNCTION_LINE("VideoSquoosher: allocated %dx%d tvTex %08X\n",
                tvTex.surface.width,
                tvTex.surface.height,
                tvTex.surface.image);
        } else {
            DEBUG_FUNCTION_LINE("VideoSquoosher: GX2InitTexture failed for tvTex!\n");
        }

        GX2InitSampler(&sampler,
            GX2_TEX_CLAMP_MODE_CLAMP,
            GX2_TEX_XY_FILTER_MODE_LINEAR
        );

        ownContextState = (GX2ContextState*)MEMAllocFromMappedMemoryForGX2Ex(
            sizeof(GX2ContextState),
            GX2_CONTEXT_STATE_ALIGNMENT
        );
        if(ownContextState == NULL){
            OSFatal("VideoSquoosher: Failed to alloc ownContextState\n");
        }
        GX2SetupContextStateEx(ownContextState, GX2_TRUE);

        GX2SetContextState(ownContextState);
        GX2SetColorBuffer(&main_cbuf, GX2_RENDER_TARGET_0);
        GX2SetContextState(originalContextSave);
    }

    if(main_cbuf.surface.image){
        if (target == GX2_SCAN_TARGET_DRC) {
            copyToTexture(cbuf,&drcTex);
        } else if (target == GX2_SCAN_TARGET_TV) {
            copyToTexture(cbuf,&tvTex);

            GX2SetContextState(ownContextState);

            GX2ClearColor(main_cbuf, (float)0x56 / 255, (float)0x7a / 255, (float)0xfc / 255, 1.0f);

            GX2SetContextState(ownContextState);

            GX2SetViewport(
                0.0f, 0.0f,
                main_cbuf.surface.width, main_cbuf.surface.height,
                0.0f, 1.0f
            );
            GX2SetScissor(
                0, 0,
                main_cbuf.surface.width, main_cbuf.surface.height
            );

            // draw DRC
            drawTexture(&drcTex, &sampler, 0, 0, 1280/2, 720);

            // draw TV
            drawTexture(&tvTex, &sampler, 1280/2, 0, 1280/2, 720);

            GX2SetContextState(originalContextSave);

            real_GX2CopyColorBufferToScanBuffer(&main_cbuf, target);
            return;
        }
    }else{
        DEBUG_FUNCTION_LINE("VideoSquoosher: main_cbuf.surface.image is null \n");
    }

    real_GX2CopyColorBufferToScanBuffer(cbuf, target);
}

WUPS_MUST_REPLACE(GX2CopyColorBufferToScanBuffer, WUPS_LOADER_LIBRARY_GX2, GX2CopyColorBufferToScanBuffer);
WUPS_MUST_REPLACE(GX2SetContextState, WUPS_LOADER_LIBRARY_GX2, GX2SetContextState);
