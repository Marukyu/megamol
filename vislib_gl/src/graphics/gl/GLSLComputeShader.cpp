/*
 * GLSLComputeShader.cpp
 *
 * Copyright (C) 2006 - 2012 by Universitaet Stuttgart (VIS).
 * Alle Rechte vorbehalten.
 */

#ifdef _WIN32
#include <Windows.h>
#endif /* _WIN32 */

#include "vislib_gl/graphics/gl/GLSLComputeShader.h"

#include "vislib/Array.h"
#include "vislib/String.h"
#include "vislib/memutils.h"
#include "vislib/sys/sysfunctions.h"
#include "vislib_gl/graphics/gl/glverify.h"


/*
 * vislib_gl::graphics::gl::GLSLComputeShader::RequiredExtensions
 */
const char* vislib_gl::graphics::gl::GLSLComputeShader::RequiredExtensions(void) {
    static vislib::StringA exts =
        vislib::StringA(vislib_gl::graphics::gl::GLSLShader::RequiredExtensions()) + " GL_ARB_compute_shader ";
    return exts.PeekBuffer();
}


/*
 * vislib_gl::graphics::gl::GLSLComputeShader::GPU4_EXTENSION_DIRECTIVE

const char *
vislib_gl::graphics::gl::GLSLComputeShader::GPU4_EXTENSION_DIRECTIVE =
    "#extension GL_EXT_gpu_shader4:enable\n";
        */

/*
 * vislib_gl::graphics::gl::GLSLComputeShader::GLSLComputeShader
 */
vislib_gl::graphics::gl::GLSLComputeShader::GLSLComputeShader(void) : GLSLShader() {}


/*
 * vislib_gl::graphics::gl::GLSLComputeShader::~GLSLComputeShader
 */
vislib_gl::graphics::gl::GLSLComputeShader::~GLSLComputeShader(void) {}


/*
 * vislib_gl::graphics::gl::GLSLComputeShader::Compile
 */
bool vislib_gl::graphics::gl::GLSLComputeShader::Compile(const char* computeShaderSrc) {
    const char* c[] = {computeShaderSrc};

    return this->Compile(c, 1);
}


/*
 * vislib_gl::graphics::gl::GLSLComputeShader::Compile
 */
bool vislib_gl::graphics::gl::GLSLComputeShader::Compile(
    const char** computeShaderSrc, const SIZE_T cntComputeShaderSrc, bool insertLineDirective) {

    USES_GL_VERIFY;
    ASSERT(computeShaderSrc != NULL);

    this->Release();

    GLhandleARB computeShader = this->compileNewShader(
        GL_COMPUTE_SHADER, computeShaderSrc, static_cast<GLsizei>(cntComputeShaderSrc), insertLineDirective);

    /* Assemble program object. */
    GL_VERIFY_THROW(this->hProgObj = ::glCreateProgram());
    GL_VERIFY_THROW(::glAttachShader(this->hProgObj, computeShader));
    GL_VERIFY_THROW(glLinkProgram(this->hProgObj));

    return true;
}


/*
 * vislib_gl::graphics::gl::GLSLComputeShader::CompileFromFile
 */
bool vislib_gl::graphics::gl::GLSLComputeShader::CompileFromFile(const char* computeShaderFile) {
    vislib::StringA computeShaderSrc;

    if (!vislib::sys::ReadTextFile(computeShaderSrc, computeShaderFile)) {
        return false;
    }

    return this->Compile(computeShaderSrc);
}


/*
 * vislib_gl::graphics::gl::GLSLComputeShader::CompileFromFile
 */
bool vislib_gl::graphics::gl::GLSLComputeShader::CompileFromFile(
    const char** computeShaderFiles, const SIZE_T cntComputeShaderFiles, bool insertLineDirective) {

    // using arrays for automatic cleanup when a 'read' throws an exception
    vislib::Array<vislib::StringA> copmuteShaderSrcs(cntComputeShaderFiles);

    for (SIZE_T i = 0; i < cntComputeShaderFiles; i++) {
        if (!vislib::sys::ReadTextFile(copmuteShaderSrcs[i], computeShaderFiles[i])) {
            return false;
        }
    }

    // built up pointer arrays for attributes
    const char** computeShaderSrcPtrs = new const char*[cntComputeShaderFiles];

    try {
        for (SIZE_T i = 0; i < cntComputeShaderFiles; i++) {
            computeShaderSrcPtrs[i] = copmuteShaderSrcs[i].PeekBuffer();
        }

        bool retval = this->Compile(computeShaderSrcPtrs, cntComputeShaderFiles, insertLineDirective);

        ARY_SAFE_DELETE(computeShaderSrcPtrs);

        return retval;

        // free pointer arrays on exception
    } catch (OpenGLException e) { // catch OpenGLException to avoid truncating
        ARY_SAFE_DELETE(computeShaderSrcPtrs);
        throw e;
    } catch (CompileException e) {
        ARY_SAFE_DELETE(computeShaderSrcPtrs);
        throw e;
    } catch (vislib::Exception e) {
        ARY_SAFE_DELETE(computeShaderSrcPtrs);
        throw e;
    } catch (...) {
        ARY_SAFE_DELETE(computeShaderSrcPtrs);
        throw vislib::Exception("Unknown Exception", __FILE__, __LINE__);
    }

    return false; // should be unreachable code!
}


void vislib_gl::graphics::gl::GLSLComputeShader::Dispatch(
    unsigned int numGroupsX, unsigned int numGroupsY, unsigned int numGroupsZ) {
    glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
}
