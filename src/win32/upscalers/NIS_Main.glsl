// NVIDIA Image Scaling, as GLSL 4.30 compute.
//
// Generated, not written: glslang compiles the SDK's NIS_Main.hlsl with its
// HLSL front end and spirv-cross turns the SPIR-V back into GLSL, so the
// algorithm below is the one the Direct3D 11 backend runs through fxc.  The
// only edits are the sampler and block names and an rgba8 output image, which
// is the format the Direct3D 11 targets use.  Regenerate with
// .workbuddy-ai/tools/transpile-nis.py rather than editing this file.
//
// NVIDIA Image Scaling SDK - v1.0.3, MIT licence, see licence.txt.
#version 430
layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, std140) uniform cb
{
    float kDetectRatio;
    float kDetectThres;
    float kMinContrastRatio;
    float kRatioNorm;
    float kContrastBoost;
    float kEps;
    float kSharpStartY;
    float kSharpScaleY;
    float kSharpStrengthMin;
    float kSharpStrengthScale;
    float kSharpLimitMin;
    float kSharpLimitScale;
    float kScaleX;
    float kScaleY;
    float kDstNormX;
    float kDstNormY;
    float kSrcNormX;
    float kSrcNormY;
    uint kInputViewportOriginX;
    uint kInputViewportOriginY;
    uint kInputViewportWidth;
    uint kInputViewportHeight;
    uint kOutputViewportOriginX;
    uint kOutputViewportOriginY;
    uint kOutputViewportWidth;
    uint kOutputViewportHeight;
    float reserved0;
    float reserved1;
} config;

layout(binding = 0, rgba8) uniform writeonly image2D out_texture;
uniform sampler2D inputTexture;
uniform sampler2D coefScalerTexture;
uniform sampler2D coefUsmTexture;

shared float shCoefScaler[64][6];
shared float shCoefUSM[64][6];
shared float shPixelsY[1140];
shared vec4 shEdgeMap[884];

