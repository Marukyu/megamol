#include "stdafx.h"
#include "ParallelCoordinatesRenderer2D.h"

#include <algorithm>
#include "glm/gtc/functions.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "mmcore/CoreInstance.h"
#include "mmcore/FlagCall_GL.h"
#include "mmcore/param/BoolParam.h"
#include "mmcore/param/ButtonParam.h"
#include "mmcore/param/ColorParam.h"
#include "mmcore/param/EnumParam.h"
#include "mmcore/param/FlexEnumParam.h"
#include "mmcore/param/FloatParam.h"
#include "mmcore/param/IntParam.h"
#include "mmcore/param/StringParam.h"
#include "mmcore/utility/ColourParser.h"
#include "mmcore/view/CallGetTransferFunction.h"
#include "mmstd_datatools/table/TableDataCall.h"
#include "vislib/graphics/gl/IncludeAllGL.h"
#include "vislib/graphics/gl/ShaderSource.h"

#include <array>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "vislib/graphics/InputModifiers.h"

//#define FUCK_THE_PIPELINE
//#define USE_TESSELLATION
//#define REMOVE_TEXT

using namespace megamol;
using namespace megamol::infovis;
using namespace megamol::stdplugin::datatools;

ParallelCoordinatesRenderer2D::ParallelCoordinatesRenderer2D(void)
    : Renderer2D()
    , getDataSlot("getdata", "Float table input")
    , getTFSlot("getTF", "connects to the transfer function")
    , readFlagsSlot("readFlags", "reads the flag storage")
    , writeFlagsSlot("writeFlags", "writes the flag storage")
    , currentHash(0xFFFFFFFF)
    , currentFlagsVersion(0)
    , densityFBO()
    , drawModeSlot("drawMode", "Draw mode")
    , drawSelectedItemsSlot("drawSelectedItems", "Draw selected items")
    , selectedItemsColorSlot("selectedItemsColor", "Color for selected items")
    , drawOtherItemsSlot("drawOtherItems", "Draw other (e.g., non-selected) items")
    , otherItemsColorSlot("otherItemsColor", "Color for other items (e.g., non-selected)")
    , otherItemsAttribSlot("otherItemsAttrib", "attribute to use for TF lookup and item coloring")
    , drawAxesSlot("drawAxes", "Draw dimension axes")
    , axesColorSlot("axesColor", "Color for dimension axes")
    , filterIndicatorColorSlot("filterIndicatorCol", "Color for filter indicators")
    , selectionModeSlot("selectionMode", "Selection mode")
    , drawSelectionIndicatorSlot("drawSelectionIndicator", "Draw selection indicator")
    , selectionIndicatorColorSlot("selectionIndicatorColor", "Color for selection indicator")
    , pickRadiusSlot("pickRadius", "Picking radius in object-space")
    , scaleToFitSlot("scaleToFit", "fit the diagram in the viewport")
    , glDepthTestSlot("glEnableDepthTest", "Toggle GLDEPTHTEST")
    , glLineSmoothSlot("glEnableLineSmooth", "Toggle GLLINESMOOTH")
    , glLineWidthSlot("glLineWidth", "Value for glLineWidth")
    , sqrtDensitySlot("sqrtDensity", "map root of density to transfer function (instead of linear mapping)")
    //, resetFlagsSlot("resetFlags", "Reset item flags to initial state")
    , resetFiltersSlot("resetFilters", "Reset dimension filters to initial state")
    , filterStateSlot("filterState", "stores filter state for serialization")

    , halveRes("halve Resolution", "halve Resolution for FPS test")
    , approachSlot("Approach", "Numerical Value assigned to each temporal reconstruction approach")

    , numTicks(5)
    , columnCount(0)
    , itemCount(0)
    , dataBuffer(0)
    , minimumsBuffer(0)
    , maximumsBuffer(0)
    , axisIndirectionBuffer(0)
    , filtersBuffer(0)
    , minmaxBuffer(0)
    , interactionState(InteractionState::NONE)
    , pickedAxis(-1)
    , pickedIndicatorAxis(-1)
    , pickedIndicatorIndex(-1)
    , strokeStartX(0)
    , strokeStartY(0)
    , strokeEndX(0)
    , strokeEndY(0)
    , needSelectionUpdate(false)
    , needFlagsUpdate(false)
    , lastTimeStep(0)
    , font("Evolventa-SansSerif", core::utility::SDFFont::RenderType::RENDERTYPE_FILL) {

    this->getDataSlot.SetCompatibleCall<table::TableDataCallDescription>();
    this->MakeSlotAvailable(&this->getDataSlot);

    this->getTFSlot.SetCompatibleCall<core::view::CallGetTransferFunctionDescription>();
    this->MakeSlotAvailable(&this->getTFSlot);

    this->readFlagsSlot.SetCompatibleCall<core::FlagCallRead_GLDescription>();
    this->MakeSlotAvailable(&this->readFlagsSlot);
    this->writeFlagsSlot.SetCompatibleCall<core::FlagCallWrite_GLDescription>();
    this->MakeSlotAvailable(&this->writeFlagsSlot);

    auto drawModes = new core::param::EnumParam(DRAW_DISCRETE);
    drawModes->SetTypePair(DRAW_DISCRETE, "Discrete");
    drawModes->SetTypePair(DRAW_CONTINUOUS, "Continuous");
    drawModes->SetTypePair(DRAW_HISTOGRAM, "Histogram");
    drawModeSlot.SetParameter(drawModes);
    this->MakeSlotAvailable(&drawModeSlot);

    drawSelectedItemsSlot << new core::param::BoolParam(true);
    this->MakeSlotAvailable(&drawSelectedItemsSlot);

    selectedItemsColorSlot << new core::param::ColorParam("red");
    this->MakeSlotAvailable(&selectedItemsColorSlot);

    drawOtherItemsSlot << new core::param::BoolParam(true);
    this->MakeSlotAvailable(&drawOtherItemsSlot);

    otherItemsColorSlot << new core::param::ColorParam("gray");
    this->MakeSlotAvailable(&otherItemsColorSlot);
    otherItemsAttribSlot << new core::param::FlexEnumParam("undef");
    this->MakeSlotAvailable(&this->otherItemsAttribSlot);

    drawAxesSlot << new core::param::BoolParam(true);
    this->MakeSlotAvailable(&drawAxesSlot);

    axesColorSlot << new core::param::ColorParam("white");
    this->MakeSlotAvailable(&axesColorSlot);

    filterIndicatorColorSlot << new core::param::ColorParam("orange");
    this->MakeSlotAvailable(&filterIndicatorColorSlot);

    drawSelectionIndicatorSlot << new core::param::BoolParam(true);
    this->MakeSlotAvailable(&drawSelectionIndicatorSlot);

    selectionIndicatorColorSlot << new core::param::ColorParam("MegaMolBlue");
    this->MakeSlotAvailable(&selectionIndicatorColorSlot);

    auto pickModes = new core::param::EnumParam(SELECT_STROKE);
    pickModes->SetTypePair(SELECT_PICK, "Pick");
    pickModes->SetTypePair(SELECT_STROKE, "Stroke");
    selectionModeSlot.SetParameter(pickModes);
    this->MakeSlotAvailable(&selectionModeSlot);

    pickRadiusSlot << new core::param::FloatParam(0.1f, 0.01f, 10.0f);
    this->MakeSlotAvailable(&pickRadiusSlot);

    scaleToFitSlot << new core::param::BoolParam(false);
    scaleToFitSlot.SetUpdateCallback(this, &ParallelCoordinatesRenderer2D::scalingChangedCallback);
    this->MakeSlotAvailable(&scaleToFitSlot);

    glDepthTestSlot << new core::param::BoolParam(false);
    this->MakeSlotAvailable(&glDepthTestSlot);

    glLineSmoothSlot << new core::param::BoolParam(false);
    this->MakeSlotAvailable(&glLineSmoothSlot);

    glLineWidthSlot << new core::param::FloatParam(1.0f, 0.1f);
    this->MakeSlotAvailable(&glLineWidthSlot);

    sqrtDensitySlot << new core::param::BoolParam(true);
    this->MakeSlotAvailable(&sqrtDensitySlot);

    // resetFlagsSlot << new core::param::ButtonParam();
    // resetFlagsSlot.SetUpdateCallback(this, &ParallelCoordinatesRenderer2D::resetFlagsSlotCallback);
    // this->MakeSlotAvailable(&resetFlagsSlot);

    resetFiltersSlot << new core::param::ButtonParam();
    resetFiltersSlot.SetUpdateCallback(this, &ParallelCoordinatesRenderer2D::resetFiltersSlotCallback);
    this->MakeSlotAvailable(&resetFiltersSlot);

    filterStateSlot << new ::core::param::StringParam("");
    // filterStateSlot.Param<core::param::StringParam>()->SetGUIVisible(false);
    this->MakeSlotAvailable(&filterStateSlot);

    this->halveRes << new core::param::BoolParam(false);
    this->MakeSlotAvailable(&halveRes);

    this->approachSlot << new core::param::IntParam(0);
    this->MakeSlotAvailable(&approachSlot);

    fragmentMinMax.resize(2);
}

