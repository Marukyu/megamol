/*
 * AOSphereRenderer.h
 *
 * Copyright (C) 2018 by VISUS (Universitaet Stuttgart)
 * Alle Rechte vorbehalten.
 */

#pragma once

#include "geometry_calls/MultiParticleDataCall.h"
#include "mmcore/Call.h"
#include "mmcore/CallerSlot.h"
#include "mmcore/param/ParamSlot.h"
#include "mmcore_gl/view/CallRender3DGL.h"
#include "mmcore_gl/view/Renderer3DModuleGL.h"
#include "protein_calls/MolecularDataCall.h"
#include "vislib/Array.h"
#include "vislib_gl/graphics/gl/GLSLShader.h"


namespace megamol {
namespace demos_gl {

/**
 * Renderer for simple sphere glyphs
 */
class AOSphereRenderer : public megamol::core_gl::view::Renderer3DModuleGL {
public:
    /**
     * Answer the name of this module.
     *
     * @return The name of this module.
     */
    static const char* ClassName(void) {
        return "AOSphereRenderer";
    }

    /**
     * Answer a human readable description of this module.
     *
     * @return A human readable description of this module.
     */
    static const char* Description(void) {
        return "Renderer for sphere glyphs.";
    }

    /**
     * Answers whether this module is available on the current system.
     *
     * @return 'true' if the module is available, 'false' otherwise.
     */
    static bool IsAvailable(void) {
        return true;
    }

    /** Ctor. */
    AOSphereRenderer(void);

    /** Dtor. */
    virtual ~AOSphereRenderer(void);

protected:
    /**
     * Implementation of 'Create'.
     *
     * @return 'true' on success, 'false' otherwise.
     */
    virtual bool create(void);

    /**
     * The get extents callback. The module should set the members of
     * 'call' to tell the caller the extents of its data (bounding boxes
     * and times).
     *
     * @param call The calling call.
     *
     * @return The return value of the function.
     */
    virtual bool GetExtents(megamol::core_gl::view::CallRender3DGL& call);

    /**
     * Implementation of 'Release'.
     */
    virtual void release(void);

    /**
     * The render callback.
     *
     * @param call The calling call.
     *
     * @return The return value of the function.
     */
    virtual bool Render(megamol::core_gl::view::CallRender3DGL& call);

private:
    /**
     * TODO: Document
     */
    void resizeVolume();
    void uploadCameraUniforms(
        megamol::core_gl::view::CallRender3DGL& call, vislib_gl::graphics::gl::GLSLShader* sphereShader);
    void renderParticles(megamol::core_gl::view::CallRender3DGL& call, geocalls::MultiParticleDataCall* c2);
    void renderParticles(megamol::core_gl::view::CallRender3DGL& call, protein_calls::MolecularDataCall* mol);
    void renderParticlesVBO(megamol::core_gl::view::CallRender3DGL& call, geocalls::MultiParticleDataCall* c2);
    void renderParticlesVBO(megamol::core_gl::view::CallRender3DGL& call, protein_calls::MolecularDataCall* mol);


    /**
     * TODO: Document
     */
    void createEmptyVolume();

    /**
     * TODO: Document
     */
    void createFullVolume();

    /**
     * TODO: Document
     */
    void createVolumeCPU(class geocalls::MultiParticleDataCall& c2);

    /**
     * TODO: Document
     */
    void createVolumeGLSL(class geocalls::MultiParticleDataCall& c2);
    void createVolumeGLSL(protein_calls::MolecularDataCall& mol);


    /**
     * Write particle positions and radii to a VBO for rendering and processing in CUDA.
     */
    void writeParticlePositionsVBO(class geocalls::MultiParticleDataCall& c2);
    void writeParticlePositionsVBO(protein_calls::MolecularDataCall& mol);
    void createVolumeCPU(protein_calls::MolecularDataCall& mol);


    /** The sphere shader */
    vislib_gl::graphics::gl::GLSLShader sphereShaderAOMainAxes[4];

    /** The sphere shader */
    vislib_gl::graphics::gl::GLSLShader sphereShaderAONormals[4];

    /** The call for data */
    megamol::core::CallerSlot getDataSlot;

    /** The call for Transfer function */
    megamol::core::CallerSlot getTFSlot;

    /** The call for clipping plane */
    megamol::core::CallerSlot getClipPlaneSlot;

    /** A simple black-to-white transfer function texture as fallback */
    unsigned int greyTF;

    /** Turn rendering on/off */
    megamol::core::param::ParamSlot renderFlagSlot;

    /** The size of the volume in numbers of voxels */
    megamol::core::param::ParamSlot volSizeXSlot;

    /** The size of the volume in numbers of voxels */
    megamol::core::param::ParamSlot volSizeYSlot;

    /** The size of the volume in numbers of voxels */
    megamol::core::param::ParamSlot volSizeZSlot;

    /** The generation method */
    megamol::core::param::ParamSlot volGenSlot;

    /** The access method */
    megamol::core::param::ParamSlot volAccSlot;

    /** The access step length in voxels */
    megamol::core::param::ParamSlot aoStepLengthSlot;

    /** The generation factor (influence factor of a single sphere on a voxel) */
    megamol::core::param::ParamSlot aoGenFacSlot;

    /** The evaluation factor (shadowing amount factor multiplied with the ambient occlusion factors) */
    megamol::core::param::ParamSlot aoEvalFacSlot;

    /** The shading mode */
    megamol::core::param::ParamSlot aoShadModeSlot;

    /** Clip ambient occlusion data */
    megamol::core::param::ParamSlot aoClipFlagSlot;

    /** The density volume texture id */
    unsigned int volTex;

    /** FBO for GLSL volume generation */
    GLuint volFBO;

    /** The volume generation shader */
    vislib_gl::graphics::gl::GLSLShader updateVolumeShader;

    // VBO for all particles
    GLuint particleVBO;
    // number of particles
    UINT64 particleCountVBO;

    // vertex array for atom spheres (molecule rendering)
    vislib::Array<float> vertSpheres;
    // vertex array for atom colours (molecule rendering)
    vislib::Array<unsigned char> vertColours;

    vislib::Array<vislib::Array<float>> sphereSlices;
    vislib::Array<unsigned int> sphereCountSlices;

    float clipDat[4];
    float clipCol[3];
};

} // namespace demos_gl
} /* end namespace megamol */
