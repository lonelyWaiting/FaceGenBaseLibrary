//
// Copyright (c) 2015 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
// Authors:     Andrew Beatty
// Created:     Oct. 8, 2009
//

#include "stdafx.h"

#include "Fg3dMesh.hpp"
#include "FgException.hpp"
#include "FgStdStream.hpp"
#include "FgBounds.hpp"
#include "FgFileSystem.hpp"

using namespace std;

static string triIdent = "FRTRI003";

static
string
readString(istream & istr,bool wchar)
{
    uint        size;
    fgRead(istr,size);
    string      str;
    if (size == 0)
        return str;
    str.resize(size);
    for (uint ii=0; ii<size; ++ii) {
        if (wchar) {
            wchar_t     wch;
            fgRead(istr,wch);
            str[ii] = char(wch);
        }
        else
            istr.read(&str[ii],1);
    }
    // Get rid of NULL terminating character required by spec:
    str.resize(size-1);
    return str;
}

Fg3dMesh
fgLoadTri(istream & istr)
{
    Fg3dMesh            mesh;
    // Check for file type identifier
    char                cdata[9];
    istr.read(cdata,8);
    if (strncmp(cdata,"FRTRI103",8) == 0)
        fgThrow("File is encrypted, use 'fileconvert' utility to decrypt");
    if (strncmp(cdata,triIdent.data(),8) != 0)           // 0 indicates no difference
        fgThrow("File not in TRI format");
        // Read in the header
    uint32      numVerts,
                numTris,
                numQuads,
                numLabVerts,
                numSurfPts,
                numUvs,
                texExt,
                numDiffMorph,
                numStatMorph,
                numStatMorphVerts;
    char        buff[16];
    fgRead(istr,numVerts);
    fgRead(istr,numTris);
    fgRead(istr,numQuads);
    fgRead(istr,numLabVerts);
    fgRead(istr,numSurfPts);
    fgRead(istr,numUvs);
    fgRead(istr,texExt);
    fgRead(istr,numDiffMorph);
    fgRead(istr,numStatMorph);
    fgRead(istr,numStatMorphVerts);
    istr.read(buff,16);
    bool    texs = ((texExt & 0x01) != 0),
            wchar = ((texExt & 0x02) != 0);
    if (wchar)
        fgout << fgnl << "WARNING: Unicode labels being converted to ASCII.";

    // Read in the verts:
    FGASSERT(numVerts > 0);     // Valid TRI must have to have verts
    mesh.verts.resize(numVerts);
    vector<FgVect3F>    targVerts(numStatMorphVerts);
    istr.read(reinterpret_cast<char*>(&mesh.verts[0]),int(12*numVerts));
    if (numStatMorphVerts > 0)
        istr.read(reinterpret_cast<char*>(&targVerts[0]),int(12*numStatMorphVerts));

    // Read in the surface (a TRI has only one):
    std::vector<FgVect3UI>  tris(numTris);
    if (numTris > 0)
        istr.read(reinterpret_cast<char*>(&tris[0]),int(12*numTris));
    std::vector<FgVect4UI>  quads(numQuads);
    if (numQuads > 0)
        istr.read(reinterpret_cast<char*>(&quads[0]),int(16*numQuads));
    // Marked verts:
    mesh.markedVerts.resize(numLabVerts);
    for (uint jj=0; jj<numLabVerts; jj++) {
        istr.read(reinterpret_cast<char*>(&mesh.markedVerts[jj].idx),4);
        mesh.markedVerts[jj].label = readString(istr,wchar);
    }
    // Surface points:
    std::vector<FgSurfPoint>   surfPoints;
    for (uint ii=0; ii<numSurfPts; ii++) {
        FgSurfPoint     sp;
        fgRead(istr,sp.triEquivIdx);
        fgRead(istr,sp.weights);
        sp.label = readString(istr,wchar);
        surfPoints.push_back(sp);
    }
    // Texture coordinates:
    vector<FgVect3UI>   triUvInds;
    vector<FgVect4UI>   quadUvInds;
    if (numUvs > 0) {
        triUvInds.resize(tris.size());
        quadUvInds.resize(quads.size());
        mesh.uvs.resize(numUvs);
        istr.read(reinterpret_cast<char*>(&mesh.uvs[0]),int(8*numUvs));
        if (tris.size() > 0)
            istr.read(reinterpret_cast<char*>(&triUvInds[0]),int(12*numTris));
        if (quads.size() > 0)
            istr.read(reinterpret_cast<char*>(&quadUvInds[0]),int(16*numQuads));
    }
    else if (texs) { // In the case of per vertex UVs we have to convert to indexed UVs
        triUvInds.resize(tris.size());
        quadUvInds.resize(quads.size());
        mesh.uvs.resize(mesh.verts.size());
        istr.read(reinterpret_cast<char*>(&mesh.uvs[0]),int(8*numVerts));
        for (uint ii=0; ii<tris.size(); ii++)
            for (uint jj=0; jj<3; jj++)
                triUvInds[ii][jj] = tris[ii][jj];
        for (uint ii=0; ii<quads.size(); ii++)
            for (uint jj=0; jj<4; jj++)
                quadUvInds[ii][jj] = quads[ii][jj];
    }
    // Delta morphs:
    mesh.deltaMorphs.resize(numDiffMorph);
    for (uint mm=0; mm<numDiffMorph; mm++) {
        mesh.deltaMorphs[mm].name = readString(istr,wchar);
        mesh.deltaMorphs[mm].verts.resize(numVerts);
        float       scale;
        fgRead(istr,scale);
        for (uint vv=0; vv<numVerts; vv++) {
            FgVect3S    sval;
            fgRead(istr,sval);
            mesh.deltaMorphs[mm].verts[vv] = FgVect3F(sval) * scale;
        }
    }
    // Target morphs:
    size_t                      targVertsStart = 0;
    mesh.targetMorphs.reserve(numStatMorph);
    for (uint ii=0; ii<numStatMorph; ++ii) {
        FgIndexedMorph       tm;
        tm.name = readString(istr,wchar);
        uint32                      numTargVerts;
        fgRead(istr,numTargVerts);
        tm.baseInds.resize(numTargVerts);
        istr.read((char*)&tm.baseInds[0],4*numTargVerts);
        tm.verts = fgSubvec(targVerts,targVertsStart,numTargVerts);
        targVertsStart += numTargVerts;
        mesh.targetMorphs.push_back(tm);
    }
    mesh.surfaces = fgSvec(Fg3dSurface(tris,quads,triUvInds,quadUvInds,surfPoints));
    return mesh;
}