ParallelCoordinatesRenderer2D::~ParallelCoordinatesRenderer2D(void) { this->Release(); }

bool ParallelCoordinatesRenderer2D::enableProgramAndBind(vislib::graphics::gl::GLSLShader& program) {
    program.Enable();
    // bindbuffer?
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, dataBuffer);
    auto flags = this->readFlagsSlot.CallAs<core::FlagCallRead_GL>();
    flags->getData()->flags->bind(1);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, minimumsBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, maximumsBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, axisIndirectionBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, filtersBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, minmaxBuffer);

    glUniform2f(program.ParameterLocation("scaling"), 1.0f, 1.0f); // scaling, whatever
    glUniformMatrix4fv(program.ParameterLocation("modelView"), 1, GL_FALSE, modelViewMatrix_column);
    glUniformMatrix4fv(program.ParameterLocation("projection"), 1, GL_FALSE, projMatrix_column);
    glUniform1ui(program.ParameterLocation("dimensionCount"), this->columnCount);
    glUniform1ui(program.ParameterLocation("itemCount"), this->itemCount);

    glUniform2f(program.ParameterLocation("margin"), this->marginX, this->marginY);
    glUniform1f(program.ParameterLocation("axisDistance"), this->axisDistance);
    glUniform1f(program.ParameterLocation("axisHeight"), this->axisHeight);

    return true;
}

