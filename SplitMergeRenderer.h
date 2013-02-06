/*
 * SplitMergeRenderer.h
 *
 * Author: Guido Reina
 * Copyright (C) 2012 by Universitaet Stuttgart (VISUS).
 * All rights reserved.
 */

#ifndef MEGAMOLCORE_SPLITMERGERENDERER_H_INCLUDED
#define MEGAMOLCORE_SPLITMERGERENDERER_H_INCLUDED
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
#pragma once
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */

#include "param/ParamSlot.h"
#include "CallerSlot.h"
#include "view/Renderer2DModule.h"
#include "SplitMergeCall.h"
#include "IntSelectionCall.h"
#include "vislib/GLSLShader.h"
#include <vislib/OpenGLTexture2D.h>
#include "vislib/Rectangle.h"

namespace megamol {
namespace protein {

    class SplitMergeRenderer : public megamol::core::view::Renderer2DModule {
    public:

        /**
         * Answer the name of this module.
         *
         * @return The name of this module.
         */
        static const char *ClassName(void) {
            return "SplitMergeRenderer";
        }

        /**
         * Answer a human readable description of this module.
         *
         * @return A human readable description of this module.
         */
        static const char *Description(void) {
            return "renders entities with a metric property and how this "
                "property is propagated between entities";
        }

        /**
         * Answers whether this module is available on the current system.
         *
         * @return 'true' if the module is available, 'false' otherwise.
         */
        static bool IsAvailable(void) {
            return true;
        }

        /** ctor */
        SplitMergeRenderer(void);

        /** dtor */
        ~SplitMergeRenderer(void);

    protected:
        
        /**
         * Implementation of 'Create'.
         *
         * @return 'true' on success, 'false' otherwise.
         */
        virtual bool create(void);
        
        /**
         * Implementation of 'Release'.
         */
        virtual void release(void);

        /**
         * Callback for mouse events (move, press, and release)
         *
         * @param x The x coordinate of the mouse in world space
         * @param y The y coordinate of the mouse in world space
         * @param flags The mouse flags
         */
        virtual bool MouseEvent(float x, float y, megamol::core::view::MouseFlags flags);

    private:

        /**********************************************************************
         * 'render'-functions
         **********************************************************************/

        void calcExtents();

        void closePath(SplitMergeCall::SplitMergeMappable *smm, int seriesIdx,
            vislib::Array<GLubyte> &cmds, vislib::Array<float> &coords, int idx, int start);

        /**
         * The get extents callback. The module should set the members of
         * 'call' to tell the caller the extents of its data (bounding boxes
         * and times).
         *
         * @param call The calling call.
         *
         * @return The return value of the function.
         */
        virtual bool GetExtents(megamol::core::view::CallRender2D& call);

        /**
        * The Open GL Render callback.
        *
        * @param call The calling call.
        * @return The return value of the function.
        */
        virtual bool Render(megamol::core::view::CallRender2D& call);

        /**********************************************************************
         * variables
         **********************************************************************/

        /** caller slot */
        core::CallerSlot dataCallerSlot;
        
        /** caller slot */
        core::CallerSlot selectionCallerSlot;

        /** caller slot */
        core::CallerSlot hiddenCallerSlot;

        SplitMergeCall *diagram;
        
        IntSelectionCall *selectionCall;

        IntSelectionCall *hiddenCall;

        ///** clear diagram parameter */
        //megamol::core::param::ParamSlot clearDiagramParam;

        megamol::core::param::ParamSlot showGuidesParam;

        megamol::core::param::ParamSlot foregroundColorParam;

        megamol::core::param::ParamSlot visibilityFromSelection;

        megamol::core::param::ParamSlot numVisibilityPropagationRounds;

        vislib::math::Vector<float, 4> fgColor;
        
        const vislib::math::Vector<float, 4> unselectedColor;

        GLuint fontBase;
        GLuint numChars;
        //GLuint pathBase;
        float maxY;
        float seriesSpacing;
        float noseLength;
        float fontSize;
        vislib::Array<int> sortedSeries;
        vislib::Array<int> sortedSeriesInverse;
        vislib::math::Rectangle<float> bounds;
        SplitMergeCall::SplitMergeSeries *selectedSeries;
        vislib::Array<bool> seriesVisible;
        vislib::Array<int> selectionLevel;
    };

} /* end namespace protein */
} /* end namespace megamol */

#endif // MEGAMOLCORE_SPLITMERGERENDERER_H_INCLUDED
