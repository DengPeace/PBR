//
//  pbrMaterial.h
//  PBR Material — Cook-Torrance BRDF
//

#pragma once

#include "material.h"
#include "../texture.h"

class PBRMaterial : public Material {
public:
    PBRMaterial();
    ~PBRMaterial();

public:
    Texture*    mAlbedoMap { nullptr };
    Texture*    mNormalMap { nullptr };
    Texture*    mMetallicMap { nullptr };
    Texture*    mRoughnessMap { nullptr };
    Texture*    mAoMap { nullptr };
    Texture*    mEnvMap { nullptr };       // Spherical environment map
};
