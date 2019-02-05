#include <wups.h>
#include <malloc.h>
#include <string.h>
#include <nsysnet/socket.h>
#include <utils/logger.h>
#include <coreinit/filesystem.h>
#include <gx2/swap.h>
#include <gx2/enum.h>
#include <gx2/surface.h>
#include <gx2/mem.h>
#include <gx2/context.h>
#include <gx2/state.h>
#include <gx2/clear.h>

#include "shaders/Texture2DShader.h"

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("Example plugin");
WUPS_PLUGIN_DESCRIPTION("This is just an example plugin and will log the FSOpenFile function.");
WUPS_PLUGIN_VERSION("v1.0");
WUPS_PLUGIN_AUTHOR("Maschell");
WUPS_PLUGIN_LICENSE("BSD");

GX2ColorBuffer main_cbuf;
GX2Texture tex;
GX2Sampler sampler;
GX2ContextState ctx;

/**
    Add this to one of your projects file to have access to SD/USB.
**/
WUPS_FS_ACCESS()

/**
    All of this defines can be used in ANY file.
    It's possible to split it up into multiple files.

**/

/**
    Get's called ONCE when the loader exits, but BEFORE the ON_APPLICATION_START gets called or functions are overridden.
**/
INITIALIZE_PLUGIN(){
    socket_lib_init();
    log_init();
DEBUG_FUNCTION_LINE("INITIALIZE_PLUGIN of example_plugin!\n");
}


DEINITIALIZE_PLUGIN(){
    socket_lib_init();
    log_init();
    DEBUG_FUNCTION_LINE("DEINITIALIZE_PLUGIN of example_plugin!\n");
}
ON_APPLICATION_START(){
    socket_lib_init();
    log_init();

    DEBUG_FUNCTION_LINE("ON_APPLICATION_START of example_plugin!\n");

    memset(&main_cbuf, 0, sizeof(main_cbuf));
    main_cbuf.surface.width = 854;
    main_cbuf.surface.height = 480;
    main_cbuf.surface.format = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
    main_cbuf.surface.depth = 1;
    main_cbuf.surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
    main_cbuf.surface.tileMode = GX2_TILE_MODE_LINEAR_ALIGNED;
    main_cbuf.surface.use = (GX2SurfaceUse) (GX2_SURFACE_USE_TEXTURE | GX2_SURFACE_USE_COLOR_BUFFER);
    main_cbuf.surface.mipLevels = 1;
    main_cbuf.viewNumSlices = 1;

    tex.surface = main_cbuf.surface;
    tex.viewNumMips = 1;
    tex.viewNumSlices = 1;
    tex.compMap = 0x00010203;
}
ON_APPLICATION_ENDING(){
    DEBUG_FUNCTION_LINE("ON_APPLICATION_ENDING of example_plugin!\n");

    if (main_cbuf.surface.image) {
        free(main_cbuf.surface.image);
        main_cbuf.surface.image = NULL;
    }
    if (tex.surface.image) {
        free(tex.surface.image);
        tex.surface.image = NULL;
    }
    Texture2DShader::destroyInstance();
}

DECL_FUNCTION(void, GX2CopyColorBufferToScanBuffer, GX2ColorBuffer* cbuf, GX2ScanTarget target) {
    //DEBUG_FUNCTION_LINE("cbuf: %08X; target: %d\n", cbuf, target);

    if (!main_cbuf.surface.imageSize) {
        GX2CalcSurfaceSizeAndAlignment(&main_cbuf.surface);
        GX2InitColorBufferRegs(&main_cbuf);

        GX2CalcSurfaceSizeAndAlignment(&tex.surface);
        GX2InitTextureRegs(&tex);

        GX2SetupContextStateEx(&ctx, TRUE);
        GX2InitSampler(&sampler, GX2_TEX_CLAMP_MODE_WRAP, GX2_TEX_XY_FILTER_MODE_POINT);

        if (main_cbuf.surface.imageSize) {
            main_cbuf.surface.image = memalign(main_cbuf.surface.alignment, main_cbuf.surface.imageSize);
            DEBUG_FUNCTION_LINE("allocated cbuf %08X\n", main_cbuf.surface.image);
        } else {
            DEBUG_FUNCTION_LINE("didn't allocate cbuf?\n");
        }
        if (tex.surface.imageSize) {
            tex.surface.image = memalign(tex.surface.alignment, tex.surface.imageSize);
            DEBUG_FUNCTION_LINE("allocated tex %08X\n", tex.surface.image);
        } else {
            DEBUG_FUNCTION_LINE("didn't allocate tex?\n");
        }
    }

    if (target == GX2_SCAN_TARGET_DRC) {
        if (main_cbuf.surface.image) {
            //GX2SetContextState(&ctx);

            //GX2SetColorBuffer(&main_cbuf, GX2_RENDER_TARGET_0);

            //GX2SetViewport(0, 0, (float)main_cbuf.surface.width, (float)main_cbuf.surface.height, 0.0f, 1.0f);
            //GX2SetScissor(0, 0, (float)main_cbuf.surface.width, (float)main_cbuf.surface.height);

            GX2SetAlphaTest(TRUE, GX2_COMPARE_FUNC_GREATER, 0.0f);
            GX2SetDepthOnlyControl(FALSE, FALSE, GX2_COMPARE_FUNC_NEVER);
            GX2SetColorControl(GX2_LOGIC_OP_COPY, 0xFF, FALSE, TRUE);
            GX2SetCullOnlyControl(GX2_FRONT_FACE_CCW, FALSE, FALSE);

            GX2ClearColor(&main_cbuf, 1.0f, 0.0f, 0.0f, 1.0f);

            GX2Invalidate((GX2InvalidateMode)(GX2_INVALIDATE_MODE_COLOR_BUFFER | GX2_INVALIDATE_MODE_CPU_TEXTURE), cbuf->surface.image, cbuf->surface.imageSize);

            GX2CopySurface(&cbuf->surface, cbuf->viewMip, cbuf->viewFirstSlice, &tex.surface, 0, 0);

            GX2Invalidate((GX2InvalidateMode)(GX2_INVALIDATE_MODE_COLOR_BUFFER | GX2_INVALIDATE_MODE_CPU_TEXTURE), main_cbuf.surface.image, main_cbuf.surface.imageSize);

            Texture2DShader::instance()->setShaders();
            Texture2DShader::instance()->setAttributeBuffer();
            Texture2DShader::instance()->setAngle(0.0f);
            Texture2DShader::instance()->setOffset(glm::vec3(0.0f));
            Texture2DShader::instance()->setScale(glm::vec3(1.0f));
            Texture2DShader::instance()->setColorIntensity(glm::vec4(1.0f));
            Texture2DShader::instance()->setBlurring(glm::vec3(0.0f));
            Texture2DShader::instance()->setTextureAndSampler(&tex, &sampler);
            Texture2DShader::instance()->draw();

            GX2Flush();

            //GX2CopySurface(&cbuf->surface, cbuf->viewMip, cbuf->viewFirstSlice, &main_cbuf.surface, 0, 0);

            //
        } else {
            //DEBUG_FUNCTION_LINE("oop\n");
        }
        real_GX2CopyColorBufferToScanBuffer(cbuf, target);
    } else if (target == GX2_SCAN_TARGET_TV) {
        real_GX2CopyColorBufferToScanBuffer(&main_cbuf, target);
    }
}

WUPS_MUST_REPLACE(GX2CopyColorBufferToScanBuffer, WUPS_LOADER_LIBRARY_GX2, GX2CopyColorBufferToScanBuffer);
