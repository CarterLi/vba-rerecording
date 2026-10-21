# Upscaler ports: what was learned adding them to the WebGPU renderer

This is the working knowledge behind the upscaling menu and the three
renderers that fill it, written down after the WebGPU backend got its first
four shaders.  It is meant for whoever adds the next one.

## The shape of the thing

An upscaler is a chain of pixel shader passes over the frame.  Each renderer
keeps a table of them:

| renderer | source | table |
| --- | --- | --- |
| Direct3D 11 | HLSL, ours | `g_upscalers` in `Direct3D11.cpp` |
| OpenGL 3.3 | GLSL, upstream verbatim | `g_libretroShaders` in `OpenGL3.cpp` |
| WebGPU | WGSL, ours | `g_wgpuUpscalers` in `Wgpu.cpp` |

A pass reads the texture the pass before it made - the frame for the first
one - and writes its own.  The last pass writes the back buffer directly
whenever it comes out at the size of the window, so a single pass upscaler
costs one render pass and no blit.

Adding one to the menu is a row in each table, an `RCDATA` resource in
`vba.rc`, a `UPSCALER_*` value, an id in `resource.h` and a `MENUITEM`.
`src/win32/upscalers/README.txt` lists the files and which upstream shader
each came from; keep it in step.

Two rules that are easy to get wrong:

* **The ids and the values are in the same order.**  `MainWndOptions.cpp`
  works the value out arithmetically from the id, so a new upscaler is a
  new id below `ID_OPTIONS_VIDEO_UPSCALER_SHARP_0` and every sharpening and
  max-scale id moves up by one.
* **The enum value goes at the end.**  The settings file holds the number,
  so inserting one in the middle silently changes what everybody's saved
  choice means.  `UPSCALER_LAST` is there so the load-time check does not
  have to be edited every time one is added.

## Porting a shader to WGSL

wgpu compiles WGSL and nothing else, so a port is not a translation of the
GLSL or the HLSL but a third reading of the same upstream file.  The
workflow that worked:

1. **Transpile it first**, for a reference.  `glslangValidator -V` then
   `naga` gets you a compiling WGSL module, which is worth having when the
   hand written one misbehaves.
2. **Write the port by hand**, against the upstream file, keeping its names
   and its order so the two can be read side by side.
3. **Compare the pixels**, not the text.  A frame drawn by both, at 3x, is
   the only check that catches a swapped swizzle.

`.workbuddy-ai/tools/wgsl/` holds the three programs that do this:
`glsl2wgsl.py` (transpile one upstream file), `wgslcheck.cpp` (compile a
WGSL module through the real wgpu-native device and build its pipeline) and
`rendertest.cpp` (draw one frame on the GPU and write it out, plus
`refcheck.py` which does the whole comparison in one command).  They are
not tracked; the README next to them says how to build them.

### What the translator needs, and why

Three rewrites, all of them things Vulkan GLSL or naga demand:

* non-opaque uniforms go into one **unnamed** interface block, or `glslang
  -V` refuses the file.  Unnamed matters: the members stay in scope, so the
  shader keeps reading `TextureSize` rather than `block.TextureSize`;
* `sampler2D` is split into `texture2D` + `sampler`, because WGSL has no
  combined image sampler - and naga's SPIR-V front end cannot read one at
  all (`invalid id %N`, on a five line shader as much as on ours);
* every `texture()` call is rebuilt as `texture(sampler2D(t, s), uv)`.
  GLSL has no three argument `texture(texture, sampler, uv)`; that spelling
  is HLSL and glslang says "no matching overloaded function".

### The traps, in the order they bit

* **naga flips Y by default**, because it assumes Vulkan's clip space.  Pass
  `--keep-coordinate-space`, or the reference comes out upside down and
  every comparison is nonsense.
* **The two stages are translated apart**, so their varyings get locations
  assigned apart: xbr-lv2's vertex shader carries `color` at location 0 and
  its fragment shader starts at 0 with the texcoord.  Shift one side, or
  give both explicit `layout(location)` in the GLSL.
* **The translated vertex shader reads the sizes block**, so binding 2 has
  to be visible to the vertex stage as well.
* **One module cannot hold two `_Sizes`, two uniforms or two `main_1`**, so
  merging the stages means renaming one side onto the other.
* **`bvec4` has no WGSL equivalent.**  Either keep a `vec4` of ones and
  zeros and do and/or/not as arithmetic (what the HLSL ports do), or keep a
  `vec4<bool>` and use `select`.  Do not mix the two conventions in one
  shader.
* **The texcoord is a `vec2` in most files and a `vec4` in the xbr ones**,
  which matters the moment you synthesise one.

### The port itself

* Taps that land on texel centres, or on the nearest texel to a corner, read
  best with `textureLoad` and integer coordinates: `floor(uv * size)`,
  clamped to the edge.  That is exactly what a point sampler gives, without
  the sampler state, and it cannot drift at a texel boundary.
* Taps that upstream takes with `textureOffset` want the same treatment.
* Taps that rely on bilinear filtering (`lanczos2` and its family) need a
  real sampler, and the D3D11 chain table has no per-pass linear flag yet -
  adding one of those means extending the table first.
* Taps upstream computes in its vertex shader are computed in the pixel
  shader here.  The renderer has one quad vertex shader for every upscaler,
  and eight varyings are eight interpolators the pixel shader does not need
  to spend.
* The quad comes out of `@builtin(vertex_index)`; a pass binds no vertex
  buffer and reads no attribute.
* Constants are written once, when the frame, the window or the scale cap
  changes - not per frame.

## Verification, and what it is worth

Two levels, both cheap once set up:

* `wgslcheck` compiles the module through the same device the emulator uses
  and builds the pipeline with the descriptors `prepareUpscaler()` uses.  It
  catches everything up to and including the bind group layout.
* `rendertest` draws a frame.  At 1:1 the output must be the frame back
  (true for resamplers like Lanczos, and for xBR, whose inequations all
  cancel there); at 3x it is compared against the translated version, and
  against a CPU reference where one exists.

The four ports so far all came out at **0/255 in every colour channel**
against the translated shader.  Alpha is not compared: upstream writes
`.rgb` only, and the renderer draws with blending off.

## Colour space: the one that got shipped broken

The frame arrives as bytes the core has already encoded, and Direct3D 11 and
OpenGL hand those bytes to the swap chain untouched.  **A surface format that
encodes on the way out encodes them a second time and washes the picture
out** - lifted blacks, flat contrast, the on screen display included.

`wgpuSurfaceGetCapabilities()` offers `Bgra8UnormSrgb` first on both Vulkan
and Direct3D 12, so the renderer must pick a plain format explicitly rather
than take `formats[0]`.  Measured through an sRGB target: a byte of 8 comes
back as 50, 40 as 110.

If a backend ever offers nothing but sRGB formats, the honest fix is to make
the frame and overlay textures sRGB as well so the sampling decodes - but
that puts the upscalers into linear space, where their thresholds were not
tuned.  Prefer the plain format.

## Menu text that names itself

`setRenderMethodModel()` rewrites the render method entry to name what is
running ("Direct3D 11 (Flip)", "WebGPU (D3D12)").  It reads the current
text, strips the model an earlier update added, and appends the new one -
so **the strip list has to carry every name the function can append**.  It
only knew the three Direct3D presentation suffixes, and the WebGPU entry
read "WebGPU (D3D12) (D3D12) (D3D12)" after three menu updates.  Any
function that rewrites a menu item from its own previous text has this
failure mode; strip until nothing matches, and keep the list complete.
