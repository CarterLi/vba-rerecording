# Upscaler ports: what was learned adding them to the WebGPU renderer

This is the working knowledge behind the upscaling menu and the three
renderers that fill it, written down after the WebGPU backend went from its
first four shaders to the two-texture chains.  It is meant for whoever adds
the next one.

## The shape of the thing

An upscaler is a chain of pixel shader passes over the frame.  Each renderer
keeps a table of them:

| renderer | source | table |
| --- | --- | --- |
| Direct3D 11 | HLSL, ours | `g_upscalers` in `Direct3D11.cpp` |
| OpenGL 3.3 | GLSL, upstream verbatim | `g_libretroShaders` in `OpenGL3.cpp` |
| WebGPU | WGSL, ours | `g_wgpuUpscalers` in `Wgpu.cpp` |

The first two tables share their shape - a list of passes, each naming a
resource, a scale and the output it reads as its second texture.  The WebGPU
one carries the same three things per pass, except that its resource list is
one entry per pass rather than one for the whole chain, because the passes of
a chain are separate WGSL files and not entry points of one module.

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
`refcheck.py` and `refcheck2.py`, which do the comparison in one command).
`refcheckN.py` is the same check for a chain of any length: it takes the
upstream file, the port and the second texture of every pass - `-` for none,
`f` for the frame, `n` for the output of pass n - which is the `seconds` column
of the table, draws every pass at the frame size and the last one at 3x, and
compares each pass as it runs.  They are not tracked; the README next to them
says how to build them.

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

A file with two samplers - PT SkyWalker reads the frame and the raw frame -
takes two slots: 0 and 1 for the first texture and its sampler, 3 and 4 for
the second, with 2 kept for the sizes so that a file with one sampler lands
on the bindings it always had.  The `COMPAT_TEXTURE` macro has to pair each
call with the sampler of the texture it names, and the paste needs the two
step form a C preprocessor always needs: 2xBR reaches its taps through
`#define Source Texture`, and `a##Sampler` pastes the name before it is
expanded, which asks for a `SourceSampler` that does not exist.

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
* **The sizes block is in the order of the upstream file**, not in the order
  the renderer uses.  xbr-lv2 starts with two ints and then the three sizes;
  PT SkyWalker starts with `OutputSize` and carries four of them.  A harness
  that fills the block at fixed offsets silently feeds a shader the wrong
  numbers - `OrigTextureSize` left at zero turns the shadow coordinate into a
  division by zero and samples nowhere.  `refcheck.py` reads the struct the
  translated module declares, works out where each member lands, and hands
  the 64 bytes to `rendertest -c`.
* **`modf()` is the one builtin glslang lowers into a struct return**, which
  naga's SPIR-V front end refuses ("Special type is not registered within the
  module").  2xBR's unpack is written with it; the translator rewrites the
  call into a function of its own with an `inout` parameter, which both
  compilers accept and which means the same for the positive values these
  shaders pass it.
* **A pass whose two stages do not declare the same uniforms** leaves the
  merged module reading a member the vertex stage's block does not have.
  2xBR's second pass takes `OrigTextureSize` only in its pixel stage; the
  patch adds what is missing to the block rather than dropping it.

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
* **Most of the files also nudge the texcoord by 1.0001 in their vertex
  stage** (`TEX0.xy = TexCoord.xy * 1.0001`), and almost none of them care.
  Two do: super-xBR's second pass picks its grid by testing
  `fract(uv * size).x > 0.5`, which at 1:1 is a half exactly, and its jinc
  pass snaps a four by four with `floor(pc - 0.5)`, where a sample sitting on
  a half falls on the wrong side of the floor and the whole window moves one
  texel.  Both ports multiply by 1.0001 themselves, at a cost of nothing, so
  that they agree with the two renderers that compile the nudge.  The others
  leave it out: a tap that is a whole texel from a texel centre reads the
  same texel either way.
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

The eighteen files of the ten chains ported so far all came out at **0/255 in
every colour channel** against the translated shaders.  Alpha is not
compared: upstream writes `.rgb` only, and the renderer draws with blending
off.

PT SkyWalker is the first that reads two textures, and both of them are the
frame: upstream's second one is the raw frame, before RetroArch's palette
stages, which this emulator does not have.  Direct3D 11 binds the frame to
`t1` for it, and the WGSL port reads binding 0 twice rather than carrying a
binding that is always the same texture.

2xBR is the first chain of two.  `refcheck2.py` runs it whole: the translated
first pass makes the flags, both second passes read the flags their own
chain made - the port's reads the port's - and the frames at the end are
compared, so a mistake in either pass shows up.  `rendertest -w` writes the
1:1 frame for that, and `-0` / `-3` take the two textures of the second pass
from files.  Each pass needs its own sizes block - the flags pass runs at the
frame size and the blend pass at the target - so `refcheck2.py` hands
`rendertest -c` one file per pass.

xBR lv3 multipass has the same shape, and its second pass reads the frame at
binding 3 exactly the way 2xBR's does - with `textureLoad`, the filtering
sampler of the layout staying unused.  That the comparison holds anyway is
worth knowing: `rendertest` samples nearest, as the renderer binds
`samplerPoint`, so the port's nearest reads come out equal to the translated
shader's filtered ones on the frame a harness draws.  A filtered tap would
not, and none of these shaders has one.

xBRZ is the first port that is not exact at 1:1: on a frame whose texels have
nothing to do with each other the corner blend fires and mixes a neighbour in,
2/255 of it, and the translated shader does the very same thing - so the check
that means anything there is the 3x one, and both levels came out at 0/255.
Its `scale` comes from the constants block rather than from
`OutputSize * SourceSize.zw` as upstream writes it: the renderer's `scale.xy`
is that product, and at 1x and 3x the two are the same number.

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
