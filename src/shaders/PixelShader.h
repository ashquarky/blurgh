/****************************************************************************
 * Copyright (C) 2015 Dimok
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#ifndef PIXEL_SHADER_H
#define PIXEL_SHADER_H

#include <memory/mappedmemory.h>
#include "Shader.h"

class PixelShader : public Shader {
public:
    PixelShader()
        : pixelShader((GX2PixelShader*) MEMAllocFromMappedMemoryForGX2Ex(sizeof(GX2PixelShader), 0x40)) {
        if(pixelShader) {
            memset(pixelShader, 0, sizeof(GX2PixelShader));
            pixelShader->mode = GX2_SHADER_MODE_UNIFORM_REGISTER;
        }
    }
    virtual ~PixelShader() {
        if(pixelShader) {
            if(pixelShader->program)
                MEMFreeToMappedMemory(pixelShader->program);

            for(uint32_t i = 0; i < pixelShader->uniformBlockCount; i++)
                MEMFreeToMappedMemory((void*)pixelShader->uniformBlocks[i].name);

            if(pixelShader->uniformBlocks)
                MEMFreeToMappedMemory((void*)pixelShader->uniformBlocks);

            for(uint32_t i = 0; i < pixelShader->uniformVarCount; i++)
                MEMFreeToMappedMemory((void*)pixelShader->uniformVars[i].name);

            if(pixelShader->uniformVars)
                MEMFreeToMappedMemory((void*)pixelShader->uniformVars);

            if(pixelShader->initialValues)
                MEMFreeToMappedMemory((void*)pixelShader->initialValues);

            for(uint32_t i = 0; i < pixelShader->samplerVarCount; i++)
                MEMFreeToMappedMemory((void*)pixelShader->samplerVars[i].name);

            if(pixelShader->samplerVars)
                MEMFreeToMappedMemory((void*)pixelShader->samplerVars);

            if(pixelShader->loopVars)
                MEMFreeToMappedMemory((void*)pixelShader->loopVars);

            MEMFreeToMappedMemory(pixelShader);
        }
    }

    void setProgram(const uint32_t * program, const uint32_t & programSize, const uint32_t * regs, const uint32_t & regsSize) {
        if(!pixelShader)
            return;

        //! this must be moved into an area where the graphic engine has access to and must be aligned to 0x100
        pixelShader->size = programSize;
        pixelShader->program = (uint8_t*)MEMAllocFromMappedMemoryForGX2Ex(pixelShader->size, GX2_SHADER_PROGRAM_ALIGNMENT);
        if(pixelShader->program) {
            memcpy(pixelShader->program, program, pixelShader->size);
            GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, pixelShader->program, pixelShader->size);
        }

        memcpy(&pixelShader->regs, regs, regsSize);
    }

    void addUniformVar(const GX2UniformVar & var) {
        if(!pixelShader)
            return;

        uint32_t idx = pixelShader->uniformVarCount;

        GX2UniformVar* newVar = (GX2UniformVar*) MEMAllocFromMappedMemoryForGX2Ex((pixelShader->uniformVarCount + 1) * sizeof(GX2UniformVar), 0x10);
        if(newVar) {
            if(pixelShader->uniformVars) {
                memcpy(newVar, pixelShader->uniformVars, pixelShader->uniformVarCount * sizeof(GX2UniformVar));
                MEMFreeToMappedMemory(pixelShader->uniformVars);
            }
            pixelShader->uniformVars = newVar;

            memcpy(pixelShader->uniformVars + idx, &var, sizeof(GX2UniformVar));
            pixelShader->uniformVars[idx].name = (char*) MEMAllocFromMappedMemory(strlen(var.name) + 1);
            strcpy((char*)pixelShader->uniformVars[idx].name, var.name);

            pixelShader->uniformVarCount++;
        }
    }

    void addSamplerVar(const GX2SamplerVar & var) {
        if(!pixelShader)
            return;

        uint32_t idx = pixelShader->samplerVarCount;

        GX2SamplerVar* newVar = (GX2SamplerVar*) MEMAllocFromMappedMemoryForGX2Ex((pixelShader->samplerVarCount + 1) * sizeof(GX2SamplerVar), 0x10);
        if(newVar) {
            if(pixelShader->samplerVars) {
                memcpy(newVar, pixelShader->samplerVars, pixelShader->samplerVarCount * sizeof(GX2SamplerVar));
                MEMFreeToMappedMemory(pixelShader->samplerVars);
            }
            pixelShader->samplerVars = newVar;

            memcpy(pixelShader->samplerVars + idx, &var, sizeof(GX2SamplerVar));
            pixelShader->samplerVars[idx].name = (char*) MEMAllocFromMappedMemory(strlen(var.name) + 1);
            strcpy((char*)pixelShader->samplerVars[idx].name, var.name);

            pixelShader->samplerVarCount++;
        }
    }
    GX2PixelShader * getPixelShader() const {
        return pixelShader;
    }

    void setShader(void) const {
        GX2SetPixelShader(pixelShader);
    }

    static inline void setUniformReg(uint32_t location, uint32_t size, const void * reg) {
        GX2SetPixelUniformReg(location, size, (uint32_t *)reg);
    }
protected:
    GX2PixelShader *pixelShader;
};

#endif // PIXEL_SHADER_H
