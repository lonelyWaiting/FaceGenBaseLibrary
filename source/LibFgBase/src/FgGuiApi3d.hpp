//
// Copyright (c) 2015 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
// Authors:     Andrew Beatty
// Created:     March 29, 2011
//

#ifndef FGGUIAPI3D_HPP
#define FGGUIAPI3D_HPP

#include "FgGuiApiBase.hpp"
#include "FgDepGraph.hpp"
#include "Fg3dCamera.hpp"
#include "FgLighting.hpp"
#include "Fg3dRenderOptions.hpp"
#include "Fg3dNormals.hpp"

struct  FgGuiApi3d : public FgGuiApi<FgGuiApi3d>
{
    // Inputs:
    // If meshesN is not a sink, then surface point and marked vertex creation on it is enabled:
    FgDgn<vector<Fg3dMesh> >    meshesN;        // Possibly modified if not a sink
    FgDgn<vector<FgVerts> >     vertssN;        // Must be same size as meshesN
    FgDgn<vector<Fg3dNormals> > normssN;        // "
    FgDgn<vector<FgImgs> >      texssN;         // " and each texs can be of any size
    FgDgn<FgMat32D>             viewBounds;
    FgDgn<size_t>               panTiltMode;    // 0 - pan/tilt, 1 - unconstrained
    FgDgn<FgLighting>           light;
    FgDgn<Fg3dCamera>           xform;
    FgDgn<Fg3dRenderOptions>    renderOptions;
    FgDgn<size_t>               vertMarkModeN;  // 0 - single, 1 - edge seam, 2 - fold seam
    FgDgn<FgString>             pointLabel;     // Label for surface point creation
    bool                        panTiltLimits;  // Limit pan and tilt to +/- 90 degrees (default false)
    // Can be invalid. Can be empty. Must be pow2 square otherwise. Alpha defines foreground transparency:
    FgDgn<FgImgRgbaUb>          bgImgN;
    uint                        bgImgUpdateFlag;    // Need not be defined if bgImgN is invalid
    FgDgn<FgVect2UI>            bgImgOrigDimsN; // Allows display of correct aspect ratio

    // Modified:
    FgDgn<FgVect2D>             panTiltDegrees;
    FgDgn<FgQuaternionD>        pose;
    FgDgn<FgVect2D>             trans;
    FgDgn<double>               logRelSize;
    FgDgn<FgVect2UI>            viewportDims;

    uint                        updateFlagIdx;
    uint                        updateTexFlagIdx;

    // Actions:
    struct  VertIdx
    {
        size_t      meshIdx;
        size_t      vertIdx;
    };
    // bool: is shift key down as well ?
    boost::function<void(bool,VertIdx,FgVect3F)>    ctlDragAction;          // Can be empty
    // bool: is shift key down as well ? FgVect2I: drag delta in pixels
    boost::function<void(bool,FgVect2I)>            bothButtonsDragAction;  // "

    FgGuiApi3d() : panTiltLimits(false) {}

    // Implementation:
    void
    panTilt(FgVect2I delta);

    // Used by two-finger rotate gesture. Does nothing in pan-tilt mode:
    void
    roll(int delta);

    void
    scale(int delta);

    void
    translate(FgVect2I delta);

    void
    markSurfPoint(FgVect2UI winSize,FgVect2I pos,FgMat44F toOics);

    void
    markVertex(FgVect2UI winSize,FgVect2I pos,FgMat44F toOics);

    void
    ctlClick(FgVect2UI winSize,FgVect2I pos,FgMat44F toOics);

    void
    ctlDrag(bool left, FgVect2UI winSize,FgVect2I delta,FgMat44F toOics);

private:
    VertIdx         lastCtlClick;
};

#endif