bool ParallelCoordinatesRenderer2D::create(void) {
    glGenBuffers(1, &dataBuffer);
    glGenBuffers(1, &minimumsBuffer);
    glGenBuffers(1, &maximumsBuffer);
    glGenBuffers(1, &axisIndirectionBuffer);
    glGenBuffers(1, &filtersBuffer);
    glGenBuffers(1, &minmaxBuffer);
    glGenBuffers(1, &counterBuffer);
    vislib::sys::Log::DefaultLog.WriteInfo("GENERATED");
    nuFB = std::make_shared<glowl::FramebufferObject>(1, 1, true);
    nuFB->createColorAttachment(GL_RGB32F, GL_RGB, GL_FLOAT);
    //  vislib::sys::Log::DefaultLog.WriteInfo(
    //      "WORKED WORKED WORKED WORKED WORKED WORKED WORKED WORKED WORKED WORKED WORKED WORKED WORKED WORKED ");

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &origFBO);
    glGenFramebuffers(1, &amortizedFboA);
    glGenFramebuffers(1, &amortizedFboB);
    glGenFramebuffers(1, &amortizedFboC);
    glGenFramebuffers(1, &amortizedFboD);
    glGenFramebuffers(1, &amortizedMsaaFboA);
    glGenFramebuffers(1, &amortizedMsaaFboB);
    glGenTextures(1, &imageStorageA);
    glGenTextures(1, &msImageStorageA);
    glGenTextures(1, &imageStorageB);
    glGenTextures(1, &msImageStorageB);
    glGenTextures(1, &imageStorageC);
    glGenTextures(1, &msImageStorageC);
    glGenTextures(1, &imageStorageD);
    glGenTextures(1, &msImageStorageD);
    glGenTextures(1, &depthStore);
    glGenTextures(1, &stenStore);
    glGenTextures(1, &depthStore2);
    glGenRenderbuffers(1, &nuDRB);
    glGenRenderbuffers(1, &nuSRB);

    glBindFramebuffer(GL_FRAMEBUFFER, amortizedMsaaFboA);
    //glBindTexture(GL_TEXTURE_2D, imStoreI);
    glEnable(GL_MULTISAMPLE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msImageStorageA);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, 0);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, GL_RGB, 1, 1, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imStoreI, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msImageStorageA, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glBindFramebuffer(GL_TEXTURE_2D, amortizedMsaaFboB);
    glEnable(GL_MULTISAMPLE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msImageStorageB);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, 0);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, GL_RGB, 1, 1, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imStoreI2, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msImageStorageB, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_FRAMEBUFFER, origFBO);

    glBindFramebuffer(GL_TEXTURE_2D, amortizedFboA);
    glBindTexture(GL_TEXTURE_2D, imageStorageA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imageStorageA, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glBindFramebuffer(GL_TEXTURE_2D, amortizedFboB);
    glBindTexture(GL_TEXTURE_2D, imageStorageB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imageStorageB, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glBindFramebuffer(GL_TEXTURE_2D, amortizedFboC);
    glBindTexture(GL_TEXTURE_2D, imageStorageC);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imageStorageC, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glBindFramebuffer(GL_TEXTURE_2D, amortizedFboD);
    glBindTexture(GL_TEXTURE_2D, imageStorageD);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imageStorageD, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);


    instance()->ShaderSourceFactory().MakeShaderSource("pc_reconstruction::vert0", vertex_shader_src);
    instance()->ShaderSourceFactory().MakeShaderSource("pc_reconstruction::frag0", fragment_shader_src);
    pc_reconstruction0_shdr = std::make_unique<vislib::graphics::gl::GLSLShader>();
    pc_reconstruction0_shdr->Compile(
        vertex_shader_src.Code(), vertex_shader_src.Count(), fragment_shader_src.Code(), fragment_shader_src.Count());
    pc_reconstruction0_shdr->Link();

    instance()->ShaderSourceFactory().MakeShaderSource("pc_reconstruction::vert1", vertex_shader_src);
    instance()->ShaderSourceFactory().MakeShaderSource("pc_reconstruction::frag1", fragment_shader_src);
    pc_reconstruction1_shdr = std::make_unique<vislib::graphics::gl::GLSLShader>();
    pc_reconstruction1_shdr->Compile(
        vertex_shader_src.Code(), vertex_shader_src.Count(), fragment_shader_src.Code(), fragment_shader_src.Count());
    pc_reconstruction1_shdr->Link();

#ifndef REMOVE_TEXT
    if (!font.Initialise(this->GetCoreInstance())) return false;
    font.SetBatchDrawMode(true);
#endif

    if (!makeProgram("::pc_axes_draw::axes", this->drawAxesProgram)) return false;
    if (!makeProgram("::pc_axes_draw::scales", this->drawScalesProgram)) return false;
    if (!makeProgram("::pc_axes_draw::filterindicators", this->drawFilterIndicatorsProgram)) return false;

    if (!makeProgram("::pc_item_stroke::indicator", this->drawStrokeIndicatorProgram)) return false;
    if (!makeProgram("::pc_item_pick::indicator", this->drawPickIndicatorProgram)) return false;

    if (!makeProgram("::pc_item_draw::discrete", this->drawItemsDiscreteProgram)) return false;
    if (!makeProgram("::pc_item_draw::discreteT", this->drawItemsTriangleProgram)) return false;
    if (!makeProgram("::pc_item_draw::muhaha", this->traceItemsDiscreteProgram)) return false;

    if (!makeProgram("::pc_item_draw::discTess", drawItemsDiscreteTessProgram)) return false;
    glGetIntegerv(GL_MAX_TESS_GEN_LEVEL, &this->maxAxes); // TODO we should reject data with more axes!
    this->isoLinesPerInvocation = maxAxes; // warning: for tesslevel n there are JUST n lines!!! not n+1 !!

    if (!makeProgram("::fragment_count", this->drawItemContinuousProgram)) return false;
    if (!makeProgram("::fragment_count", this->minMaxProgram)) return false;

    if (!makeProgram("::pc_item_draw::histogram", this->drawItemsHistogramProgram)) return false;

    if (!makeProgram("::pc_item_filter", this->filterProgram)) return false;
    if (!makeProgram("::pc_item_pick", this->pickProgram)) return false;
    if (!makeProgram("::pc_item_stroke", this->strokeProgram)) return false;

    glGetProgramiv(this->filterProgram, GL_COMPUTE_WORK_GROUP_SIZE, filterWorkgroupSize);
    glGetProgramiv(this->minMaxProgram, GL_COMPUTE_WORK_GROUP_SIZE, counterWorkgroupSize);
    glGetProgramiv(this->pickProgram, GL_COMPUTE_WORK_GROUP_SIZE, pickWorkgroupSize);
    glGetProgramiv(this->strokeProgram, GL_COMPUTE_WORK_GROUP_SIZE, strokeWorkgroupSize);

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxWorkgroupCount[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxWorkgroupCount[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxWorkgroupCount[2]);

    // this->filterStateSlot.ForceSetDirty();


    return true;
}

void ParallelCoordinatesRenderer2D::release(void) {
    glDeleteBuffers(1, &dataBuffer);
    glDeleteBuffers(1, &minimumsBuffer);
    glDeleteBuffers(1, &maximumsBuffer);
    glDeleteBuffers(1, &axisIndirectionBuffer);
    glDeleteBuffers(1, &filtersBuffer);
    glDeleteBuffers(1, &minmaxBuffer);
    glDeleteBuffers(1, &counterBuffer);

    this->drawAxesProgram.Release();
}

int ParallelCoordinatesRenderer2D::mouseXtoAxis(float x) {
    float f = (x - this->marginX) / this->axisDistance;
    float frac = f - static_cast<long>(f);
    int integral = static_cast<int>(std::round(f));
    if (integral >= static_cast<int>(this->columnCount) || integral < 0) return -1;
    if (frac > 0.7 || frac < 0.3) {
        // vislib::sys::Log::DefaultLog.WriteInfo("picking axis %i at mouse position of axis %i",
        // axisIndirection[integral], integral);
        return axisIndirection[integral];
    } else {
        return -1;
    }
}

void ParallelCoordinatesRenderer2D::pickIndicator(float x, float y, int& axis, int& index) {
    axis = mouseXtoAxis(x);
    index = -1;
    if (axis != -1) {
        // calculate position of click and filters in [0, 1] range of axis height
        float pickPos = (y - this->marginY) / this->axisHeight;
        float upperPos = (this->filters[axis].upper - minimums[axis]) / (maximums[axis] - minimums[axis]);
        float lowerPos = (this->filters[axis].lower - minimums[axis]) / (maximums[axis] - minimums[axis]);

        // Add small epsilon for better UI feeling because indicator is drawn only to one side.
        // This also handles intuitive selection if upper and lower filter are set to the same value.
        upperPos += 0.01;
        lowerPos -= 0.01;

        float distUpper = fabs(upperPos - pickPos);
        float distLower = fabs(lowerPos - pickPos);

        float thresh = 0.1f;
        if (distUpper < thresh && distUpper < distLower) {
            index = 1;
        } else if (distLower < thresh) {
            index = 0;
        }
    }
    if (index == -1) {
        axis = -1;
    }
}

bool ParallelCoordinatesRenderer2D::scalingChangedCallback(core::param::ParamSlot& caller) {
    this->computeScaling();
    return true;
}

// bool ParallelCoordinatesRenderer2D::resetFlagsSlotCallback(core::param::ParamSlot& caller) { return true; }

bool ParallelCoordinatesRenderer2D::resetFiltersSlotCallback(core::param::ParamSlot& caller) {
    for (GLuint i = 0; i < this->columnCount; i++) {
        this->filters[i].lower = minimums[i];
        this->filters[i].upper = maximums[i];
    }
    this->needFlagsUpdate = true;
    return true;
}

void ParallelCoordinatesRenderer2D::assertData(core::view::CallRender2D& call) {
    auto floats = getDataSlot.CallAs<megamol::stdplugin::datatools::table::TableDataCall>();
    if (floats == nullptr) return;
    auto tc = getTFSlot.CallAs<megamol::core::view::CallGetTransferFunction>();
    if (tc == nullptr) {
        vislib::sys::Log::DefaultLog.WriteMsg(
            vislib::sys::Log::LEVEL_ERROR, "ParallelCoordinatesRenderer2D requires a transfer function!");
        return;
    }
    auto flagsc = readFlagsSlot.CallAs<core::FlagCallRead_GL>();
    if (flagsc == nullptr) {
        vislib::sys::Log::DefaultLog.WriteMsg(
            vislib::sys::Log::LEVEL_ERROR, "ParallelCoordinatesRenderer2D requires a flag storage!");
        return;
    }

    floats->SetFrameID(static_cast<unsigned int>(call.Time()));
    (*floats)(1);
    (*floats)(0);
    call.SetTimeFramesCount(floats->GetFrameCount());
    auto hash = floats->DataHash();
    (*flagsc)(core::FlagCallRead_GL::CallGetData);
    if (flagsc->hasUpdate()) {
        this->currentFlagsVersion = flagsc->version();
        flagsc->getData()->validateFlagCount(floats->GetRowsCount());
    }

    if (hash != this->currentHash || this->lastTimeStep != static_cast<unsigned int>(call.Time()) ||
        this->otherItemsAttribSlot.IsDirty()) {
        // set minmax for TF only when frame or hash changes
        try {
            auto colcol = this->columnIndex.at(this->otherItemsAttribSlot.Param<core::param::FlexEnumParam>()->Value());
            tc->SetRange(
                {floats->GetColumnsInfos()[colcol].MinimumValue(), floats->GetColumnsInfos()[colcol].MaximumValue()});
            this->otherItemsAttribSlot.ResetDirty();
        } catch (std::out_of_range& ex) {
            vislib::sys::Log::DefaultLog.WriteError(
                "ParallelCoordinatesRenderer2D: tried to color lines by non-existing column '%s'",
                this->otherItemsAttribSlot.Param<core::param::FlexEnumParam>()->Value().c_str());
        }
    }

    if (hash != this->currentHash || this->lastTimeStep != static_cast<unsigned int>(call.Time())) {

        this->computeScaling();

        this->columnCount = static_cast<GLuint>(floats->GetColumnsCount());
        this->itemCount = static_cast<GLuint>(floats->GetRowsCount());
        this->axisIndirection.resize(columnCount);
        this->filters.resize(columnCount);
        this->minimums.resize(columnCount);
        this->maximums.resize(columnCount);
        this->names.resize(columnCount);
        this->otherItemsAttribSlot.Param<core::param::FlexEnumParam>()->ClearValues();
        this->columnIndex.clear();
        for (GLuint x = 0; x < columnCount; x++) {
            axisIndirection[x] = x;
            filters[x].dimension = 0;
            filters[x].flags = 0;
            minimums[x] = floats->GetColumnsInfos()[x].MinimumValue();
            maximums[x] = floats->GetColumnsInfos()[x].MaximumValue();
            names[x] = floats->GetColumnsInfos()[x].Name();
            filters[x].lower = minimums[x];
            filters[x].upper = maximums[x];
            this->otherItemsAttribSlot.Param<core::param::FlexEnumParam>()->AddValue(
                floats->GetColumnsInfos()[x].Name());
            this->columnIndex[floats->GetColumnsInfos()[x].Name()] = x;
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, dataBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, this->columnCount * this->itemCount * sizeof(float), floats->GetData(),
            GL_STATIC_DRAW); // TODO: huh.
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, minimumsBuffer);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER, this->columnCount * sizeof(GLfloat), this->minimums.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, maximumsBuffer);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER, this->columnCount * sizeof(GLfloat), this->maximums.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, axisIndirectionBuffer);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER, this->columnCount * sizeof(GLuint), axisIndirection.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, filtersBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, this->columnCount * sizeof(DimensionFilter), this->filters.data(),
            GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, minmaxBuffer);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER, 2 * sizeof(GLfloat), fragmentMinMax.data(), GL_DYNAMIC_READ); // TODO: huh.
        this->currentHash = hash;
        this->lastTimeStep = static_cast<unsigned int>(call.Time());
    }

    (*tc)(0);

    makeDebugLabel(GL_BUFFER, DEBUG_NAME(dataBuffer));
    makeDebugLabel(GL_BUFFER, DEBUG_NAME(minimumsBuffer));
    makeDebugLabel(GL_BUFFER, DEBUG_NAME(maximumsBuffer));
    makeDebugLabel(GL_BUFFER, DEBUG_NAME(axisIndirectionBuffer));
    makeDebugLabel(GL_BUFFER, DEBUG_NAME(filtersBuffer));
    makeDebugLabel(GL_BUFFER, DEBUG_NAME(minmaxBuffer));
}

void ParallelCoordinatesRenderer2D::computeScaling(void) {
    auto fc = getDataSlot.CallAs<megamol::stdplugin::datatools::table::TableDataCall>();
    if (fc == nullptr) return;

    this->marginX = 20.f;
    this->marginY = 20.f;
    this->axisDistance = 40.0f;
    this->fontSize = this->axisDistance / 10.0f;
    this->bounds.SetLeft(0.0f);
    this->bounds.SetRight(2.0f * marginX + this->axisDistance * (fc->GetColumnsCount() - 1));

    if (this->scaleToFitSlot.Param<core::param::BoolParam>()->Value()) {
        // scale to fit
        float requiredHeight = this->bounds.Width() / windowAspect;
        this->axisHeight = requiredHeight - 3.0f * marginY;
    } else {
        this->axisHeight = 80.0f;
    }
    this->bounds.SetBottom(0.0f);
    this->bounds.SetTop(3.0f * marginY + this->axisHeight);
}