void main()
{
    uvec2 _1763 = gl_WorkGroupID.xy;
    uint _1857 = _1763.x;
    int _1859 = int(32u * _1857);
    uint _1861 = _1763.y;
    int _1863 = int(24u * _1861);
    int _1872 = int(floor(((float(_1859) + 0.5) * config.kScaleX) - 0.5));
    int _1881 = int(floor(((float(_1863) + 0.5) * config.kScaleY) - 0.5));
    int _1906 = (int(ceil(((float(_1859 + 32) + 0.5) * config.kScaleX) - 0.5)) - _1872) + 5;
    int _1911 = (int(ceil(((float(_1863 + 24) + 0.5) * config.kScaleY) - 0.5)) - _1881) + 5;
    int _1915 = _1906 + (_1906 & 1);
    int _1919 = _1911 + (_1911 & 1);
    int _1922 = _1915 * _1919;
    int _1925 = _1915 + (-4);
    int _1931 = _1925 * (_1919 + (-4));
    uint _1933 = gl_LocalInvocationID.x * 2u;
    float _1792[2][2];
    for (uint _4753 = _1933; _4753 < (uint(_1922) >> uint(1)); _4753 += 256u)
    {
        uint _1945 = _4753 / uint(_1915);
        uint _1950 = _4753 % uint(_1915);
        float _1960 = (float(uint(_1872) + _1950) + (-1.5)) * config.kSrcNormX;
        float _1970 = (float(uint(_1881) + (_1945 * 2u)) + (-1.5)) * config.kSrcNormY;
        vec3 _1793 = textureLod(inputTexture, vec2(_1960, _1970), 0.0).xyz;
        _1792[0][0] = ((0.2125999927520751953125 * _1793.x) + (0.715200006961822509765625 * _1793.y)) + (0.072200000286102294921875 * _1793.z);
        _1793 = textureLod(inputTexture, vec2(_1960 + config.kSrcNormX, _1970), 0.0).xyz;
        _1792[0][1] = ((0.2125999927520751953125 * _1793.x) + (0.715200006961822509765625 * _1793.y)) + (0.072200000286102294921875 * _1793.z);
        _1793 = textureLod(inputTexture, vec2(_1960, _1970 + config.kSrcNormY), 0.0).xyz;
        _1792[1][0] = ((0.2125999927520751953125 * _1793.x) + (0.715200006961822509765625 * _1793.y)) + (0.072200000286102294921875 * _1793.z);
        _1793 = textureLod(inputTexture, vec2(_1960 + config.kSrcNormX, _1970 + config.kSrcNormY), 0.0).xyz;
        _1792[1][1] = ((0.2125999927520751953125 * _1793.x) + (0.715200006961822509765625 * _1793.y)) + (0.072200000286102294921875 * _1793.z);
        uint _2017 = (_1945 * 76u) + _1950;
        shPixelsY[_2017] = _1792[0][0];
        shPixelsY[_2017 + 1u] = _1792[0][1];
        shPixelsY[_2017 + 38u] = _1792[1][0];
        shPixelsY[_2017 + 39u] = _1792[1][1];
    }
    barrier();
    uint _2043 = gl_LocalInvocationID.x * 2u;
    float _1802[4][4];
    for (uint _4754 = _2043; _4754 < (uint(_1931) >> uint(1)); _4754 += 256u)
    {
        uint _2055 = _4754 / uint(_1925);
        uint _2060 = _4754 % uint(_1925);
        uint _2064 = (_2055 * 68u) + _2060;
        uint _2069 = (((_2055 * 2u) + 1u) * 38u) + _2060;
        _1802[0][0] = shPixelsY[_2069 + 1u];
        _1802[0][1] = shPixelsY[_2069 + 2u];
        _1802[0][2] = shPixelsY[_2069 + 3u];
        _1802[0][3] = shPixelsY[_2069 + 4u];
        _1802[1][0] = shPixelsY[_2069 + 39u];
        _1802[1][1] = shPixelsY[_2069 + 40u];
        _1802[1][2] = shPixelsY[_2069 + 41u];
        _1802[1][3] = shPixelsY[_2069 + 42u];
        _1802[2][0] = shPixelsY[_2069 + 77u];
        _1802[2][1] = shPixelsY[_2069 + 78u];
        _1802[2][2] = shPixelsY[_2069 + 79u];
        _1802[2][3] = shPixelsY[_2069 + 80u];
        _1802[3][0] = shPixelsY[_2069 + 115u];
        _1802[3][1] = shPixelsY[_2069 + 116u];
        _1802[3][2] = shPixelsY[_2069 + 117u];
        _1802[3][3] = shPixelsY[_2069 + 118u];
        float _1803[4][4] = _1802;
        vec4 _4865;
        do
        {
            float _2427 = abs(((((_1803[0][0] + _1803[0][1]) + _1803[0][2]) - _1803[2][0]) - _1803[2][1]) - _1803[2][2]);
            float _2469 = abs(((((_1803[1][0] + _1803[0][0]) + _1803[0][1]) - _1803[2][1]) - _1803[2][2]) - _1803[1][2]);
            float _2511 = abs(((((_1803[0][0] + _1803[1][0]) + _1803[2][0]) - _1803[0][2]) - _1803[1][2]) - _1803[2][2]);
            float _2553 = abs(((((_1803[1][0] + _1803[2][0]) + _1803[2][1]) - _1803[0][1]) - _1803[0][2]) - _1803[1][2]);
            float _2556 = max(_2427, _2511);
            float _2559 = min(_2427, _2511);
            float _2562 = max(_2469, _2553);
            float _2565 = min(_2469, _2553);
            if ((_2556 + _2562) == 0.0)
            {
                _4865 = vec4(0.0);
                break;
            }
            float _2577 = min(_2556 / (_2556 + _2562), 1.0);
            bool _2594 = ((_2556 > (_2559 * config.kDetectRatio)) && (_2556 > config.kDetectThres)) && (_2556 > _2565);
            bool _2609 = ((_2562 > (_2565 * config.kDetectRatio)) && (_2562 > config.kDetectThres)) && (_2562 > _2559);
            bool _2612 = _2556 == _2427;
            bool _2615 = _2562 == _2469;
            float _2620 = (_2594 && _2609) ? _2577 : 1.0;
            float _2625 = (_2594 && _2609) ? (1.0 - _2577) : 1.0;
            _4865 = vec4((_2594 && _2612) ? _2620 : 0.0, (_2594 && (!_2612)) ? _2620 : 0.0, (_2609 && _2615) ? _2625 : 0.0, (_2609 && (!_2615)) ? _2625 : 0.0);
            break;
        } while(false);
        shEdgeMap[_2064] = _4865;
        float _1806[4][4] = _1802;
        vec4 _4866;
        do
        {
            float _2721 = abs(((((_1806[0][1] + _1806[0][2]) + _1806[0][3]) - _1806[2][1]) - _1806[2][2]) - _1806[2][3]);
            float _2763 = abs(((((_1806[1][1] + _1806[0][1]) + _1806[0][2]) - _1806[2][2]) - _1806[2][3]) - _1806[1][3]);
            float _2805 = abs(((((_1806[0][1] + _1806[1][1]) + _1806[2][1]) - _1806[0][3]) - _1806[1][3]) - _1806[2][3]);
            float _2847 = abs(((((_1806[1][1] + _1806[2][1]) + _1806[2][2]) - _1806[0][2]) - _1806[0][3]) - _1806[1][3]);
            float _2850 = max(_2721, _2805);
            float _2853 = min(_2721, _2805);
            float _2856 = max(_2763, _2847);
            float _2859 = min(_2763, _2847);
            if ((_2850 + _2856) == 0.0)
            {
                _4866 = vec4(0.0);
                break;
            }
            float _2871 = min(_2850 / (_2850 + _2856), 1.0);
            bool _2888 = ((_2850 > (_2853 * config.kDetectRatio)) && (_2850 > config.kDetectThres)) && (_2850 > _2859);
            bool _2903 = ((_2856 > (_2859 * config.kDetectRatio)) && (_2856 > config.kDetectThres)) && (_2856 > _2853);
            bool _2906 = _2850 == _2721;
            bool _2909 = _2856 == _2763;
            float _2914 = (_2888 && _2903) ? _2871 : 1.0;
            float _2919 = (_2888 && _2903) ? (1.0 - _2871) : 1.0;
            _4866 = vec4((_2888 && _2906) ? _2914 : 0.0, (_2888 && (!_2906)) ? _2914 : 0.0, (_2903 && _2909) ? _2919 : 0.0, (_2903 && (!_2909)) ? _2919 : 0.0);
            break;
        } while(false);
        shEdgeMap[_2064 + 1u] = _4866;
        float _1809[4][4] = _1802;
        vec4 _4867;
        do
        {
            float _3015 = abs(((((_1809[1][0] + _1809[1][1]) + _1809[1][2]) - _1809[3][0]) - _1809[3][1]) - _1809[3][2]);
            float _3057 = abs(((((_1809[2][0] + _1809[1][0]) + _1809[1][1]) - _1809[3][1]) - _1809[3][2]) - _1809[2][2]);
            float _3099 = abs(((((_1809[1][0] + _1809[2][0]) + _1809[3][0]) - _1809[1][2]) - _1809[2][2]) - _1809[3][2]);
            float _3141 = abs(((((_1809[2][0] + _1809[3][0]) + _1809[3][1]) - _1809[1][1]) - _1809[1][2]) - _1809[2][2]);
            float _3144 = max(_3015, _3099);
            float _3147 = min(_3015, _3099);
            float _3150 = max(_3057, _3141);
            float _3153 = min(_3057, _3141);
            if ((_3144 + _3150) == 0.0)
            {
                _4867 = vec4(0.0);
                break;
            }
            float _3165 = min(_3144 / (_3144 + _3150), 1.0);
            bool _3182 = ((_3144 > (_3147 * config.kDetectRatio)) && (_3144 > config.kDetectThres)) && (_3144 > _3153);
            bool _3197 = ((_3150 > (_3153 * config.kDetectRatio)) && (_3150 > config.kDetectThres)) && (_3150 > _3147);
            bool _3200 = _3144 == _3015;
            bool _3203 = _3150 == _3057;
            float _3208 = (_3182 && _3197) ? _3165 : 1.0;
            float _3213 = (_3182 && _3197) ? (1.0 - _3165) : 1.0;
            _4867 = vec4((_3182 && _3200) ? _3208 : 0.0, (_3182 && (!_3200)) ? _3208 : 0.0, (_3197 && _3203) ? _3213 : 0.0, (_3197 && (!_3203)) ? _3213 : 0.0);
            break;
        } while(false);
        shEdgeMap[_2064 + 34u] = _4867;
        float _1812[4][4] = _1802;
        vec4 _4868;
        do
        {
            float _3309 = abs(((((_1812[1][1] + _1812[1][2]) + _1812[1][3]) - _1812[3][1]) - _1812[3][2]) - _1812[3][3]);
            float _3351 = abs(((((_1812[2][1] + _1812[1][1]) + _1812[1][2]) - _1812[3][2]) - _1812[3][3]) - _1812[2][3]);
            float _3393 = abs(((((_1812[1][1] + _1812[2][1]) + _1812[3][1]) - _1812[1][3]) - _1812[2][3]) - _1812[3][3]);
            float _3435 = abs(((((_1812[2][1] + _1812[3][1]) + _1812[3][2]) - _1812[1][2]) - _1812[1][3]) - _1812[2][3]);
            float _3438 = max(_3309, _3393);
            float _3441 = min(_3309, _3393);
            float _3444 = max(_3351, _3435);
            float _3447 = min(_3351, _3435);
            if ((_3438 + _3444) == 0.0)
            {
                _4868 = vec4(0.0);
                break;
            }
            float _3459 = min(_3438 / (_3438 + _3444), 1.0);
            bool _3476 = ((_3438 > (_3441 * config.kDetectRatio)) && (_3438 > config.kDetectThres)) && (_3438 > _3447);
            bool _3491 = ((_3444 > (_3447 * config.kDetectRatio)) && (_3444 > config.kDetectThres)) && (_3444 > _3441);
            bool _3494 = _3438 == _3309;
            bool _3497 = _3444 == _3351;
            float _3502 = (_3476 && _3491) ? _3459 : 1.0;
            float _3507 = (_3476 && _3491) ? (1.0 - _3459) : 1.0;
            _4868 = vec4((_3476 && _3494) ? _3502 : 0.0, (_3476 && (!_3494)) ? _3502 : 0.0, (_3491 && _3497) ? _3507 : 0.0, (_3491 && (!_3497)) ? _3507 : 0.0);
            break;
        } while(false);
        shEdgeMap[_2064 + 35u] = _4868;
    }
    int _2127 = int(gl_LocalInvocationID.x);
    if (_2127 < 128)
    {
        int _3548 = _2127 >> 1;
        int _3550 = _2127 & 1;
        vec4 _3540 = texelFetch(coefScalerTexture, ivec2(_3550, _3548), 0);
        int _3557 = _3550 * 4;
        shCoefScaler[_3548][_3557] = _3540.x;
        shCoefScaler[_3548][_3557 + 1] = _3540.y;
        if (_3550 == 0)
        {
            shCoefScaler[_3548][2] = _3540.z;
            shCoefScaler[_3548][3] = _3540.w;
        }
        _3540 = texelFetch(coefUsmTexture, ivec2(_3550, _3548), 0);
        shCoefUSM[_3548][_3557] = _3540.x;
        shCoefUSM[_3548][_3557 + 1] = _3540.y;
        if (_3550 == 0)
        {
            shCoefUSM[_3548][2] = _3540.z;
            shCoefUSM[_3548][3] = _3540.w;
        }
    }
    barrier();
    ivec2 _1816 = ivec2(int(gl_LocalInvocationID.x % 32u), int(gl_LocalInvocationID.x / 32u));
    int _2139 = _1859 + _1816.x;
    float _2145 = (0.5 + float(_2139)) * config.kScaleX;
    float _2146 = _2145 - 0.5;
    int _2152 = int(floor(_2146) - float(_1872));
    float _2156 = _2146 - floor(_2146);
    int _2159 = int(_2156 * 64.0);
    vec4 _1831[2][2];
    float _1838[6][6];
    float _3678[6];
    float _3681[6];
    float _3684[7];
    float _3691[6];
    float _3695[7];
    float _3702[6];
    for (int _4755 = 0; _4755 < 6; _4755++)
    {
        int _2171 = (_1863 + _1816.y) + (_4755 * 4);
        float _2177 = (0.5 + float(_2171)) * config.kScaleY;
        float _2178 = _2177 - 0.5;
        int _2184 = int(floor(_2178) - float(_1881));
        float _2188 = _2178 - floor(_2178);
        int _2191 = int(_2188 * 64.0);
        int _2195 = (_2184 * 34) + _2152;
        _1831[0][0] = shEdgeMap[_2195];
        _1831[0][1] = shEdgeMap[_2195 + 1];
        _1831[1][0] = shEdgeMap[_2195 + 34];
        _1831[1][1] = shEdgeMap[_2195 + 35];
        vec4 _2229 = mix(mix(_1831[0][0], _1831[0][1], vec4(_2156)), mix(_1831[1][0], _1831[1][1], vec4(_2156)), vec4(_2188)) * 1.0;
        vec4 _1832 = _2229;
        int _2233 = (_2184 * 38) + _2152;
        _1838[0][0] = shPixelsY[_2233];
        _1838[0][1] = shPixelsY[_2233 + 1];
        _1838[0][2] = shPixelsY[_2233 + 2];
        _1838[0][3] = shPixelsY[_2233 + 3];
        _1838[0][4] = shPixelsY[_2233 + 4];
        _1838[0][5] = shPixelsY[_2233 + 5];
        _1838[1][0] = shPixelsY[_2233 + 38];
        _1838[1][1] = shPixelsY[_2233 + 39];
        _1838[1][2] = shPixelsY[_2233 + 40];
        _1838[1][3] = shPixelsY[_2233 + 41];
        _1838[1][4] = shPixelsY[_2233 + 42];
        _1838[1][5] = shPixelsY[_2233 + 43];
        _1838[2][0] = shPixelsY[_2233 + 76];
        _1838[2][1] = shPixelsY[_2233 + 77];
        _1838[2][2] = shPixelsY[_2233 + 78];
        _1838[2][3] = shPixelsY[_2233 + 79];
        _1838[2][4] = shPixelsY[_2233 + 80];
        _1838[2][5] = shPixelsY[_2233 + 81];
        _1838[3][0] = shPixelsY[_2233 + 114];
        _1838[3][1] = shPixelsY[_2233 + 115];
        _1838[3][2] = shPixelsY[_2233 + 116];
        _1838[3][3] = shPixelsY[_2233 + 117];
        _1838[3][4] = shPixelsY[_2233 + 118];
        _1838[3][5] = shPixelsY[_2233 + 119];
        _1838[4][0] = shPixelsY[_2233 + 152];
        _1838[4][1] = shPixelsY[_2233 + 153];
        _1838[4][2] = shPixelsY[_2233 + 154];
        _1838[4][3] = shPixelsY[_2233 + 155];
        _1838[4][4] = shPixelsY[_2233 + 156];
        _1838[4][5] = shPixelsY[_2233 + 157];
        _1838[5][0] = shPixelsY[_2233 + 190];
        _1838[5][1] = shPixelsY[_2233 + 191];
        _1838[5][2] = shPixelsY[_2233 + 192];
        _1838[5][3] = shPixelsY[_2233 + 193];
        _1838[5][4] = shPixelsY[_2233 + 194];
        _1838[5][5] = shPixelsY[_2233 + 195];
        float _3635[6][6] = _1838;
        float _3651 = _3635[0][0];
        _3635 = _1838;
        float _5404 = _3635[1][0];
        _3635 = _1838;
        float _5418 = _3635[2][0];
        _3635 = _1838;
        float _5432 = _3635[3][0];
        _3635 = _1838;
        float _5446 = _3635[4][0];
        _3635 = _1838;
        float _5460 = _3635[5][0];
        _3635 = _1838;
        float _5478 = _3635[0][1];
        _3635 = _1838;
        float _5490 = _3635[1][1];
        _3635 = _1838;
        float _5502 = _3635[2][1];
        _3635 = _1838;
        float _5514 = _3635[3][1];
        _3635 = _1838;
        float _5526 = _3635[4][1];
        _3635 = _1838;
        float _5538 = _3635[5][1];
        _3635 = _1838;
        float _5563 = _3635[0][2];
        _3635 = _1838;
        float _5575 = _3635[1][2];
        _3635 = _1838;
        float _5587 = _3635[2][2];
        _3635 = _1838;
        float _5599 = _3635[3][2];
        _3635 = _1838;
        float _5611 = _3635[4][2];
        _3635 = _1838;
        float _5634 = ((((((((_3651 * shCoefScaler[_2191][0]) + (_5404 * shCoefScaler[_2191][1])) + (_5418 * shCoefScaler[_2191][2])) + (_5432 * shCoefScaler[_2191][3])) + (_5446 * shCoefScaler[_2191][4])) + (_5460 * shCoefScaler[_2191][5])) * shCoefScaler[_2159][0]) + (((((((_5478 * shCoefScaler[_2191][0]) + (_5490 * shCoefScaler[_2191][1])) + (_5502 * shCoefScaler[_2191][2])) + (_5514 * shCoefScaler[_2191][3])) + (_5526 * shCoefScaler[_2191][4])) + (_5538 * shCoefScaler[_2191][5])) * shCoefScaler[_2159][1])) + (((((((_5563 * shCoefScaler[_2191][0]) + (_5575 * shCoefScaler[_2191][1])) + (_5587 * shCoefScaler[_2191][2])) + (_5599 * shCoefScaler[_2191][3])) + (_5611 * shCoefScaler[_2191][4])) + (_3635[5][2] * shCoefScaler[_2191][5])) * shCoefScaler[_2159][2]);
        _3635 = _1838;
        float _5648 = _3635[0][3];
        _3635 = _1838;
        float _5660 = _3635[1][3];
        _3635 = _1838;
        float _5672 = _3635[2][3];
        _3635 = _1838;
        float _5684 = _3635[3][3];
        _3635 = _1838;
        float _5696 = _3635[4][3];
        _3635 = _1838;
        float _5708 = _3635[5][3];
        _3635 = _1838;
        float _5733 = _3635[0][4];
        _3635 = _1838;
        float _5745 = _3635[1][4];
        _3635 = _1838;
        float _5757 = _3635[2][4];
        _3635 = _1838;
        float _5769 = _3635[3][4];
        _3635 = _1838;
        float _5781 = _3635[4][4];
        _3635 = _1838;
        float _5793 = _3635[5][4];
        _3635 = _1838;
        float _5818 = _3635[0][5];
        _3635 = _1838;
        float _5830 = _3635[1][5];
        _3635 = _1838;
        float _5842 = _3635[2][5];
        _3635 = _1838;
        float _5854 = _3635[3][5];
        _3635 = _1838;
        float _5866 = _3635[4][5];
        _3635 = _1838;
        float _5889 = ((_5634 + (((((((_5648 * shCoefScaler[_2191][0]) + (_5660 * shCoefScaler[_2191][1])) + (_5672 * shCoefScaler[_2191][2])) + (_5684 * shCoefScaler[_2191][3])) + (_5696 * shCoefScaler[_2191][4])) + (_5708 * shCoefScaler[_2191][5])) * shCoefScaler[_2159][3])) + (((((((_5733 * shCoefScaler[_2191][0]) + (_5745 * shCoefScaler[_2191][1])) + (_5757 * shCoefScaler[_2191][2])) + (_5769 * shCoefScaler[_2191][3])) + (_5781 * shCoefScaler[_2191][4])) + (_5793 * shCoefScaler[_2191][5])) * shCoefScaler[_2159][4])) + (((((((_5818 * shCoefScaler[_2191][0]) + (_5830 * shCoefScaler[_2191][1])) + (_5842 * shCoefScaler[_2191][2])) + (_5854 * shCoefScaler[_2191][3])) + (_5866 * shCoefScaler[_2191][4])) + (_3635[5][5] * shCoefScaler[_2191][5])) * shCoefScaler[_2159][5]);
        float _1843[6][6] = _1838;
        vec4 _1848 = _2229;
        float _4779;
        if (_1848.x > 0.0)
        {
            _3678[0] = mix(_1843[0][2], _1843[0][3], _2156);
            _3678[1] = mix(_1843[1][2], _1843[1][3], _2156);
            _3678[2] = mix(_1843[2][2], _1843[2][3], _2156);
            _3678[3] = mix(_1843[3][2], _1843[3][3], _2156);
            _3678[4] = mix(_1843[4][2], _1843[4][3], _2156);
            _3678[5] = mix(_1843[5][2], _1843[5][3], _2156);
            float _4050[6] = _3678;
            float _4077 = _4050[0];
            _4050 = _3678;
            float _5966 = _4050[1];
            _4050 = _3678;
            float _5980 = _4050[2];
            _4050 = _3678;
            float _5994 = _4050[3];
            _4050 = _3678;
            float _6008 = _4050[4];
            _4050 = _3678;
            float _6024 = (((((shCoefScaler[_2191][0] * _4077) + (shCoefScaler[_2191][1] * _5966)) + (shCoefScaler[_2191][2] * _5980)) + (shCoefScaler[_2191][3] * _5994)) + (shCoefScaler[_2191][4] * _6008)) + (shCoefScaler[_2191][5] * _4050[5]);
            float _4053[6] = _3678;
            float _4096 = _4053[0];
            _4053 = _3678;
            float _6036 = _4053[1];
            _4053 = _3678;
            float _6050 = _4053[2];
            _4053 = _3678;
            float _6064 = _4053[3];
            _4053 = _3678;
            float _6078 = _4053[4];
            _4053 = _3678;
            float _4113 = 1.0 - clamp((_6024 - config.kSharpStartY) * config.kSharpScaleY, 0.0, 1.0);
            float _4132 = ((_4113 * config.kSharpLimitScale) + config.kSharpLimitMin) * _6024;
            bool _4164 = _2191 <= 32;
            float _4168 = _4164 ? _3678[0] : _3678[3];
            float _4182 = _4164 ? _3678[2] : _3678[5];
            float _4195 = max(max(_3678[1], _3678[2]), _4168) - min(min(_3678[1], _3678[2]), _4168);
            float _4198 = max(max(_3678[3], _3678[4]), _4182) - min(min(_3678[3], _3678[4]), _4182);
            float _3735 = (_6024 + (min(_4132, max(-_4132, ((((((shCoefUSM[_2191][0] * _4096) + (shCoefUSM[_2191][1] * _6036)) + (shCoefUSM[_2191][2] * _6050)) + (shCoefUSM[_2191][3] * _6064)) + (shCoefUSM[_2191][4] * _6078)) + (shCoefUSM[_2191][5] * _4053[5])) * ((_4113 * config.kSharpStrengthScale) + config.kSharpStrengthMin))) * ((1.0 - clamp(((max(_4195, _4198) / (min(_4195, _4198) + config.kEps)) - config.kMinContrastRatio) * config.kRatioNorm, 0.0, 1.0)) * config.kContrastBoost))) * _1848.x;
            _4779 = _3735;
        }
        else
        {
            _4779 = 0.0;
        }
        float _4794;
        if (_1848.y > 0.0)
        {
            _3681[0] = mix(_1843[2][0], _1843[3][0], _2188);
            _3681[1] = mix(_1843[2][1], _1843[3][1], _2188);
            _3681[2] = mix(_1843[2][2], _1843[3][2], _2188);
            _3681[3] = mix(_1843[2][3], _1843[3][3], _2188);
            _3681[4] = mix(_1843[2][4], _1843[3][4], _2188);
            _3681[5] = mix(_1843[2][5], _1843[3][5], _2188);
            float _4223[6] = _3681;
            float _4250 = _4223[0];
            _4223 = _3681;
            float _6171 = _4223[1];
            _4223 = _3681;
            float _6185 = _4223[2];
            _4223 = _3681;
            float _6199 = _4223[3];
            _4223 = _3681;
            float _6213 = _4223[4];
            _4223 = _3681;
            float _6229 = (((((shCoefScaler[_2159][0] * _4250) + (shCoefScaler[_2159][1] * _6171)) + (shCoefScaler[_2159][2] * _6185)) + (shCoefScaler[_2159][3] * _6199)) + (shCoefScaler[_2159][4] * _6213)) + (shCoefScaler[_2159][5] * _4223[5]);
            float _4226[6] = _3681;
            float _4269 = _4226[0];
            _4226 = _3681;
            float _6241 = _4226[1];
            _4226 = _3681;
            float _6255 = _4226[2];
            _4226 = _3681;
            float _6269 = _4226[3];
            _4226 = _3681;
            float _6283 = _4226[4];
            _4226 = _3681;
            float _4286 = 1.0 - clamp((_6229 - config.kSharpStartY) * config.kSharpScaleY, 0.0, 1.0);
            float _4305 = ((_4286 * config.kSharpLimitScale) + config.kSharpLimitMin) * _6229;
            bool _4337 = _2159 <= 32;
            float _4341 = _4337 ? _3681[0] : _3681[3];
            float _4355 = _4337 ? _3681[2] : _3681[5];
            float _4368 = max(max(_3681[1], _3681[2]), _4341) - min(min(_3681[1], _3681[2]), _4341);
            float _4371 = max(max(_3681[3], _3681[4]), _4355) - min(min(_3681[3], _3681[4]), _4355);
            float _3767 = (_6229 + (min(_4305, max(-_4305, ((((((shCoefUSM[_2159][0] * _4269) + (shCoefUSM[_2159][1] * _6241)) + (shCoefUSM[_2159][2] * _6255)) + (shCoefUSM[_2159][3] * _6269)) + (shCoefUSM[_2159][4] * _6283)) + (shCoefUSM[_2159][5] * _4226[5])) * ((_4286 * config.kSharpStrengthScale) + config.kSharpStrengthMin))) * ((1.0 - clamp(((max(_4368, _4371) / (min(_4368, _4371) + config.kEps)) - config.kMinContrastRatio) * config.kRatioNorm, 0.0, 1.0)) * config.kContrastBoost))) * _1848.y;
            _4794 = _4779 + _3767;
        }
        else
        {
            _4794 = _4779;
        }
        float _4810;
        if (_1848.z > 0.0)
        {
            float _3778 = 0.5 * (_2156 - _2188);
            float _3779 = 0.5 + _3778;
            _3684[1] = mix(_1843[2][1], _1843[1][2], _3779);
            _3684[3] = mix(_1843[3][2], _1843[2][3], _3779);
            _3684[5] = mix(_1843[4][3], _1843[3][4], _3779);
            _3684[0] = mix(_1843[1][1], (_3778 >= 0.0) ? _1843[0][2] : _1843[2][0], abs(_3778));
            _3684[2] = mix(_1843[2][2], (_3778 >= 0.0) ? _1843[1][3] : _1843[3][1], abs(_3778));
            _3684[4] = mix(_1843[3][3], (_3778 >= 0.0) ? _1843[2][4] : _1843[4][2], abs(_3778));
            _3684[6] = mix(_1843[4][4], (_3778 >= 0.0) ? _1843[3][5] : _1843[5][3], abs(_3778));
            float _3861 = _2156 + _2188;
            float _4783;
            if (_3861 >= 1.0)
            {
                _3691[0] = _3684[1];
                _3691[1] = _3684[2];
                _3691[2] = _3684[3];
                _3691[3] = _3684[4];
                _3691[4] = _3684[5];
                _3691[5] = _3684[6];
                _4783 = _3861 - 1.0;
            }
            else
            {
                _3691[0] = _3684[0];
                _3691[1] = _3684[1];
                _3691[2] = _3684[2];
                _3691[3] = _3684[3];
                _3691[4] = _3684[4];
                _3691[5] = _3684[5];
                _4783 = _3861;
            }
            int _3901 = int(_4783 * 64.0);
            float _4396[6] = _3691;
            float _4423 = _4396[0];
            _4396 = _3691;
            float _6416 = _4396[1];
            _4396 = _3691;
            float _6430 = _4396[2];
            _4396 = _3691;
            float _6444 = _4396[3];
            _4396 = _3691;
            float _6458 = _4396[4];
            _4396 = _3691;
            float _6474 = (((((shCoefScaler[_3901][0] * _4423) + (shCoefScaler[_3901][1] * _6416)) + (shCoefScaler[_3901][2] * _6430)) + (shCoefScaler[_3901][3] * _6444)) + (shCoefScaler[_3901][4] * _6458)) + (shCoefScaler[_3901][5] * _4396[5]);
            float _4399[6] = _3691;
            float _4442 = _4399[0];
            _4399 = _3691;
            float _6486 = _4399[1];
            _4399 = _3691;
            float _6500 = _4399[2];
            _4399 = _3691;
            float _6514 = _4399[3];
            _4399 = _3691;
            float _6528 = _4399[4];
            _4399 = _3691;
            float _4459 = 1.0 - clamp((_6474 - config.kSharpStartY) * config.kSharpScaleY, 0.0, 1.0);
            float _4478 = ((_4459 * config.kSharpLimitScale) + config.kSharpLimitMin) * _6474;
            bool _4510 = _3901 <= 32;
            float _4514 = _4510 ? _3691[0] : _3691[3];
            float _4528 = _4510 ? _3691[2] : _3691[5];
            float _4541 = max(max(_3691[1], _3691[2]), _4514) - min(min(_3691[1], _3691[2]), _4514);
            float _4544 = max(max(_3691[3], _3691[4]), _4528) - min(min(_3691[3], _3691[4]), _4528);
            float _3905 = (_6474 + (min(_4478, max(-_4478, ((((((shCoefUSM[_3901][0] * _4442) + (shCoefUSM[_3901][1] * _6486)) + (shCoefUSM[_3901][2] * _6500)) + (shCoefUSM[_3901][3] * _6514)) + (shCoefUSM[_3901][4] * _6528)) + (shCoefUSM[_3901][5] * _4399[5])) * ((_4459 * config.kSharpStrengthScale) + config.kSharpStrengthMin))) * ((1.0 - clamp(((max(_4541, _4544) / (min(_4541, _4544) + config.kEps)) - config.kMinContrastRatio) * config.kRatioNorm, 0.0, 1.0)) * config.kContrastBoost))) * _1848.z;
            _4810 = _4794 + _3905;
        }
        else
        {
            _4810 = _4794;
        }
        float _4812;
        if (_1848.w > 0.0)
        {
            float _3916 = 0.5 * (_2156 + _2188);
            _3695[1] = mix(_1843[3][1], _1843[4][2], _3916);
            _3695[3] = mix(_1843[2][2], _1843[3][3], _3916);
            _3695[5] = mix(_1843[1][3], _1843[2][4], _3916);
            float _3939 = _3916 - 0.5;
            _3695[0] = mix(_1843[4][1], (_3939 >= 0.0) ? _1843[5][2] : _1843[3][0], abs(_3939));
            _3695[2] = mix(_1843[3][2], (_3939 >= 0.0) ? _1843[4][3] : _1843[2][1], abs(_3939));
            _3695[4] = mix(_1843[2][3], (_3939 >= 0.0) ? _1843[3][4] : _1843[1][2], abs(_3939));
            _3695[6] = mix(_1843[1][4], (_3939 >= 0.0) ? _1843[2][5] : _1843[0][3], abs(_3939));
            float _3998 = _2156 - _2188;
            float _3999 = 1.0 + _3998;
            float _4799;
            if (_3999 >= 1.0)
            {
                _3702[0] = _3695[1];
                _3702[1] = _3695[2];
                _3702[2] = _3695[3];
                _3702[3] = _3695[4];
                _3702[4] = _3695[5];
                _3702[5] = _3695[6];
                _4799 = _3998;
            }
            else
            {
                _3702[0] = _3695[0];
                _3702[1] = _3695[1];
                _3702[2] = _3695[2];
                _3702[3] = _3695[3];
                _3702[4] = _3695[4];
                _3702[5] = _3695[5];
                _4799 = _3999;
            }
            int _4039 = int(_4799 * 64.0);
            float _4569[6] = _3702;
            float _4596 = _4569[0];
            _4569 = _3702;
            float _6661 = _4569[1];
            _4569 = _3702;
            float _6675 = _4569[2];
            _4569 = _3702;
            float _6689 = _4569[3];
            _4569 = _3702;
            float _6703 = _4569[4];
            _4569 = _3702;
            float _6719 = (((((shCoefScaler[_4039][0] * _4596) + (shCoefScaler[_4039][1] * _6661)) + (shCoefScaler[_4039][2] * _6675)) + (shCoefScaler[_4039][3] * _6689)) + (shCoefScaler[_4039][4] * _6703)) + (shCoefScaler[_4039][5] * _4569[5]);
            float _4572[6] = _3702;
            float _4615 = _4572[0];
            _4572 = _3702;
            float _6731 = _4572[1];
            _4572 = _3702;
            float _6745 = _4572[2];
            _4572 = _3702;
            float _6759 = _4572[3];
            _4572 = _3702;
            float _6773 = _4572[4];
            _4572 = _3702;
            float _4632 = 1.0 - clamp((_6719 - config.kSharpStartY) * config.kSharpScaleY, 0.0, 1.0);
            float _4651 = ((_4632 * config.kSharpLimitScale) + config.kSharpLimitMin) * _6719;
            bool _4683 = _4039 <= 32;
            float _4687 = _4683 ? _3702[0] : _3702[3];
            float _4701 = _4683 ? _3702[2] : _3702[5];
            float _4714 = max(max(_3702[1], _3702[2]), _4687) - min(min(_3702[1], _3702[2]), _4687);
            float _4717 = max(max(_3702[3], _3702[4]), _4701) - min(min(_3702[3], _3702[4]), _4701);
            float _4043 = (_6719 + (min(_4651, max(-_4651, ((((((shCoefUSM[_4039][0] * _4615) + (shCoefUSM[_4039][1] * _6731)) + (shCoefUSM[_4039][2] * _6745)) + (shCoefUSM[_4039][3] * _6759)) + (shCoefUSM[_4039][4] * _6773)) + (shCoefUSM[_4039][5] * _4572[5])) * ((_4632 * config.kSharpStrengthScale) + config.kSharpStrengthMin))) * ((1.0 - clamp(((max(_4714, _4717) / (min(_4714, _4717) + config.kEps)) - config.kMinContrastRatio) * config.kRatioNorm, 0.0, 1.0)) * config.kContrastBoost))) * _1848.w;
            _4812 = _4810 + _4043;
        }
        else
        {
            _4812 = _4810;
        }
        vec4 _1851 = textureLod(inputTexture, vec2(_2145 * config.kSrcNormX, _2177 * config.kSrcNormY), 0.0);
        vec3 _1853 = vec3(_1851.x, _1851.y, _1851.z);
        float _2324 = ((_5889 * ((((1.0 - _1832.x) - _1832.y) - _1832.z) - _1832.w)) + _4812) - (((0.2125999927520751953125 * _1853.x) + (0.715200006961822509765625 * _1853.y)) + (0.072200000286102294921875 * _1853.z));
        _1851.x += _2324;
        _1851.y += _2324;
        _1851.z += _2324;
        imageStore(out_texture, ivec2(uvec2(vec2(float(_2139), float(_2171)))), _1851);
    }
}
