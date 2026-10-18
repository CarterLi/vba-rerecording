NVIDIA Image Scaling (NIS) SDK

NIS_Config.h   - vendor file, verbatim from the NVIDIA Image Scaling SDK v1.0.3
                 https://github.com/NVIDIAGameWorks/NVIDIAImageScaling
                 (NIS/NIS_Config.h).  Provides NISConfig, NVScalerUpdateConfig()
                 and the coefficient tables the compute shader reads.
licence.txt    - vendor file, verbatim (licence.txt upstream spelling).
NIS_Shader.h   - generated: the SDK's shaders as C++ raw string literals.

The SDK shaders (NIS/NIS_Main.hlsl and NIS/NIS_Scaler.h) are not compiled at
build time.  Direct3D11.cpp compiles NIS_Main.hlsl with D3DCompile and serves
NIS_Scaler.h from NIS_Shader.h through an ID3DInclude, which keeps the
executable self contained (no shader files to ship next to it).

To regenerate NIS_Shader.h, download both .hlsl/.h files from the repository
above and run (PowerShell):

  $main = [IO.File]::ReadAllText("NIS_Main.hlsl")
  $scaler = [IO.File]::ReadAllText("NIS_Scaler.h")
  $h = 'static const char *g_nisMainShader = R"NIS_SHADER(' + "`n" + $main + ')NIS_SHADER";' + "`n`n"
  $h += 'static const char *g_nisScalerShader = R"NIS_SHADER(' + "`n" + $scaler + ')NIS_SHADER";' + "`n"
  [IO.File]::WriteAllText("NIS_Shader.h", $h)

Keep the header comment in NIS_Shader.h when regenerating.

Licence: MIT (see licence.txt).  The shaders keep their own copyright notice
inside the string literals, which is what the licence requires.