bool ParallelCoordinatesRenderer2D::GetExtents(core::view::CallRender2D& call) {
    windowAspect = static_cast<float>(call.GetViewport().AspectRatio());

    this->assertData(call);

    call.SetBoundingBox(this->bounds);

    return true;
}

bool ParallelCoordinatesRenderer2D::OnMouseButton(
    core::view::MouseButton button, core::view::MouseButtonAction action, core::view::Modifiers mods) {

    if (this->ctrlDown) {
        // these clicks go to the view
        return false;
    }

    // TODO could values from mouse move be updated from other threads? otherwise this copy is useless
    float mousePressedX = this->mouseX;
    float mousePressedY = this->mouseY;

    if (button != core::view::MouseButton::BUTTON_LEFT) {
        // ignore everything which is not left mouse button
        return false;
    }

    // any up/down event stops interaction, but only down changes selection
    this->interactionState = InteractionState::NONE;
    if (action == core::view::MouseButtonAction::PRESS) {
        this->pickedAxis = -1;
        this->pickedIndicatorAxis = -1;
        this->pickedIndicatorIndex = -1;

        if (this->altDown) {
            this->pickedAxis = mouseXtoAxis(mousePressedX);
            if (this->pickedAxis != -1) {
                this->interactionState = InteractionState::INTERACTION_DRAG;
            }
            return true;
        }

        if (this->shiftDown) {

            auto axis = mouseXtoAxis(mousePressedX);
            if (axis != -1) {
                float base = this->marginY * 0.5f - fontSize * 0.5f;
                if ((mousePressedY > base && mousePressedY < base + fontSize) ||
                    (mousePressedY > base + this->marginY + this->axisHeight &&
                        mousePressedY < base + this->marginY + this->axisHeight + fontSize)) {
                    std::swap(this->filters[axis].lower, this->filters[axis].upper);
                    this->needFlagsUpdate = true;
                    return true;
                }
            }

            pickIndicator(mousePressedX, mousePressedY, this->pickedIndicatorAxis, this->pickedIndicatorIndex);
            if (this->pickedIndicatorAxis != -1) {
                this->interactionState = InteractionState::INTERACTION_FILTER;
            }
            return true;
        }

        this->interactionState = InteractionState::INTERACTION_SELECT;
        this->strokeStartX = mousePressedX;
        this->strokeStartY = mousePressedY;
        this->strokeEndX = mousePressedX;
        this->strokeEndY = mousePressedY;
        this->needSelectionUpdate = true;
        return true;
    }

    return true;
}

bool ParallelCoordinatesRenderer2D::OnMouseMove(double x, double y) {
    this->mouseX = x;
    this->mouseY = y;

    if (this->ctrlDown) {
        // these clicks go to the view
        return false;
    }

    if (this->interactionState == InteractionState::INTERACTION_DRAG) {
        int currAxis = mouseXtoAxis(this->mouseX);
        if (currAxis != this->pickedAxis && currAxis >= 0 && currAxis < static_cast<int>(this->columnCount)) {
            auto pickedAxisIt = std::find(this->axisIndirection.begin(), this->axisIndirection.end(), this->pickedAxis);
            int pickedIdx = std::distance(this->axisIndirection.begin(), pickedAxisIt);
            this->axisIndirection.erase(pickedAxisIt);

            auto currAxisIt = std::find(this->axisIndirection.begin(), this->axisIndirection.end(), currAxis);
            int currIdx = std::distance(this->axisIndirection.begin(), currAxisIt);
            if (pickedIdx <= currIdx) {
                currAxisIt++;
            }
            this->axisIndirection.insert(currAxisIt, this->pickedAxis);

            this->needFlagsUpdate = true;
        }

        return true;
    }

    if (this->interactionState == InteractionState::INTERACTION_FILTER) {
        float val = ((this->mouseY - this->marginY) / this->axisHeight) *
                        (maximums[this->pickedIndicatorAxis] - minimums[this->pickedIndicatorAxis]) +
                    minimums[this->pickedIndicatorAxis];
        if (this->pickedIndicatorIndex == 0) {
            val = std::clamp(val, minimums[this->pickedIndicatorAxis], maximums[this->pickedIndicatorAxis]);
            this->filters[this->pickedIndicatorAxis].lower = val;
        } else {
            val = std::clamp(val, minimums[this->pickedIndicatorAxis], maximums[this->pickedIndicatorAxis]);
            this->filters[this->pickedIndicatorAxis].upper = val;
        }
        this->needFlagsUpdate = true;

        return true;
    }

    if (this->interactionState == InteractionState::INTERACTION_SELECT) {
        if (this->mouseX != this->strokeEndX || this->mouseY != this->strokeEndY) {
            this->strokeEndX = this->mouseX;
            this->strokeEndY = this->mouseY;
            this->needSelectionUpdate = true;
        }

        return true;
    }

    return false;
}

bool ParallelCoordinatesRenderer2D::OnKey(
    core::view::Key key, core::view::KeyAction action, core::view::Modifiers mods) {
    ctrlDown = mods.test(core::view::Modifier::CTRL);
    altDown = mods.test(core::view::Modifier::ALT);
    shiftDown = mods.test(core::view::Modifier::SHIFT);
    return false;
}

void ParallelCoordinatesRenderer2D::drawAxes(void) {
    debugPush(1, "drawAxes");
    if (this->columnCount > 0) {
        this->enableProgramAndBind(this->drawAxesProgram);
        glUniform4fv(this->drawAxesProgram.ParameterLocation("color"), 1,
            this->axesColorSlot.Param<core::param::ColorParam>()->Value().data());
        glUniform1i(this->drawAxesProgram.ParameterLocation("pickedAxis"), pickedAxis);
        glDrawArraysInstanced(GL_LINES, 0, 2, this->columnCount);
        this->drawAxesProgram.Disable();

        this->enableProgramAndBind(this->drawScalesProgram);
        glUniform4fv(this->drawScalesProgram.ParameterLocation("color"), 1,
            this->axesColorSlot.Param<core::param::ColorParam>()->Value().data());
        glUniform1ui(this->drawScalesProgram.ParameterLocation("numTicks"), this->numTicks);
        glUniform1f(this->drawScalesProgram.ParameterLocation("axisHalfTick"), 2.0f);
        glUniform1i(this->drawScalesProgram.ParameterLocation("pickedAxis"), pickedAxis);
        glDrawArraysInstanced(GL_LINES, 0, 2, this->columnCount * this->numTicks);
        this->drawScalesProgram.Disable();

        this->enableProgramAndBind(this->drawFilterIndicatorsProgram);
        glUniform4fv(this->drawFilterIndicatorsProgram.ParameterLocation("color"), 1,
            this->filterIndicatorColorSlot.Param<core::param::ColorParam>()->Value().data());
        glUniform1f(this->drawFilterIndicatorsProgram.ParameterLocation("axisHalfTick"), 2.0f);
        glUniform2i(this->drawFilterIndicatorsProgram.ParameterLocation("pickedIndicator"), pickedIndicatorAxis,
            pickedIndicatorIndex);
        glDrawArraysInstanced(GL_LINE_STRIP, 0, 3, this->columnCount * 2);
        this->drawScalesProgram.Disable();
        float red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
        const float* color;
#ifndef REMOVE_TEXT
        glActiveTexture(GL_TEXTURE0);
        font.ClearBatchDrawCache();
        for (unsigned int c = 0; c < this->columnCount; c++) {
            unsigned int realCol = this->axisIndirection[c];
            if (this->pickedAxis == realCol) {
                color = red;
            } else {
                color = this->axesColorSlot.Param<core::param::ColorParam>()->Value().data();
            }
            float x = this->marginX + this->axisDistance * c;
#    if 0
            this->font.DrawString(color, x, this->marginY * 0.5f                   , fontSize, false, std::to_string(minimums[realCol]).c_str(), vislib::graphics::AbstractFont::ALIGN_CENTER_MIDDLE);
            this->font.DrawString(color, x, this->marginY * 1.5f + this->axisHeight, fontSize, false, std::to_string(maximums[realCol]).c_str(), vislib::graphics::AbstractFont::ALIGN_CENTER_MIDDLE);
#    else
            float bottom = filters[realCol].lower;
            // bottom *= (maximums[realCol] - minimums[realCol]);
            // bottom += minimums[realCol];
            float top = filters[realCol].upper;
            // top *= (maximums[realCol] - minimums[realCol]);
            // top += minimums[realCol];
            this->font.DrawString(color, x, this->marginY * 0.5f, fontSize, false, std::to_string(bottom).c_str(),
                core::utility::AbstractFont::ALIGN_CENTER_MIDDLE);
            this->font.DrawString(color, x, this->marginY * 1.5f + this->axisHeight, fontSize, false,
                std::to_string(top).c_str(), core::utility::AbstractFont::ALIGN_CENTER_MIDDLE);
#    endif
            this->font.DrawString(color, x,
                this->marginY * (2.0f + static_cast<float>(c % 2) * 0.5f) + this->axisHeight, fontSize * 2.0f, false,
                names[realCol].c_str(), core::utility::AbstractFont::ALIGN_CENTER_MIDDLE);
        }
        this->font.BatchDrawString();
#endif
    }
    debugPop();
}

