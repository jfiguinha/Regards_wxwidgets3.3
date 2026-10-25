// Énumération des modes de fusion alignée sur LayerBlendMode
#define BLEND_NORMAL   0
#define BLEND_MULTIPLY 1
#define BLEND_SCREEN   2
#define BLEND_DARKEN   4
#define BLEND_LIGHTEN  5
#define BLEND_ADD      6
#define BLEND_SUBTRACT 7

inline float blendChannel(float src, float dst, int mode) {
    if (mode == BLEND_MULTIPLY) return src * dst;
    if (mode == BLEND_SCREEN)   return src + dst - src * dst;
    if (mode == BLEND_DARKEN)   return min(src, dst);
    if (mode == BLEND_LIGHTEN)  return max(src, dst);
    if (mode == BLEND_ADD)      return min(1.0f, src + dst);
    if (mode == BLEND_SUBTRACT) return max(0.0f, dst - src);
    return src; // Normal
}

__kernel void compositeLayersKernel(
    __global const uchar4* bg,      // Image de fond (BGRA 8 bits)
    __global const uchar4* layers,  // Buffer contenant tous les calques concaténés
    __global const float* opacities,// Tableau des opacités par calque (0.0 à 1.0)
    __global const int* modes,      // Tableau des modes de fusion par calque
    __global uchar4* dst,           // Image finale de sortie (BGRA 8 bits)
    int numLayers,
    int width,
    int height,
    long layerStridePixels)         // Nombre de pixels séparant le début de chaque calque (width * height)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height)
        return;

    int pixelIdx = y * width + x;

    // Charger le fond et le normaliser dans [0.0, 1.0]
    uchar4 bgPixel = bg[pixelIdx];
    float4 accumColor = (float4)(bgPixel.x / 255.0f, bgPixel.y / 255.0f, bgPixel.z / 255.0f, bgPixel.w / 255.0f);

    // Itérer sur chaque calque pour CE pixel précis
    for (int i = 0; i < numLayers; ++i) {
        long currentLayerPixelIdx = (long)i * layerStridePixels + pixelIdx;
        uchar4 layerPixel = layers[currentLayerPixelIdx];
        
        float opacity = opacities[i];
        int mode = modes[i];

        float srcAlpha = (layerPixel.w / 255.0f) * opacity;
        if (srcAlpha <= 0.0f)
            continue;

        float dstAlpha = accumColor.w;
        float outAlpha = srcAlpha + dstAlpha * (1.0f - srcAlpha);

        float3 srcColor = (float3)(layerPixel.x / 255.0f, layerPixel.y / 255.0f, layerPixel.z / 255.0f);
        float3 dstColor = (float3)(accumColor.x, accumColor.y, accumColor.z);
        
        float3 blendedColor;
        blendedColor.x = blendChannel(srcColor.x, dstColor.x, mode);
        blendedColor.y = blendChannel(srcColor.y, dstColor.y, mode);
        blendedColor.z = blendChannel(srcColor.z, dstColor.z, mode);

        // Formule de composition W3C (Espace non-prémultiplié)
        float3 premultiplied = (1.0f - srcAlpha) * dstAlpha * dstColor +
                               (1.0f - dstAlpha) * srcAlpha * srcColor +
                               srcAlpha * dstAlpha * blendedColor;

        accumColor.xyz = (outAlpha > 0.0f) ? (premultiplied / outAlpha) : (float3)(0.0f);
        accumColor.w = outAlpha;
    }

    // Écrêtage et écriture finale au format 8 bits BGRA
    dst[pixelIdx] = (uchar4)(
        (uchar)(clamp(accumColor.x, 0.0f, 1.0f) * 255.0f),
        (uchar)(clamp(accumColor.y, 0.0f, 1.0f) * 255.0f),
        (uchar)(clamp(accumColor.z, 0.0f, 1.0f) * 255.0f),
        (uchar)(clamp(accumColor.w, 0.0f, 1.0f) * 255.0f)
    );
}