Fg3dMesh
fgLoadTri(const FgString & fname)
{
    Fg3dMesh        ret;
    try {
        FgIfstream      ff(fname);
        ret = fgLoadTri(ff);
    }
    catch (FgException & e) {
        e.m_ct.back().data = fname;
        throw;
    }
    ret.name = fgPathToBase(fname);
    return ret;
}

Fg3dMesh
fgLoadTri(
    const FgString &    meshFile,
    const FgString &    texImage)
{
    Fg3dMesh        mesh = fgLoadTri(meshFile);
    FgImgRgbaUb     image;
    fgLoadImgAnyFormat(texImage,image);
    mesh.texImages.push_back(image);
    return mesh;
}

static
void
writeLabel(ostream & ostr,const string & str)
{
    // The spec requires writing a null terminator after the string:
    fgWrite(ostr,uint32(str.length()+1));
    ostr.write(str.c_str(),str.length()+1);
}

void
fgSaveTri(
    const FgString &    fname,
    const Fg3dMesh &    inMesh)
{
    Fg3dMesh        mesh(inMesh);
    if (mesh.surfaces.size() > 1)
        mesh.mergeAllSurfaces();
    const Fg3dSurface surf =
        (mesh.surfaces.size() > 0) ? mesh.surfaces[0] : Fg3dSurface();
    const vector<FgSurfPoint> & surfPoints = surf.surfPoints;

    size_t              numTargetMorphVerts = mesh.numTargetMorphVerts(),
                        numBaseVerts = mesh.verts.size();
    FgOfstream          ff(fname);
    ff.write(triIdent.data(),8);
    fgWrite(ff,int32(numBaseVerts));               // V
    fgWrite(ff,int32(surf.numTris()));             // T
    fgWrite(ff,int32(surf.numQuads()));            // Q
    fgWrite(ff,int32(mesh.markedVerts.size()));    // numLabVerts (LV)
    fgWrite(ff,int32(surfPoints.size()));          // numSurfPts (LS)
    int32       numUvs = int32(mesh.uvs.size());
    fgWrite(ff,int32(numUvs));                     // numUvs (X > 0 -> per-facet texture coordinates)
    if (surf.hasUvIndices())
        fgWrite(ff,int32(0x01));                   // <ext>: 0x01 -> texture coordinates
    else
        fgWrite(ff,int32(0));
    fgWrite(ff,int32(mesh.deltaMorphs.size()));    // numDiffMorph
    fgWrite(ff,int32(mesh.targetMorphs.size()));   // numStatMorph
    fgWrite(ff,int32(numTargetMorphVerts));        // numStatMorphVerts
    fgWrite(ff,int32(0));
    fgWrite(ff,int32(0));
    fgWrite(ff,int32(0));
    fgWrite(ff,int32(0));

    // Verts:
    for (uint ii=0; ii<mesh.verts.size(); ++ii)
        fgWrite(ff,mesh.verts[ii]);
    for (size_t ii=0; ii<mesh.targetMorphs.size(); ++ii) {
        const FgIndexedMorph &   tm = mesh.targetMorphs[ii];
        for (size_t jj=0; jj<tm.verts.size(); ++jj)
            fgWrite(ff,tm.verts[jj]);
    }

    // Facets:
    for (uint ii=0; ii<surf.numTris(); ++ii)
        fgWrite(ff,surf.getTri(ii));
    for (uint ii=0; ii<surf.numQuads(); ++ii)
        fgWrite(ff,surf.getQuad(ii));

    // Marked Verts:
    for (size_t ii=0; ii<mesh.markedVerts.size(); ++ii) {
        fgWrite(ff,mesh.markedVerts[ii].idx);
        writeLabel(ff,mesh.markedVerts[ii].label);
    }

    // Surface Points:
    for (size_t ii=0; ii<surfPoints.size(); ii++) {
        const FgSurfPoint &     sp = surfPoints[ii];
        fgWrite(ff,sp.triEquivIdx);
        fgWrite(ff,sp.weights);
        writeLabel(ff,sp.label);
    }
    // UV list and per-facet UV indices if present:
    if (surf.hasUvIndices())
    {
        for(size_t ii=0; ii < mesh.uvs.size(); ++ii)
            fgWrite(ff,mesh.uvs[ii]);
        for(size_t ii=0; ii < surf.tris.uvInds.size(); ++ii)
            fgWrite(ff,surf.tris.uvInds[ii]);
        for(uint ii=0; ii < surf.quads.uvInds.size(); ++ii)
            fgWrite(ff,surf.quads.uvInds[ii]);
    }

    // Delta morphs:
    for (size_t ii=0; ii<mesh.deltaMorphs.size(); ++ii) {   
        const FgMorph &    morph = mesh.deltaMorphs[ii];
        FGASSERT(!morph.verts.empty());
        writeLabel(ff,morph.name.as_ascii());
        float   scale =
            float(numeric_limits<short>::max()-1) /
            fgMaxElem(fgAbs(fgBounds(morph.verts)));
        fgWrite(ff,1.0f/scale);
        for (size_t jj=0; jj<morph.verts.size(); ++jj)
            for (size_t kk=0; kk<3; ++kk)
                fgWrite(ff,short(std::floor(morph.verts[jj][kk]*scale)+0.5f));
    }

    // Target morphs:
    for (size_t ii=0; ii<mesh.targetMorphs.size(); ++ii) {
        const FgIndexedMorph &   morph = mesh.targetMorphs[ii];
        writeLabel(ff,morph.name.as_ascii());
        fgWrite(ff,uint32(morph.baseInds.size()));
        for (size_t jj=0; jj<morph.baseInds.size(); ++jj)
            fgWrite(ff,uint32(morph.baseInds[jj]));
    }
}