void ParallelCoordinatesRenderer2D::drawDiscrete(
    const float otherColor[4], const float selectedColor[4], float tfColorFactor) {
    if (this->drawOtherItemsSlot.Param<core::param::BoolParam>()->Value()) {
        this->drawItemsDiscrete(core::FlagStorage::ENABLED | core::FlagStorage::SELECTED | core::FlagStorage::FILTERED,
            core::FlagStorage::ENABLED, otherColor, tfColorFactor);
    }
    if (this->drawSelectedItemsSlot.Param<core::param::BoolParam>()->Value()) {
        this->drawItemsDiscrete(core::FlagStorage::ENABLED | core::FlagStorage::SELECTED | core::FlagStorage::FILTERED,
            core::FlagStorage::ENABLED | core::FlagStorage::SELECTED, selectedColor, tfColorFactor);
    }
}

void ParallelCoordinatesRenderer2D::drawItemsDiscrete(
    uint32_t testMask, uint32_t passMask, const float color[4], float tfColorFactor) {
    auto tf = this->getTFSlot.CallAs<megamol::core::view::CallGetTransferFunction>();
    if (tf == nullptr) return;

    debugPush(2, "drawItemsDiscrete");

#ifdef FUCK_THE_PIPELINE
    vislib::graphics::gl::GLSLShader& prog = this->traceItemsDiscreteProgram;
#else
#    ifdef USE_TESSELLATION
    vislib::graphics::gl::GLSLShader& prog = this->drawItemsDiscreteTessProgram;
#    else
    //vislib::graphics::gl::GLSLShader& prog = this->drawItemsDiscreteProgram;
    vislib::graphics::gl::GLSLShader& prog = this->drawItemsTriangleProgram;
#    endif
#endif

    this->enableProgramAndBind(prog);
    // vislib::sys::Log::DefaultLog.WriteInfo("setting tf range to [%f, %f]", tf->Range()[0], tf->Range()[1]);
    tf->BindConvenience(prog, GL_TEXTURE5, 5);
    glUniform4fv(prog.ParameterLocation("color"), 1, color);
    glUniform1f(prog.ParameterLocation("tfColorFactor"), tfColorFactor);
    glUniform1f(prog.ParameterLocation("widthR"),  res[0]);
    glUniform1f(prog.ParameterLocation("heightR"),  res[1]);
    try {
        auto colcol = this->columnIndex.at(this->otherItemsAttribSlot.Param<core::param::FlexEnumParam>()->Value());
        glUniform1i(prog.ParameterLocation("colorColumn"), colcol);
    } catch (std::out_of_range& ex) {
        // vislib::sys::Log::DefaultLog.WriteError(
        //    "ParallelCoordinatesRenderer2D: tried to color lines by non-existing column '%s'",
        //    this->otherItemsAttribSlot.Param<core::param::FlexEnumParam>()->Value().c_str());
        glUniform1i(prog.ParameterLocation("colorColumn"), -1);
    }
    glUniform1ui(prog.ParameterLocation("fragmentTestMask"), testMask);
    glUniform1ui(prog.ParameterLocation("fragmentPassMask"), passMask);

    glEnable(GL_CLIP_DISTANCE0);
#ifdef FUCK_THE_PIPELINE
    glDrawArrays(GL_TRIANGLES, 0, 6 * ((this->itemCount / 128) + 1));
#else
#    ifdef USE_TESSELLATION
    glUniform1i(prog.ParameterLocation("isoLinesPerInvocation"), isoLinesPerInvocation);
    glPatchParameteri(GL_PATCH_VERTICES, 1);
    glDrawArrays(GL_PATCHES, 0, (this->itemCount / isoLinesPerInvocation) + 1);
#    else
    // glDrawArraysInstanced(GL_LINE_STRIP, 0, this->columnCount, this->itemCount);
    // glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, this->columnCount * 2, this->itemCount);
     //glDrawArrays(GL_LINES, 0, (this->columnCount - 1) * 2 * this->itemCount);
     glDrawArrays(GL_TRIANGLES, 0, (this->columnCount - 1) * 6 * this->itemCount);
#    endif
#endif
    prog.Disable();
    glDisable(GL_CLIP_DISTANCE0);
    debugPop();
}

void ParallelCoordinatesRenderer2D::drawPickIndicator(float x, float y, float pickRadius, const float color[4]) {
    auto& program = this->drawPickIndicatorProgram;

    this->enableProgramAndBind(program);

    glUniform2f(program.ParameterLocation("mouse"), x, y);
    glUniform1f(program.ParameterLocation("pickRadius"), pickRadius);

    glUniform4fv(program.ParameterLocation("indicatorColor"), 1, color);
    // glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // glEnable(GL_DEPTH_TEST);
    program.Disable();
}

void ParallelCoordinatesRenderer2D::drawStrokeIndicator(float x0, float y0, float x1, float y1, const float color[4]) {
    auto& prog = this->drawStrokeIndicatorProgram;

    this->enableProgramAndBind(prog);

    glUniform2f(prog.ParameterLocation("mousePressed"), x0, y0);
    glUniform2f(prog.ParameterLocation("mouseReleased"), x1, y1);

    glUniform4fv(prog.ParameterLocation("indicatorColor"), 1, color);
    // glDisable(GL_DEPTH_TEST);
    //glDrawArrays(GL_LINES, 0, 2);
     glEnable(GL_DEPTH_TEST);
    prog.Disable();
}

void ParallelCoordinatesRenderer2D::doPicking(float x, float y, float pickRadius) {
    debugPush(3, "doPicking");

    this->enableProgramAndBind(pickProgram);

    glUniform2f(pickProgram.ParameterLocation("mouse"), x, y);
    glUniform1f(pickProgram.ParameterLocation("pickRadius"), pickRadius);

    GLuint groupCounts[3];
    computeDispatchSizes(itemCount, pickWorkgroupSize, maxWorkgroupCount, groupCounts);

    pickProgram.Dispatch(groupCounts[0], groupCounts[1], groupCounts[2]);
    ::glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    pickProgram.Disable();
    debugPop();
}

void ParallelCoordinatesRenderer2D::doStroking(float x0, float y0, float x1, float y1) {
    debugPush(3, "doStroking");

    this->enableProgramAndBind(strokeProgram);

    glUniform2f(strokeProgram.ParameterLocation("mousePressed"), x0, y0);
    glUniform2f(strokeProgram.ParameterLocation("mouseReleased"), x1, y1);

    GLuint groupCounts[3];
    computeDispatchSizes(itemCount, strokeWorkgroupSize, maxWorkgroupCount, groupCounts);

    strokeProgram.Dispatch(groupCounts[0], groupCounts[1], groupCounts[2]);
    ::glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    strokeProgram.Disable();
    debugPop();
}

