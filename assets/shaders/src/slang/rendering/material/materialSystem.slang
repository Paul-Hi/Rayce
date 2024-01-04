import core.globals;
__exported import rendering.material.material;
__exported import rendering.material.materialInterface;

extension Material
{
    IMaterial getMaterial()
    {
        bool smoothAlpha = max(this.alpha.x, this.alpha.y) < EPSILON;
        bool smoothDielectric = this.interiorIor / this.exteriorIor == 1.0;

        switch(this.bxdfType)
        {
            case EBxDFType::smoothConductor:
                {
                    return SmoothConductorMaterial(this);
                }
            case EBxDFType::roughConductor:
                {
                    if(smoothAlpha)
                    {
                        return SmoothConductorMaterial(this);
                    }

                    return RoughConductorMaterial(this);
                }
            case EBxDFType::smoothDielectricThin:
                {
                    return SmoothThinPlateDielectricMaterial(this);
                }
            case EBxDFType::smoothDielectric:
                {
                    return SmoothDielectricMaterial(this);
                }
            case EBxDFType::roughDielectric:
                {
                    if(smoothAlpha || smoothDielectric)
                    {
                        return SmoothDielectricMaterial(this);
                    }

                    return RoughDielectricMaterial(this);
                }
            case EBxDFType::smoothPlastic:
               {
                  return SmoothPlasticMaterial(this);
               }
            case EBxDFType::roughPlastic:
               {
                    if(smoothAlpha || smoothDielectric)
                    {
                        return SmoothPlasticMaterial(this);
                    }

                    return RoughPlasticMaterial(this);
               }
            case EBxDFType::lambertDiffuse:
            default:
                {
                    return DiffuseMaterial(this);
                }
        }
    }
}

extension IMaterial
{
    IAdapter getAdapter(bool twoSided)
    {
        if(twoSided == 1)
        {
            return TwoSidedAdapter();
        }

        return DefaultAdapter();
    }
}