void ParallelCoordinatesRenderer2D::doFragmentCount(void) {
    debugPush(4, "doFragmentCount");
    int invocations[] = {static_cast<int>(std::ceil(windowWidth / 16)), static_cast<int>(std::ceil(windowHeight / 16))};
    GLuint invocationCount = invocations[0] * invocations[1];

    size_t bytes = sizeof(uint32_t) * 2 * invocationCount;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, counterBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bytes, nullptr, GL_STATIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, counterBuffer);

    makeDebugLabel(GL_BUFFER, DEBUG_NAME(counterBuffer));

    glActiveTexture(GL_TEXTURE1);
    densityFBO.BindColourTexture();

    GLuint groupCounts[3] = {
        static_cast<GLuint>((std::max)(1.0f, std::ceil(float(invocations[0]) / counterWorkgroupSize[0]))),
        static_cast<GLuint>((std::max)(1.0f, std::ceil(float(invocations[1]) / counterWorkgroupSize[1]))), 1};

    this->enableProgramAndBind(minMaxProgram);

    // uniforms invocationcount etc.
    glUniform1ui(minMaxProgram.ParameterLocation("invocationCount"), invocationCount);
    glUniform4fv(minMaxProgram.ParameterLocation("clearColor"), 1, backgroundColor);
    glUniform2ui(minMaxProgram.ParameterLocation("resolution"), windowWidth, windowHeight);
    glUniform2ui(minMaxProgram.ParameterLocation("fragmentCountStepSize"), invocations[0], invocations[1]);


    minMaxProgram.Dispatch(groupCounts[0], groupCounts[1], groupCounts[2]);

    minMaxProgram.Disable();

    // todo read back minmax and check for plausibility!
    debugPop();
}

void ParallelCoordinatesRenderer2D::drawItemsContinuous(void) {
    auto tf = this->getTFSlot.CallAs<megamol::core::view::CallGetTransferFunction>();
    if (tf == nullptr) return;
    debugPush(6, "drawItemsContinuous");
    doFragmentCount();
    this->enableProgramAndBind(drawItemContinuousProgram);
    // glUniform2f(drawItemContinuousProgram.ParameterLocation("bottomLeft"), 0.0f, 0.0f);
    // glUniform2f(drawItemContinuousProgram.ParameterLocation("topRight"), windowWidth, windowHeight);
    densityFBO.BindColourTexture();
    // vislib::sys::Log::DefaultLog.WriteInfo("setting tf range to [%f, %f]", tf->Range()[0], tf->Range()[1]);
    tf->BindConvenience(drawItemContinuousProgram, GL_TEXTURE5, 5);
    glUniform1i(this->drawItemContinuousProgram.ParameterLocation("fragmentCount"), 1);
    glUniform4fv(this->drawItemContinuousProgram.ParameterLocation("clearColor"), 1, backgroundColor);
    glUniform1i(this->drawItemContinuousProgram.ParameterLocation("sqrtDensity"),
        this->sqrtDensitySlot.Param<core::param::BoolParam>()->Value() ? 1 : 0);
    glEnable(GL_CLIP_DISTANCE0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    drawItemContinuousProgram.Disable();
    glDisable(GL_CLIP_DISTANCE0);
    debugPop();
}

void ParallelCoordinatesRenderer2D::drawItemsHistogram(void) {
    debugPush(7, "drawItemsHistogram");
    doFragmentCount();
    this->enableProgramAndBind(drawItemsHistogramProgram);
    glActiveTexture(GL_TEXTURE1);
    densityFBO.BindColourTexture();
    glUniform4fv(this->drawItemContinuousProgram.ParameterLocation("clearColor"), 1, backgroundColor);
    glEnable(GL_CLIP_DISTANCE0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    drawItemContinuousProgram.Disable();
    glDisable(GL_CLIP_DISTANCE0);
    debugPop();
}

void ParallelCoordinatesRenderer2D::drawParcos(void) {

    // TODO only when filters changed!
    GLuint groups = this->itemCount / (filterWorkgroupSize[0] * filterWorkgroupSize[1] * filterWorkgroupSize[2]);
    GLuint groupCounts[3] = {(groups % maxWorkgroupCount[0]) + 1, (groups / maxWorkgroupCount[0]) + 1, 1};
    this->enableProgramAndBind(this->filterProgram);
    filterProgram.Dispatch(groupCounts[0], groupCounts[1], groupCounts[2]);
    ::glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    const float red[] = {1.0f, 0.0f, 0.0f, 1.0};
    const float moreRed[] = {10.0f, 0.0f, 0.0f, 1.0};

    auto drawmode = this->drawModeSlot.Param<core::param::EnumParam>()->Value();

    switch (drawmode) {
    case DRAW_DISCRETE:
        this->drawDiscrete(this->otherItemsColorSlot.Param<core::param::ColorParam>()->Value().data(),
            this->selectedItemsColorSlot.Param<core::param::ColorParam>()->Value().data(), 1.0f);
        break;
    case DRAW_CONTINUOUS:
    case DRAW_HISTOGRAM:
        bool ok = true;
        if (!this->densityFBO.IsValid() || this->densityFBO.GetWidth() != windowWidth ||
            this->densityFBO.GetHeight() != windowHeight) {
            densityFBO.Release();
            ok = densityFBO.Create(windowWidth, windowHeight, GL_R32F, GL_RED, GL_FLOAT);
            makeDebugLabel(GL_TEXTURE, densityFBO.GetColourTextureID(), "densityFBO");
        }
        if (ok) {
            densityFBO.Enable();
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            //::glDisable(GL_ALPHA_TEST);
            // glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);
            this->drawDiscrete(red, moreRed, 0.0f);
            densityFBO.Disable();
            glDisable(GL_BLEND);

            if (drawmode == DRAW_CONTINUOUS) {
                this->drawItemsContinuous();
            } else if (drawmode == DRAW_HISTOGRAM) {
                this->drawItemsHistogram();
            }

        } else {
            vislib::sys::Log::DefaultLog.WriteError("could not create FBO");
        }
        break;
    }
}

void ParallelCoordinatesRenderer2D::store_filters() {
    nlohmann::json jf, jf_array;
    for (auto& f : this->filters) {
        DimensionFilter::to_json(jf, f);
        jf_array.push_back(jf);
    }
    auto js = jf_array.dump();
    this->filterStateSlot.Param<core::param::StringParam>()->SetValue(js.c_str());
}

void ParallelCoordinatesRenderer2D::load_filters() {
    try {
        auto j = nlohmann::json::parse(this->filterStateSlot.Param<core::param::StringParam>()->Value().PeekBuffer());
        int i = 0;
        for (auto& f : j) {
            if (i < this->filters.size()) {
                DimensionFilter::from_json(f, this->filters[i]);
            } else {
                break;
            }
            i++;
        }
    } catch (nlohmann::json::exception e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "ParallelCoordinatesRenderer2D: could not parse serialized filters (exception %i)!", e.id);
        return;
    }
}


bool ParallelCoordinatesRenderer2D::Render(core::view::CallRender2D& call) {
    int w = call.GetViewport().Width() ;
    int h = call.GetViewport().Height();
    res[0] = 1.0*w;
    res[1] = 1.0*h;

    windowWidth = call.GetViewport().Width();
    windowHeight = call.GetViewport().Height();
    auto bg = call.GetBackgroundColour();
    backgroundColor[0] = bg[0] / 255.0f;
    backgroundColor[1] = bg[1] / 255.0f;
    backgroundColor[2] = bg[2] / 255.0f;
    backgroundColor[3] = bg[3] / 255.0f;

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &origFBO);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &origFBOr);
    // this is the apex of suck and must die
    glGetFloatv(GL_MODELVIEW_MATRIX, modelViewMatrix_column);
    glGetFloatv(GL_PROJECTION_MATRIX, projMatrix_column);
    // end suck

    int approach = this->approachSlot.Param<core::param::IntParam>()->Value();
    int framesNeeded = 1;

    // Intel MSAA 2 frame restoration
    if (approach == 0 && this->halveRes.Param<core::param::BoolParam>()->Value()) {
        framesNeeded = 2;
        w = w / 2;
        h = h / 2;
        glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], backgroundColor[3]);

        glm::mat4 pm;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                pm[j][i] = projMatrix_column[i + j * 4];
            }
        }
        if (frametype == 0) {
            auto jit = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0.0, 0));
            pm = jit * pm;
        }
        if (frametype == 1) {
            auto jit = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0 / call.GetViewport().Width(), 0, 0));
            pm = jit * pm;
        }

        for (int i = 0; i < 16; i++) projMatrix_column[i] = glm::value_ptr(pm)[i];

        if (frametype == 0) {
            glBindFramebuffer(GL_FRAMEBUFFER, amortizedMsaaFboA);
            glEnable(GL_MULTISAMPLE);

            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msImageStorageA);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, GL_RGBA8, w, h, GL_TRUE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msImageStorageA, 0);

            //glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imStoreI, 0);
            glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], backgroundColor[3]);
        }
        if (frametype == 1) {
            glBindFramebuffer(GL_FRAMEBUFFER, amortizedMsaaFboB);
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msImageStorageB);
            //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, 0);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, GL_RGB8, w, h, GL_TRUE);
            //glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imStoreI2, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msImageStorageB, 0);
            glClearColor(0.2, 0, 0, 1);
        }
        glClear(GL_COLOR_BUFFER_BIT);


        glViewport(0, 0, w, h);
    }

    // non cbr quarter res 4 frame restoration
    if (approach == 1 && this->halveRes.Param<core::param::BoolParam>()->Value()) {
        framesNeeded = 4;
        w = w / 2;
        h = h / 2;
        glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], backgroundColor[3]);

        glm::mat4 pm;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                pm[j][i] = projMatrix_column[i + j * 4];
            }
        }

        auto jit = glm::mat4(1.0);
        if (frametype == 0) {
            jit = glm::translate(
                glm::mat4(1.0f), glm::vec3(-1.0 / call.GetViewport().Width(), 1.0 / call.GetViewport().Height(), 0));
        }
        if (frametype == 1) {
            jit = glm::translate(
                glm::mat4(1.0f), glm::vec3(1.0 / call.GetViewport().Width(), 1.0 / call.GetViewport().Height(), 0));
        }
        if (frametype == 2) {
            jit = glm::translate(
                glm::mat4(1.0f), glm::vec3(-1.0 / call.GetViewport().Width(), -1.0 / call.GetViewport().Height(), 0));
        }
        if (frametype == 3) {
            jit = glm::translate(
                glm::mat4(1.0f), glm::vec3(1.0 / call.GetViewport().Width(), -1.0 / call.GetViewport().Height(), 0));
        }
        pm = jit * pm;
        //upload jitter back to proj Mat
        for (int i = 0; i < 16; i++) projMatrix_column[i] = glm::value_ptr(pm)[i];

        if (frametype == 0) {
            glBindFramebuffer(GL_FRAMEBUFFER, amortizedFboA);
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_2D, imageStorageA);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imageStorageA, 0);
            // glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imStoreI, 0);
            glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], backgroundColor[3]);
        }
        if (frametype == 1) {
            glBindFramebuffer(GL_FRAMEBUFFER, amortizedFboB);
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_2D, imageStorageB);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imageStorageB, 0);
            glClearColor(0.2, 0, 0, 1);
        }
        if (frametype == 2) {
            glBindFramebuffer(GL_FRAMEBUFFER, amortizedFboC);
            glActiveTexture(GL_TEXTURE12);
            glBindTexture(GL_TEXTURE_2D, imageStorageC);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imageStorageC, 0);
            glClearColor(0, 0.2, 0, 1);
        }if (frametype == 3) {
            glBindFramebuffer(GL_FRAMEBUFFER, amortizedFboD);
            glActiveTexture(GL_TEXTURE13);
            glBindTexture(GL_TEXTURE_2D, imageStorageD);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imageStorageD, 0);
            glClearColor(0.15, 0.15, 0, 1);
        }
        glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0, 0, w, h);
    }

    if (false && this->halveRes.Param<core::param::BoolParam>()->Value()) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (tex.size() != w * h) {
            //glViewport(0, 0, call.GetViewport().Width(), call.GetViewport().Height());

            tex.clear();
            tex.resize(w * h);
            tex2.clear();
            tex2.resize(w * h);

            bool test = true;
            for (int i = 0; i < call.GetViewport().Height(); i++) {
                for (int j = 0; j < call.GetViewport().Width(); j++) {
                    test = !test;
                    if (test) {
                        // vislib::sys::Log::DefaultLog.WriteInfo("N");
                        tex[i * w + j] = 1.0;
                        tex2[i * w + j] = 0u;
                    } else {
                        // vislib::sys::Log::DefaultLog.WriteInfo("J");
                        tex[i * w + j] = 0.0;
                        tex2[i * w + j] = 1u;
                    }
                }
            }
            glBindFramebuffer(GL_FRAMEBUFFER, amortizedFboA);
            if (glGetError() == GL_INVALID_OPERATION) vislib::sys::Log::DefaultLog.WriteInfo("THERE WAS AN ERROR1");

            glActiveTexture(GL_TEXTURE10);
            if (glGetError() == GL_INVALID_OPERATION) vislib::sys::Log::DefaultLog.WriteInfo("THERE WAS AN ERROR1");

            glBindTexture(GL_TEXTURE_2D, imageStorageA);
            if (glGetError() == GL_INVALID_OPERATION) vislib::sys::Log::DefaultLog.WriteInfo("THERE WAS AN ERROR1");
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            if (glGetError() == GL_INVALID_OPERATION) vislib::sys::Log::DefaultLog.WriteInfo("THERE WAS AN ERROR1");

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, 0);
            if (glGetError() == GL_INVALID_OPERATION) vislib::sys::Log::DefaultLog.WriteInfo("THERE WAS AN ERROR1");
            
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imageStorageA, 0);
            if (glGetError() == GL_INVALID_OPERATION) vislib::sys::Log::DefaultLog.WriteInfo("THERE WAS AN ERROR1");
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], backgroundColor[3]);
            glClear(GL_COLOR_BUFFER_BIT);

            glBindTexture(GL_TEXTURE_2D, depthStore);
            if (glGetError() == GL_INVALID_OPERATION) vislib::sys::Log::DefaultLog.WriteInfo("THERE WAS AN ERROR1");

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            if (glGetError() == GL_INVALID_OPERATION) vislib::sys::Log::DefaultLog.WriteInfo("THERE WAS AN ERROR1");

            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, &tex[0]);
            if (glGetError() != GL_NO_ERROR) vislib::sys::Log::DefaultLog.WriteInfo("THERE WAS AN ERROR1");

            //glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, w, h, 0, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &tex2[0]);
            if (glGetError() == GL_INVALID_OPERATION) vislib::sys::Log::DefaultLog.WriteInfo("THERE WAS AN ERROR2");

            glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
            if (glGetError() == GL_INVALID_OPERATION) vislib::sys::Log::DefaultLog.WriteInfo("THERE WAS AN ERROR3");
            
            glBindFramebuffer(GL_FRAMEBUFFER, amortizedFboA);
            if (glGetError() == GL_INVALID_OPERATION) vislib::sys::Log::DefaultLog.WriteInfo("THERE WAS AN ERROR6");            
            



            glDrawPixels(w, h, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &tex2[0]);
            if (glGetError() == GL_INVALID_ENUM)
                vislib::sys::Log::DefaultLog.WriteInfo("THERE WAS AN ERROR3");

            vislib::sys::Log::DefaultLog.WriteInfo("size of Array tex2: %i", tex2.size());
            vislib::sys::Log::DefaultLog.WriteInfo("w: %i", w);
            vislib::sys::Log::DefaultLog.WriteInfo("h: %i", h);
            vislib::sys::Log::DefaultLog.WriteInfo("dsID: %i", depthStore);
            vislib::sys::Log::DefaultLog.WriteInfo("tex2.0: %i", tex2[0]);
            vislib::sys::Log::DefaultLog.WriteInfo("tex2.1; %i", tex2[1]);
        }
        //glDisable(GL_STENCIL_TEST);

        if (call.frametype == 1 || call.frametype == 0) {
            glDepthMask(GL_FALSE);
            //glClear(GL_STENCIL_BUFFER_BIT);
            
            glBindFramebuffer(GL_FRAMEBUFFER, amortizedFboA);
            
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_2D, imageStorageA);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imageStorageA, 0);
            glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], backgroundColor[3]);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindTexture(GL_TEXTURE_2D, 0);

            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xFF);
            glStencilFunc(GL_EQUAL, 1, 0xFF);

            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depthStore, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            //glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, w, h, 0, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &tex2[0]);
            glBindTexture(GL_TEXTURE_2D, imageStorageA);


        }
        if (call.frametype == 2) {
            glBindFramebuffer(GL_FRAMEBUFFER, amortizedFboB);
            glBindTexture(GL_TEXTURE_2D, imageStorageB);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imageStorageB, 0);
            glClearColor(0.2, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        
        //^^COLOR ATTACHMENT
        
        glDisable(GL_DEPTH_TEST);

        //glViewport(0, 0, w, h);
    }
    windowAspect = static_cast<float>(call.GetViewport().AspectRatio());


    this->assertData(call);

    auto fc = getDataSlot.CallAs<megamol::stdplugin::datatools::table::TableDataCall>();
    if (fc == nullptr) return false;
    auto tc = getTFSlot.CallAs<megamol::core::view::CallGetTransferFunction>();
    if (tc == nullptr) {
        vislib::sys::Log::DefaultLog.WriteWarn(
            "%s cannot draw without a transfer function!", ParallelCoordinatesRenderer2D::ClassName());
        return false;
    }

    // glDisable(GL_DEPTH_TEST);
    // glDepthMask(GL_FALSE);

    if (this->filterStateSlot.IsDirty()) {
        load_filters();
        this->filterStateSlot.ResetDirty();
        this->needFlagsUpdate = true;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, axisIndirectionBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, this->columnCount * sizeof(GLuint), axisIndirection.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, filtersBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, this->columnCount * sizeof(DimensionFilter), this->filters.data());

    // Do stroking/picking
    if (this->needSelectionUpdate) {
        this->needSelectionUpdate = false;

        switch (selectionModeSlot.Param<core::param::EnumParam>()->Value()) {
        case SELECT_STROKE:
            this->doStroking(this->strokeStartX, this->strokeStartY, this->strokeEndX, this->strokeEndY);
            break;
        case SELECT_PICK:
            this->doPicking(this->strokeEndX, this->strokeEndY,
                this->pickRadiusSlot.Param<megamol::core::param::FloatParam>()->Value());
            break;
        }

        this->needFlagsUpdate = true;
    }
    glDepthMask(GL_TRUE);
    drawParcos();
    glDepthMask(GL_FALSE);

    // Draw stroking/picking indicator
    if (this->drawSelectionIndicatorSlot.Param<core::param::BoolParam>()->Value()) {
        switch (selectionModeSlot.Param<core::param::EnumParam>()->Value()) {
        case SELECT_STROKE:
            if (this->interactionState == InteractionState::INTERACTION_SELECT) {
                this->drawStrokeIndicator(this->strokeStartX, this->strokeStartY, this->strokeEndX, this->strokeEndY,
                    this->selectionIndicatorColorSlot.Param<core::param::ColorParam>()->Value().data());
            }
            break;
        case SELECT_PICK:
            this->drawPickIndicator(this->mouseX, this->mouseY,
                this->pickRadiusSlot.Param<megamol::core::param::FloatParam>()->Value(),
                this->selectionIndicatorColorSlot.Param<core::param::ColorParam>()->Value().data());
            break;
        }
    }

    if (needFlagsUpdate) {
        needFlagsUpdate = false;
        this->store_filters();

        this->currentFlagsVersion++;
        // HAZARD: downloading everything over and over is slowish
        auto readFlags = readFlagsSlot.CallAs<core::FlagCallRead_GL>();
        auto writeFlags = writeFlagsSlot.CallAs<core::FlagCallWrite_GL>();
        if (readFlags != nullptr && writeFlags != nullptr) {
            writeFlags->setData(readFlags->getData(), this->currentFlagsVersion);
            (*writeFlags)(core::FlagCallWrite_GL::CallGetData);
#if 0
            auto flags = readFlags->getData()->flags;
            std::vector<core::FlagStorage::FlagItemType> f(flags->getByteSize()/sizeof(core::FlagStorage::FlagItemType));
            flags->bind();
            glGetBufferSubData(
                GL_SHADER_STORAGE_BUFFER, 0, flags->getByteSize(), f.data());

            core::FlagStorage::FlagVectorType::size_type numFiltered = 0, numEnabled = 0, numSelected = 0,
                                                         numSoftSelected = 0;
            for (unsigned int& i : f) {
                if ((i & core::FlagStorage::FILTERED) > 0) numFiltered++;
                if ((i & core::FlagStorage::ENABLED) > 0) numEnabled++;
                if ((i & core::FlagStorage::SELECTED) > 0) numSelected++;
                if ((i & core::FlagStorage::SOFTSELECTED) > 0) numSoftSelected++;
            }
            vislib::sys::Log::DefaultLog.WriteInfo(
                "ParallelCoordinateRenderer2D: %lu items: %lu enabled, %lu filtered, %lu selected, %lu "
                "soft-selected.",
                f.size(), numEnabled, numFiltered, numSelected, numSoftSelected);
#endif
        }
    }


    if (this->drawAxesSlot.Param<core::param::BoolParam>()->Value()) {
        drawAxes();
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    if (approach == 0 && this->halveRes.Param<core::param::BoolParam>()->Value()) {
        glViewport(0, 0, call.GetViewport().Width(), call.GetViewport().Height());

        pc_reconstruction0_shdr->Enable();

        if (frametype == 0) {
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_2D, msImageStorageA);  
            glUniform1i(pc_reconstruction0_shdr->ParameterLocation("src_tx2Da"), 10);
        }
        if (frametype == 1) {
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_2D, msImageStorageB);
            glUniform1i(pc_reconstruction0_shdr->ParameterLocation("src_tx2Db"), 11);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, origFBO);
        glUniform1i(pc_reconstruction0_shdr->ParameterLocation("h"), call.GetViewport().Height());
        glUniform1i(pc_reconstruction0_shdr->ParameterLocation("w"), call.GetViewport().Width());
        glUniform1i(pc_reconstruction0_shdr->ParameterLocation("approach"), approach);
        glUniform1i(pc_reconstruction0_shdr->ParameterLocation("frametype"), frametype);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        pc_reconstruction0_shdr->Disable();
        frametype = (frametype + 1) % 2;
    }

    if (approach == 1 && this->halveRes.Param<core::param::BoolParam>()->Value()) {
        glViewport(0, 0, call.GetViewport().Width(), call.GetViewport().Height());

        pc_reconstruction1_shdr->Enable();

        if (frametype == 0) {
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_2D, imageStorageA);
            glUniform1i(pc_reconstruction1_shdr->ParameterLocation("src_tx2Da"), 10);
        }
        if (frametype == 1) {
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_2D, imageStorageB);
            glUniform1i(pc_reconstruction1_shdr->ParameterLocation("src_tx2Db"), 11);
        }
        if (frametype == 2) {
            glActiveTexture(GL_TEXTURE12);
            glBindTexture(GL_TEXTURE_2D, imageStorageC);
            glUniform1i(pc_reconstruction1_shdr->ParameterLocation("src_tx2Dc"), 12);
        }
        if (frametype == 3) {
            glActiveTexture(GL_TEXTURE13);
            glBindTexture(GL_TEXTURE_2D, imageStorageD);
            glUniform1i(pc_reconstruction1_shdr->ParameterLocation("src_tx2Dd"), 13);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, origFBO);
        glUniform1i(pc_reconstruction1_shdr->ParameterLocation("h"), call.GetViewport().Height());
        glUniform1i(pc_reconstruction1_shdr->ParameterLocation("w"), call.GetViewport().Width());
        glUniform1i(pc_reconstruction1_shdr->ParameterLocation("approach"), approach);
        glUniform1i(pc_reconstruction1_shdr->ParameterLocation("frametype"), frametype);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        pc_reconstruction1_shdr->Disable();
        frametype = (frametype + 1) % framesNeeded;
    }

    //  Method for DepthTest
    if (false && this->halveRes.Param<core::param::BoolParam>()->Value()) {
        glViewport(0, 0, call.GetViewport().Width(), call.GetViewport().Height());

        pc_reconstruction0_shdr->Enable();

        if (call.frametype == 0 || call.frametype == 1) {
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_2D, imageStorageA);
            // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w,h,0, GL_RGB, GL_FLOAT, 0);
            //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_GREEN, GL_UNSIGNED_BYTE, &tex2[0]);
            glUniform1i(pc_reconstruction0_shdr->ParameterLocation("src_tx2D"), 10);
        }
        if (call.frametype == 2) {
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_2D, imageStorageB);
            // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, &tex2);
            glUniform1i(pc_reconstruction0_shdr->ParameterLocation("src_tx2Db"), 11);
        }


        glBindFramebuffer(GL_FRAMEBUFFER, origFBO);
        glUniform1i(pc_reconstruction0_shdr->ParameterLocation("h"), call.GetViewport().Width());

        glUniform1i(pc_reconstruction0_shdr->ParameterLocation("frametype"), call.frametype);

      
        //glStencilMask(0x00);
        glBindTexture(GL_TEXTURE_2D, imageStorageA);
        glDepthFunc(GL_ALWAYS);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDisable(GL_STENCIL_TEST);
        // vislib::sys::Log::DefaultLog.WriteInfo("h %i", glIsEnabled(GL_DEPTH_TEST));
        pc_reconstruction0_shdr->Disable();
        // glDeleteTextures(1, &imStoreI);
        // glDeleteTextures(1, &imStoreI2);
        // glDeleteFramebuffers(1, &nuFB);
        // glDeleteFramebuffers(1, &nuDRB);
        // vislib::sys::Log::DefaultLog.WriteInfo("x: %d , y: %d , xM: %d , yM: %d ", call.GetBoundingBox().GetLeft(),
        //    call.GetBoundingBox().GetTop(), call.GetBoundingBox().GetRight(), call.GetBoundingBox().GetBottom());
    }

    return true;
}