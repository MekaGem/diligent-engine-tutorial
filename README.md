# Diligent Engine Vulkan Tutorial
This repo contains a very short but complete example about how you can use the
[Diligent Engine](https://github.com/DiligentGraphics/DiligentEngine) library to blur an image using a headless
`Vulkan` renderer. The project uses [Conan](https://conan.io/) package manager and `CMake` and should be highly
reproducible. You will need a relatively modern compiler supporting C++20 as I use some newer features
(e.g. [Designated initializers](https://en.cppreference.com/w/cpp/language/aggregate_initialization)) and see no reason
to lower this requirement.

Motivation
---
For many years the default cross-platform way to start learning **"how to make your GPU render a triangle"** was to use
OpenGL for desktop and OpenGL ES for mobile devices.
It's still a very good starting point especially considering how many modern GPU features are supported
in newer versions of OpenGL and the enormous number of resources to help you along the way.
But, it's not that cross-platform anymore as Apple deprecated OpenGL across all OSes in 2018, and it's also really painful
to debug and run as a headless renderer.
So, the point of this tutorial is to show how easy it's to make your own application that renders something not super
trivial using Vulkan (without writing 1000+ lines of code) with the help of the Diligent Engine library which gracefully handles all the defaults letting the
user decide whether they want to go deeper.
To spice things up and to make this example more versatile I've also added image loading/saving.
You will see how easy it is with Conan.

And of course, even though I've tried my best to make the code as clean as possible, I'm assuming that the reader of
this tutorial has at least an average knowledge of C++.

If you want to learn more about `Vulkan` there are plenty of resources out there, including
[Vulkan Tutorial](https://vulkan-tutorial.com/),
[Sascha Willems Vulkan](https://github.com/SaschaWillems/Vulkan),
[LunarG Tutorial](https://vulkan.lunarg.com/doc/view/1.2.154.1/windows/tutorial/html/index.html)
and of course official [documentation and samples](https://www.vulkan.org/learn).

Building
---
To build this tutorial you will need:
* Windows, macOS or Linux
* C++20 compiler
* `Python` >= 3.5
* `CMake` >= 3.15
* `Conan` >= 1.33
* Build system like `Visual Studio`, `Make` or `Ninja`

If you don't have Python you can install it from the [official website](https://www.python.org/downloads/) or any other
way you may prefer.  
The same goes for CMake - [official website](https://cmake.org/download/).

To install Conan with Python >= 3.5 you can simply get it via the `pip` by running `pip install conan` or instead
following the [official documentation](https://docs.conan.io/en/latest/installation.html) to use some other method.

**_By default, Conan will create `.conan` cache directory in your user's home directory.
You can also specify the location where the `.conan` cache directory will be created by setting the environment variable
`CONAN_USER_HOME` see [reference](https://docs.conan.io/en/latest/reference/env_vars.html#conan-user-home).
This may be useful if you want to avoid any conflicts with existing cache._**

Now you can download this repo either by using git
```
git clone https://github.com/MekaGem/diligent-engine-tutorial.git
```
or simply by downloading the [repo archive](https://github.com/MekaGem/diligent-engine-tutorial/archive/refs/heads/main.zip).

To build the tutorial itself you need to run the following right inside the repo directory:
* On Windows with `Visual Studio` CMake generator (usually the default option)
```
conan install . --build=missing -if build 
cmake -S . -B build
cmake --build build --config Release
```
* On macOS and Linux using `make` or `ninja`
```
conan install . --build=missing -if build 
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

And finally, you can now run the application at `build\bin\blur.exe` (Windows) or `build/bin/blur` (macOS/Linux) and
check that it works!

Let's start!
---

### Project Configuration

I already mentioned that in this tutorial we are going to use Diligent Engine, but more specifically we are going to
use two Conan dependencies `diligent-core` and `stb` for loading and saving JPEGs.
So here is the contents of the `conanfile.txt`:
```
[requires]
diligent-core/2.5.1
stb/cci.20210910

[generators]
cmake
```
The versions of the libraries refer to the ones in the default conan index https://conan.io/center/ and the generators'
section determines how these libraries can be consumed later, in this case by CMake.

The `CMakeLists.txt` is also quite simple. It specifies minimum required CMake version
```cmake
cmake_minimum_required(VERSION 3.15)
```
name of the project
```cmake
project(diligent_engine_tutorial)
```
C++ standard version
```cmake
set(CMAKE_CXX_STANDARD 20)
```
initializes conan build
```cmake
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()
```
and finally adds an executable and link **all** the Conan libraries to it
```cmake
add_executable(blur main.cpp)
target_link_libraries(blur PRIVATE ${CONAN_LIBS})
target_include_directories(blur PRIVATE ${CONAN_INCLUDE_DIRS})
```
This approach is not very elegant and if you need to have a more fine-grained control over how dependencies are used,
then you may want to explicitly provide configuration with the
[Targets approach](https://docs.conan.io/en/latest/integrations/build_system/cmake/cmake_generator.html#targets-approach)
for example. But this works, and it's simple and good enough for our use case.

That's basically all we need! After running the `conan install` command from above all necessary files for all
the dependencies will be downloaded and built if prebuilt version is missing. And CMake will automagically just have all
these dependencies packed in the `CONAN_LIBS` and `CONAN_INCLUDE_DIRS` variables.

### Application Structure

I like the main function to be as short as reasonably needed, so it should create some application, load image, apply
blur onto it and save the result as an image as well.
```cpp
int main() {
    BlurFilterApplication blurFilterApplication{};

    Image image{};
    image.load_from_file("fox.jpg");

    auto result = blurFilterApplication.apply(image);
    result.save_to_file("output.jpg");

    return 0;
}
```
The cute little fox picture is taken from https://unsplash.com/photos/HQqIOc8oYro and saved as a `fox.jpg` right inside
the repo for convenience. Should you wish to add some arguments parsing or support different image formats, it's
completely up to you! But for now it's `fox.jpg` as an input and `output.jpg` as an output.

So, as a storage for our images we will stick to a simple `Image` struct with two methods `load_from_file` and `save_to_file`
```cpp
struct Image {
    int32_t width{};
    int32_t height{};
    std::vector<uint8_t> pixels{};

    void load_from_file(std::string_view path) {
        // ...
    }

    void save_to_file(std::string_view path) {
        // ...
    }
};
```

And the public part of the `BlurFilterApplication` will look like this
```cpp
class BlurFilterApplication {
public:
    BlurFilterApplication() {
        // ...
    }

    ~BlurFilterApplication() {
        // ...
    }

    Image apply(const Image& image) {
        // ...
    }
};
```

As for the error handling, let's use a classic `std::abort` wrapped in this little function
```cpp
void ERROR(std::string_view message) {
    std::cerr << message << std::endl;
    std::abort();
}
```
so we can easily handle any error we want later in this tutorial. Now let's move on to the implementation of the `Image`
and `BlurFilterApplication`.

### Load/Save JPEG

First, let's deal with the simplest part - loading and saving images.
Thanks to the awesome suite of [STB](https://github.com/nothings/stb)
single-header libraries and its availability in Conan, we can literally implement the `Image` in about 20 lines
of code. Again to keep things simple we assume here, that we always work with JPEG and always convert image to a
4-channel RGBA image practically setting alpha channel to 255 (completely opaque).

To use single-header `stb_image` and `stb_image_write` we need to add their implementation exactly once by defining
`STB_IMAGE_IMPLEMENTATION` and `STB_IMAGE_WRITE_IMPLEMENTATION` respectively before including the headers in one the
source files like this:

```cpp
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stb_image.h>
#include <stb_image_write.h>
```

Saving to a file is basically just forwarding the data to the `stbi_write_jpg` function. The only interesting argument
here is **100** which is the "quality" (size vs image quality tradeoff) parameter and its value must be between 1 and 100.

Loading is a bit more complicated than saving. First, we load the image data, saying that we want the result to have
exactly **4** channels. If you want to get the image as-is, then ignore the last argument and provide a pointer to an `int`
where the actual number of channels will be stored. Second, copy the data to the `Image::pixels`. Third, free
the internal buffer.

```cpp
void load_from_file(std::string_view path) {
    auto data = stbi_load(path.data(), &width, &height, nullptr, 4);
    if (!data) {
        ERROR("Failed to load image");
    }
    pixels.resize(width * height * 4);
    std::memcpy(pixels.data(), data, width * height * 4);
    stbi_image_free(data);
}

void save_to_file(std::string_view path) {
    stbi_write_jpg(path.data(), width, height, 4, pixels.data(), 100);
}
```

That's it. Now we can load `fox.jpg` and save `output.jpg`.

### Engine and Pipeline Initialization

Diligent Engine is a "Modern GPU API Abstraction Layer", meaning it's designed to emphasise modern approach to graphics
programming while still finding the commonalities between Vulkan, Direct3D 12 and Metal, and it also supports OpenGL and
Direct3D 11 to a certain extent. Moreover, Direct3D is considered to be the first class citizen in the Diligent Engine
world.
You can basically write a single HLSL shader and cross-compile it to all supported platforms.
But we will not use this feature and will use only Vulkan and GLSL to SPIR-V builtin compiler
[glslang](https://github.com/KhronosGroup/glslang).

Now, let's proceed to initializing our Application.
```cpp
BlurFilterApplication() {
#if NDEBUG
    Diligent::SetDebugMessageCallback(nullptr);
#endif

    const auto engine_factory = Diligent::GetEngineFactoryVk();

    Diligent::EngineVkCreateInfo engine_ci{};
    engine_factory->CreateDeviceAndContextsVk(engine_ci, &render_device, &immediate_context);
    if (!render_device || !immediate_context) {
        ERROR("Failed to create RenderDevice and Immediate DeviceContext");
    }

    engine_factory->CreateDefaultShaderSourceStreamFactory(nullptr, &shader_source_stream_factory);
    if (!shader_source_stream_factory) {
        ERROR("Failed to create ShaderSourceStreamFactory");
    }

    create_buffers();
    create_pipeline();
}
```
First, we are going to disable all debug messages in Release build. If you want to manually control how logs are processed
then provide an appropriate callback function, otherwise passing `nullptr` will simply mute them.

Next, we need to create an Engine Factory.
This is the first time we specify something related to Vulkan.
This `Diligent::GetEngineFactoryVk` function is for a static linkage, there is also another way to retrieve
engine factory if `diligent-core` is linked dynamically.

Then we need to create three core objects: `RenderDevice`, Immediate `DeviceContext` and a `ShaderSourceStreamFactory`.
* `RenderDevice` - is used for creating textures, buffers, shaders and pipeline states
* `DeviceContext` - is used for configuring current rendering state: e.g. setting frame buffers or transitioning resources
* `ShaderSourceStreamFactory` - is used for loading shaders from files.

There is no need to alter the `Diligent::EngineVkCreateInfo` config, the default values are just fine. By default, the
engine factory will try to create one RenderDevice and one Immediate DeviceContext which is right what we need for our
application. Having more than one DeviceContext may allow the user to push commands in parallel utilizing multiple
processing queues on your GPU, but for now let's do everything **Immediately**.

The first argument of `CreateDefaultShaderSourceStreamFactory` is a list of paths, where to look for shader files, but
the current working directory is included by default.

And the last two lines are `create_buffers();` and `create_pipeline();`. We will cover them later.

All the objects in Diligent Engine are self-managed smart pointers, so you can rely on the default destructor to release
everything for you. But there is one thing to do manually, and it's flushing the command buffer.
```cpp
~BlurFilterApplication() {
    immediate_context->Flush();
}
```
This way the application will not crash while destroying the Immediate DeviceContext.

All three objects we've just created are stored as private members:
```cpp
Diligent::RefCntAutoPtr<Diligent::IRenderDevice> render_device{};
Diligent::RefCntAutoPtr<Diligent::IDeviceContext> immediate_context{};
Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> shader_source_stream_factory{};
```

Let's get back to the `create_buffers` and `create_pipeline` functions.
The `create_buffers` is straightforward, it creates three buffers: `Constants` to store global data passed to
the shader, `Vertex` to store vertex coordinates, and `Index` to store vertex position and texture indices.
What shader is and why it needs these three buffers will be explained better when we start creating the pipeline.
Here is the code:
```cpp
void create_buffers() {
    // Create Constants Buffers
    {
        Diligent::BufferDesc constants_buffer_description{};
        constants_buffer_description.Name = "Constants Buffer";
        constants_buffer_description.Size = sizeof(Constants);
        constants_buffer_description.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
        constants_buffer_description.Usage = Diligent::USAGE_DYNAMIC;
        constants_buffer_description.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        render_device->CreateBuffer(constants_buffer_description, nullptr, &constants_buffer);
    }

    // Create Vertex Buffer
    {
        Diligent::BufferDesc vertex_buffer_description{};
        vertex_buffer_description.Name = "Vertex Buffer";
        vertex_buffer_description.Size = sizeof(Vertex) * vertices.size();
        vertex_buffer_description.BindFlags = Diligent::BIND_VERTEX_BUFFER;
        vertex_buffer_description.Usage = Diligent::USAGE_DYNAMIC;
        vertex_buffer_description.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        render_device->CreateBuffer(vertex_buffer_description, nullptr, &vertex_buffer);
    }

    // Create Index Buffer
    {
        Diligent::BufferDesc index_buffer_description{};
        index_buffer_description.Name = "Index Buffer";
        index_buffer_description.Size = sizeof(uint32_t) * indices.size();
        index_buffer_description.BindFlags = Diligent::BIND_INDEX_BUFFER;
        index_buffer_description.Usage = Diligent::USAGE_DYNAMIC;
        index_buffer_description.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        render_device->CreateBuffer(index_buffer_description, nullptr, &index_buffer);
    }
}
```
As you can see, the code for each of them is quite similar.
The `Name` is used for debug purposes.
`Size` is equal to the number of bytes we want this buffer to store.
`BindFlags` means the point where this data will be used in the rendering pipeline, or in other words the purpose of the
data.
`Usage` is Dynamic because we are going to update all the buffers every time we need to use the pipeline.
And finally, the `CPUAccessFlags` is Write because again we are going to fill this memory by CPU and the driver must
know it has to allow us to do so.
The `nullptr` is provided instead of `BufferData` because we are not going to fill the memory right now.
And, as mentioned above, buffers are created using `RenderDevice` and all three are also stored
as private member fields:
```cpp
Diligent::RefCntAutoPtr<Diligent::IBuffer> constants_buffer{};
Diligent::RefCntAutoPtr<Diligent::IBuffer> vertex_buffer{};
Diligent::RefCntAutoPtr<Diligent::IBuffer> index_buffer{};
```
The last bit left is what are the `Constants`, `vertices` and `indices`?
Here is the code for them as well:
```cpp
struct Constants {
    Diligent::float2 reversed_size{};
    int32_t blur_radius{};
    float sigma{};
};
struct Vertex {
    Diligent::float2 xy{};
    Diligent::float2 uv{};
};
static constexpr std::array<Vertex, 4> vertices = {
    Vertex{.xy = {-1.0f, -1.0f}, .uv = {0.0f, 1.0f}},
    Vertex{.xy = {+1.0f, -1.0f}, .uv = {1.0f, 1.0f}},
    Vertex{.xy = {+1.0f, +1.0f}, .uv = {1.0f, 0.0f}},
    Vertex{.xy = {-1.0f, +1.0f}, .uv = {0.0f, 0.0f}},
};
static constexpr std::array<uint32_t, 6> indices = {
    0, 1, 2,
    2, 3, 0,
};
```
The alignment of the vertices and indices is the following: each vertex has a two-component screen position `x,y` and a
two-component texture position `u,v`.

In Vulkan the coordinate system of the screen is `[-1, +1]` for both `x` and `y`, where the top-left corner is `-1,-1`
and the bottom-right is `+1,+1`.

For textures the coordinate system is `[0, 1]` for both `u` and `v`, where the top-left point is `0,0`
and the bottom-right is `1,1`.

You may notice that the vertices array invert the y-axis between position and texture coordinates, e.g. `xy = -1,-1` and
`uv = 0,1`, but should have been `0,0` instead.
This is because Diligent Engine tries to unify things across multiple APIs.
In Direct3D the screen coordinate system is left-handed and in Vulkan it's right-handed.
And to fix this a Vulkan extension `VK_KHR_MAINTENANCE1` was introduced, see
https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/.
This very extension was
[used](https://github.com/DiligentGraphics/DiligentCore/commit/4ec57381190d632e56d4b56b2a0f1ec7d80d1446)
in the Diligent Engine as well.
As a result the screen coordinate system is y-flipped and texture coordinates must be changed accordingly.

The indices array is just two triplets of vertex indices, each triplet specifies a triangle, so in this case it is:
* `(-1,-1), (+1,-1), (+1,+1)` - first triangle
* `(+1,+1), (-1,+1), (-1,-1)` - second triangle

The `Constants` buffer stores data needed for our blur implementation: radius, sigma and reversed size of the
viewport. This buffer is just a sequence of bytes, but the alignment is important for it. This buffer will be used
the shader with the `std140` memory layout. This layout is strictly defined
[in the glsl specification](https://www.khronos.org/registry/OpenGL/specs/gl/glspec45.core.pdf#page=159)
and that will help us translate bytes to bytes directly.
We will use two rules out of this spec: 32-bit integers and floats have 32-bit alignment, vec2 or vector of two 32-bit
floats has 64-bit alignment. So, in other words this 128 bits of Constants data will be mapped directly to a uniform
struct in the shader.
```glsl
layout(std140) uniform constants
{
    vec2 reversed_size;
    int blur_radius;
    float sigma;
};
```
This `std140` memory layout has its drawbacks, for example arrays of floats are aligned by 128 bits
for each float in array, and unfortunately in Vulkan this is the only reliable memory layout that allows direct
mapping into the uniform buffer struct. You can also use raw buffers with `std430`
(see https://www.khronos.org/opengl/wiki/Interface_Block_(GLSL))
but we won't touch on that in this tutorial.

Ok, now it's time to take a look at the largest method here - `create_pipeline`.
There is a lot of code here, but it's mostly configuration, thus it's verbose but not particularly difficult.
First, we need to configure top-level things:
* The `Name` which is used for debug purposes. Same as for buffers.
* Pipeline type `GRAPHICS`, because we are going to render triangles. Another popular option is `COMPUTE`.
* The topology we are going to use `TRIANGLE_LIST`, as mentioned above it expects triplets of indices.
* How many render targets we are going to use `NumRenderTargets = 1`.
* The expected render target format `RTVFormats[0]` using a predefined constant:  
  `static constexpr auto texture_format = Diligent::TEX_FORMAT_RGBA8_UNORM;`.
* Disable face culling and depth testing.
* Specify that each vertex consists of
    * 0: 2-component 32-bit float position
    * 1: 2-component 32-bit float texture coordinates
* And finally, disable blending as we are not going to use it in this tutorial.
```cpp
void create_pipeline() {
    Diligent::GraphicsPipelineStateCreateInfo graphics_pipeline_ci{};
    graphics_pipeline_ci.PSODesc.Name = "Tutorial Graphics Pipeline";
    graphics_pipeline_ci.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
    graphics_pipeline_ci.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    graphics_pipeline_ci.GraphicsPipeline.NumRenderTargets = 1;
    graphics_pipeline_ci.GraphicsPipeline.RTVFormats[0] = texture_format;
    graphics_pipeline_ci.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
    graphics_pipeline_ci.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;
    
    std::vector<Diligent::LayoutElement> layout_elements = {
        Diligent::LayoutElement{0, 0, 2, Diligent::VT_FLOAT32}, // position
        Diligent::LayoutElement{1, 0, 2, Diligent::VT_FLOAT32}, // texture coordinates
    };
    graphics_pipeline_ci.GraphicsPipeline.InputLayout.LayoutElements = layout_elements.data();
    graphics_pipeline_ci.GraphicsPipeline.InputLayout.NumElements = static_cast<uint32_t>(layout_elements.size())
    
    Diligent::RenderTargetBlendDesc blend_description{};
    blend_description.BlendEnable = false;
    graphics_pipeline_ci.GraphicsPipeline.BlendDesc.RenderTargets[0] = blend_description
```

There are so many way ways to configure this `GraphicsPipeline` and I really encourage you to look at the headers and
documentation to learn what all these options do by yourself.
This and other common types in Diligent Engine really sheds light on many commonalities and differences between various
APIs.

After we configured graphics pipeline we need to compile shaders with the help of already created
`ShaderSourceStreamFactory`.
Despite the fact, that Direct3D and HLSL shader language are considered to be first-class citizens in this library,
we are still allowed to use GLSL which will be directly compiled to SPIR-V. The rendering pipeline consists of several
stages https://en.wikipedia.org/wiki/Graphics_pipeline and only some of them are allowed to alter by the user.
We are going to provide a classic pair of `vertex` and `pixel` shaders and as the `ShaderSourceStreamFactory` provides
some default definitions like `VERTEX_SHADER` and `PIXEL_SHADER` we will combine them together in a single file called
`blur.glsl`.
```cpp
    Diligent::ShaderCreateInfo shader_ci{};
    shader_ci.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_GLSL;
    shader_ci.pShaderSourceStreamFactory = shader_source_stream_factory;
    
    Diligent::RefCntAutoPtr<Diligent::IShader> vertex_shader{};
    {
        shader_ci.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
        shader_ci.Desc.Name = "Tutorial Vertex Shader";
        shader_ci.FilePath = "blur.glsl";
        render_device->CreateShader(shader_ci, &vertex_shader);
        if (!vertex_shader) {
            ERROR("Failed to create vertex shader");
        }
    }
    
    Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader{};
    {
        shader_ci.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
        shader_ci.Desc.Name = "Tutorial Pixel Shader";
        shader_ci.FilePath = "blur.glsl";
        render_device->CreateShader(shader_ci, &pixel_shader);
        if (!pixel_shader) {
            ERROR("Failed to create pixel shader");
        }
    }
    
    graphics_pipeline_ci.pVS = vertex_shader;
    graphics_pipeline_ci.pPS = pixel_shader;
```
Now comes the tricky part.
There are two ways to use a texture in the shader, one is to use a combined texture + sampler, and another is to use
them separately. We are going to stick to the combined option as it's the same as in OpenGL.
In the shader the texture resource is specified like this
```glsl
uniform sampler2D input_texture;
```
and the name `input_texture` will refer to both the static immutable Sampler and the texture uniform.
Sampler is a description of how a given texture must be sampled to retrieve a pixel for a given coordinate, e.g.
what interpolation must be used and what happens when the sampling coordinates are outside the `[0, 1]` range.
So the texture uniform description goes to the `Variable` and the immutable Sampler description goes to
`ImmutableSamplers`. As we only have one of each we can use pointers to the struct instead of arrays.
```cpp
    Diligent::ShaderResourceVariableDesc resource_variable_description{
        Diligent::SHADER_TYPE_PIXEL,
        texture_uniform.data(),
        Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC,
    };
    Diligent::SamplerDesc sampler_description{
        Diligent::FILTER_TYPE_POINT,
        Diligent::FILTER_TYPE_POINT,
        Diligent::FILTER_TYPE_POINT,
        Diligent::TEXTURE_ADDRESS_CLAMP,
        Diligent::TEXTURE_ADDRESS_CLAMP,
        Diligent::TEXTURE_ADDRESS_CLAMP,
    };
    Diligent::ImmutableSamplerDesc immutable_sampler_description{
        Diligent::SHADER_TYPE_PIXEL,
        texture_uniform.data(),
        sampler_description,
    };
    auto& resource_layout = graphics_pipeline_ci.PSODesc.ResourceLayout;
    resource_layout.Variables = &resource_variable_description;
    resource_layout.NumVariables = 1;
    resource_layout.ImmutableSamplers = &immutable_sampler_description;
    resource_layout.NumImmutableSamplers = 1;
```
The `SHADER_TYPE_PIXEL` here means the scope where this variable will be used. In our case we only need this
texture in the pixel shader.
The `FILTER_TYPE_POINT` and `TEXTURE_ADDRESS_CLAMP` mean that we want the texture to be sampled by the nearest pixel
to a given position `uv` and that these `uv` coordinates will be clamped by `[0, 1]` before sampling the pixel.

Phew! We are finally done with the description.
The last step is to create a Pipeline object and also a ShaderResourceBinding object.
The former is a top-level rendering primitive.
You can think of it as a standalone executable program that you can run as many times as you want
if you provide inputs and outputs that match the expectation baked into the configuration.
The latter is a more mind-bending concept. You can read a nice article about it
https://developer.nvidia.com/vulkan-shader-resource-binding
but for us this object is just a way to set non-static variables. Such as the `input_texture` for example!
On the other hand, static variables, are to be set before creating a `ShaderResourceBinding`.
In our case we have only two static variables: `constants` and a Sampler for `input_texture`.
Samplers are initialized based on the `ResourceLayout`. And the `constants` value is still not set.
We are not asked to provide the data for the `constants` fields here, all we need to do, is to specify the buffer object
supposed to hold such data. And when the shader starts, it will basically load and map this whole buffer into the global
`constants` struct. The `true` in the `CreateShaderResourceBinding` asks to initialize all static resources in the
pipeline state. Without that `constants` uniform will still be unset.
```cpp
    render_device->CreateGraphicsPipelineState(graphics_pipeline_ci, &pipeline_state);

    const auto constants = pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "constants");
    if (!constants) {
        ERROR("Failed to find 'constants' variable");
    }
    constants->Set(constants_buffer);
    
    pipeline_state->CreateShaderResourceBinding(&shader_resource_binding, true);
}
```

### The Shader

But wait, there was a shader called `blur.glsl` that we compiled but then just skipped it altogether.
Now when we know what data is provided to it, it will be much easier to explain what it actually does. First, let's look
at the code, here is the vertex shader
```glsl
#ifdef VERTEX_SHADER
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 tex_coords;

layout(location = 0) out vec2 uv;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    uv = tex_coords;
}
#endif
```
and here is the pixel shader
```glsl
#ifdef PIXEL_SHADER
layout(std140) uniform constants
{
    vec2 reversed_size;
    int blur_radius;
    float sigma;
};

const float pi = 3.141592653589793;

uniform sampler2D input_texture;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(0.0);
    float v = 2.0 * sigma.x * sigma.x;
    float sum = 0;

    for (int x = -blur_radius; x <= blur_radius; ++x) {
        for (int y = -blur_radius; y <= blur_radius; ++y) {
            float cf = exp(-(x * x + y * y) / v) / (pi * v);
            out_color += texture(input_texture, uv + vec2(x, y) * reversed_size) * cf;
            sum += cf;
        }
    }

    out_color /= sum;
}
#endif
```
As it was mentioned earlier, the `VERTEX_SHADER` and `PIXEL_SHADER` macros are conveniently provided by the library when
you specify the shader language as `GLSL`. That means, these two sections of the file could have been two different
files instead and nothing would have changed.
However, such a composition when two shader stages are combined in a single file can be very convenient
if you need to share some code between them.

#### Vertex Shader

The vertex shader takes two input parameters `position` and `tex_coords` that we mapped before in the `layout_elements`.
It outputs two parameters as well `gl_Position` and `uv`.
While `gl_Position` is a reserved keyword and variable to store vertex position, the **output** `uv` matches
the other `uv` in the pixel shader **input** parameters, so this is how the vertex shader can provide some extra data
to the pixel shader. And, the main function here is very simple:
```glsl
gl_Position = vec4(position, 0.0, 1.0);
uv = tex_coords;
```
But it's not just assigning one vector to another, it also automagically interpolates `uv` by a given `gl_Position`
(see https://www.khronos.org/opengl/wiki/Fragment_Shader). What this means for us is that now a central pixel in a
triangle will correspond to a central pixel in the triangle on the texture. This way you can stretch texture to a given
shape, in this case just two triangles forming a rectangle.

#### Pixel Shader

Pixel shader is usually the most complicated one. Here we see `constants` uniform, constant `pi`, `input_texture` with
the sampler, input interpolated `uv` and the output `out_color`.
We already covered all of these except for the `pi` and the `out_color`.
`pi` is just a constant, you can define constant values right in the shader code, and they will most likely be inlined.
And `out_color` is mapped to the `layout(location = 0)` which indicates it will be written to the `RTVFormats[0]` that
we set in the pipeline description. The real texture where the color will be stored is determined at runtime, but the
expected format is baked into the pipeline object. As for the algorithm of blur, it's super suboptimal and the formula
may be easily found on the web, but overall the idea is that we have some distribution and the result is a weighted
average of pixels around given position based on this distribution.
One interesting thing is how `reversed_size` is used.
As we already know texture coordinates are `[0, 1]` but for blur we need to iterate over pixels.
To solve this we use reversed size and multiply it by the pixel diff. This trick may be very useful if you need to
convert screen coordinates to and from texture coordinates.

### Applying

We are almost ready to start rendering.
The pipeline is ready to use, but we still need to fill out the buffers and set input and output textures.
First, let's add a helper function to create texture as there will be 3 of them: input texture, render target texture
and staging texture (used for reading the resulting image).

```cpp
Diligent::RefCntAutoPtr<Diligent::ITexture> create_texture(
    Diligent::BIND_FLAGS bind_flags,
    Diligent::USAGE usage,
    Diligent::CPU_ACCESS_FLAGS cpu_access_flags,
    const Diligent::TextureData* texture_data = nullptr
) {
    Diligent::RefCntAutoPtr<Diligent::ITexture> texture{};

    Diligent::TextureDesc texture_description{};
    texture_description.Type = Diligent::RESOURCE_DIM_TEX_2D;
    texture_description.Width = width;
    texture_description.Height = height;
    texture_description.Format = texture_format;
    texture_description.MipLevels = 1;
    texture_description.BindFlags = bind_flags;
    texture_description.Usage = usage;
    texture_description.CPUAccessFlags = cpu_access_flags;
    render_device->CreateTexture(texture_description, texture_data, &texture);

    return texture;
}
```

As textures are basically just buffers, they also must be created with the same Bind, Usage and CPUAccess flags.
These flags will be different for different textures, that's why they are passed as arguments.
The resource type is `RESOURCE_DIM_TEX_2D` because we need a 2D texture.
Width and height will be taken from the input image and used for all three textures.
Format is the same as the one used in the pipeline description.
`MipLevels = 1` means that are not going to use [MipMaps](https://en.wikipedia.org/wiki/Mipmap).
And `texture_data` is either `nullptr` or contains pixel data to initialize the texture with.

Now let's see where this helper function is used. As part of the original API we have a public method
`Image apply(const Image& image)` and here is its implementation:

```cpp
Image apply(const Image& image) {
    width = image.width;
    height = image.height;
    
    Diligent::TextureSubResData texture_sub_res_data{image.pixels.data(), static_cast<uint32_t>(image.width * 4)};
    Diligent::TextureData texture_data{&texture_sub_res_data, 1};
    auto input_texture = create_texture(
        Diligent::BIND_SHADER_RESOURCE,
        Diligent::USAGE_IMMUTABLE,
        Diligent::CPU_ACCESS_NONE,
        &texture_data
    );
    
    auto render_target = create_texture(
        Diligent::BIND_RENDER_TARGET,
        Diligent::USAGE_DEFAULT,
        Diligent::CPU_ACCESS_NONE
    );
    
    auto staging_texture = create_texture(
        Diligent::BIND_NONE,
        Diligent::USAGE_STAGING,
        Diligent::CPU_ACCESS_READ
    );
    
    render(
        input_texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE),
        render_target->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET)
    );
    
    auto result = image;
    read_pixels(render_target, staging_texture, result);
    
    return result;
}
```

As was previously mentioned we need width and height to create textures, so let's just save them as private members.
And then there are three calls to the `create_texture` method.

* `input_texture` - is and immutable (written only once on creation), doesn't require any CPU access as it can't be
  changed, and will be bound as a texture shader resource. In this case `texture_data` is not `nullptr` and contains
  pixels data of the input image.
* `render_target` - will be used as a Render Target, mostly accessed by GPU (Default), and we won't access its data from
  the CPU at all.
* `staging_texture` - is only used to read pixels data, so it won't be bound to anything, used only as a staging texture
  and will be read by CPU.

After all three textures are created we can acquire appropriate views for input and render target textures and bind them
to the pipeline in the `render` method. After pushing rendering commands we are going to schedule a copying
from `render_target` into `staging_texture`, read pixels data back and return them as an Image.
Texture View is a way to interpret the Texture buffer as a something else, e.g. render target.
This is essential because texture buffer may be in various states as well as behave as different types of attachments.
For example, the same texture may have both `BIND_RENDER_TARGET` and `BIND_SHADER_RESOURCE` flags but can't be attached
as both a render target and a shader resource at the same time and usually even transitioning between different
states is costly.

### Rendering

The `render` function like a pipeline description is verbose but not very difficult.
First step is to fill out the buffers, here `Diligent::MapHelper` comes to help.
All buffers must be mapped and unmapped properly to change their data. Plus, updating the buffer doesn't mean the GPU
automatically sees updated data. MapHelper will solve the first problem by gracefully wrapping mapping and unmapping
logic leaving the user only with data manipulation. 

```cpp
void render(Diligent::ITextureView* input_texture, Diligent::ITextureView* render_target) {
    {
        Diligent::MapHelper<Constants> map_helper{
            immediate_context,
            constants_buffer,
            Diligent::MAP_WRITE,
            Diligent::MAP_FLAG_DISCARD,
        };
        map_helper->reversed_size = {1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height)};
        map_helper->blur_radius = 50;
        map_helper->sigma = 25.0;
    }

    {
        Diligent::MapHelper<std::array<Vertex, 4>> map_helper{
            immediate_context,
            vertex_buffer,
            Diligent::MAP_WRITE,
            Diligent::MAP_FLAG_DISCARD,
        };
        *map_helper = vertices;
    }

    {
        Diligent::MapHelper<std::array<uint32_t, 6>> map_helper{
            immediate_context,
            index_buffer,
            Diligent::MAP_WRITE,
            Diligent::MAP_FLAG_DISCARD,
        };
        *map_helper = indices;
    }

    // ...
}
```
Flags `MAP_WRITE` and `MAP_FLAG_DISCARD` indicate that we are going to map buffers for writing and previous content is
irrelevant and can be discarded by the GPU for optimization.
MapHelper also provides some convenient operators, so you can use it as a pointer to the template type.
Next we are going to set render targets, and in our case it's only one texture view `render_target`:
```cpp
    immediate_context->SetRenderTargets(1, &render_target, nullptr, transition_mode);
```
The `nullptr` stands for no depth/stencil view. And `transition_mode` is a constant equal to
`Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION` which we will use for our operations, and it means that all
resource transitions will happen synchronously.
Resource state transitioning is something almost non-existent in older APIs like OpenGL, where such things happen
under the hood controlled by the driver.
And `RESOURCE_STATE_TRANSITION_MODE_TRANSITION` is a nice default way to explicitly say that you just want all the
resources to be transitioned to the required states before running next operation. 

Then set the current pipeline state object
```cpp
    immediate_context->SetPipelineState(pipeline_state);
```
and also set vertex and index buffers
```cpp
    immediate_context->SetVertexBuffers(
        0,
        1,
        &vertex_buffer,
        nullptr,
        transition_mode,
        Diligent::SET_VERTEX_BUFFERS_FLAG_RESET
    );
    immediate_context->SetIndexBuffer(index_buffer, 0, transition_mode);
```

Now, we need to set `input_texture` to the shader variable, `shader_resource_binding` can find variables by location or
by the name.
```cpp
    const auto var = shader_resource_binding->GetVariableByName(
        Diligent::SHADER_TYPE_PIXEL,
        texture_uniform.data()
    );
    if (!var) {
        ERROR("Failed to find 'input_texture' variable");
    }
    var->Set(input_texture);
    
    immediate_context->CommitShaderResources(shader_resource_binding, transition_mode);
```
Additionally, call `CommitShaderResources` so that `immediate_context` can properly update changed variables in the
current pipeline state object.

And finally, push a render command.
`NumIndices` must be a multiple of 3 and the `IndexType` must match our index buffer content.
`DRAW_FLAG_VERIFY_ALL` flag tell Diligent Engine that you want to verify everything possible in this draw call.
You may consider using `DRAW_FLAG_NONE` for performance reasons.
```cpp
    Diligent::DrawIndexedAttribs draw_indexed_attribs{};
    draw_indexed_attribs.NumIndices = static_cast<uint32_t>(indices.size());
    draw_indexed_attribs.IndexType = Diligent::VT_UINT32;
    draw_indexed_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
    immediate_context->DrawIndexed(draw_indexed_attribs);
}
```
That's it, the render command it issued and at some point of time in the future it should finish and write resulting
pixels into the `render_target` texture.
But GPU works in parallel with CPU, so we need to do one more synchronized copy to know when it's time to read
pixels data.

### Read Pixels

There is a function in OpenGL named `glReadPixels` and it's one of the most complicated parts of the API.
And the reason is that it both "waits" for the resource to be ready and then reads its pixel data.
In Vulkan we need to copy the texture to a place from where it can be read, explicitly wait for copy to finish and
only after that map the memory for reading.

There is one more primitive we will need for synchronization - Fence.
There are two types of synchronization primitives in Vulkan: Fences and Semaphores.
Semaphores are used to add order between GPU and GPU operations.
Fences are used to add order between GPU and CPU operations.
In our case we are going to use a Fence since we only need to know when copying is finished by GPU to start reading by CPU.
Fence is basically an only-increasing integer which GPU can increase and CPU can wait for until it's greater or equal
a given value.
But first let's explicitly reset frame buffers.
```cpp
void read_pixels(
        Diligent::RefCntAutoPtr<Diligent::ITexture>& texture,
        Diligent::RefCntAutoPtr<Diligent::ITexture>& staging_texture,
        Image& result
) {
    // Reset FrameBuffers
    immediate_context->SetRenderTargets(0, nullptr, nullptr, transition_mode);
    
    // ...
```
This happens implicitly when copying operation is issued, but it generates warnings so let's do it manually instead.

Then, create a Fence:
```cpp
    Diligent::RefCntAutoPtr<Diligent::IFence> fence{};
    render_device->CreateFence(Diligent::FenceDesc{}, &fence);
```

Issue a copy command from `render_target` to `staging_texture`:
```cpp
    Diligent::CopyTextureAttribs copy_attributes{
        texture,
        transition_mode,
        staging_texture,
        transition_mode,
    };
    immediate_context->CopyTexture(copy_attributes);
```

And right after that add synchronization logic:
```cpp
    immediate_context->EnqueueSignal(fence, 1);
    immediate_context->Flush();
```
The `EnqueueSignal` add a command right after the previous submitted one that changes the value of the Fence to the
value provided.
And `Flush` is required before you start waiting, because without `Flush` you may end up in a deadlock situation, where
you issued a command, but it wasn't actually pushed to the GPU and you are now indefinitely waiting for it finish.

So right after the `Flush` we are waiting for the Fence to become 1:
```cpp
    // Wait for copying to finish
    fence->Wait(1);
```
And finally we map the buffer, read pixels data, copy it into the resulting `Image` and unmap the buffer back:
```cpp
    Diligent::MappedTextureSubresource texture_subresource{};
    immediate_context->MapTextureSubresource(
        staging_texture.RawPtr(),
        0,
        0,
        Diligent::MAP_READ,
        Diligent::MAP_FLAG_DO_NOT_WAIT,
        nullptr,
        texture_subresource
    );

    if (texture_subresource.Stride != width * 4) {
        ERROR("Stride value must be equal to width * 4");
    }
    std::memcpy(result.pixels.data(), texture_subresource.pData, width * height * 4);

    immediate_context->UnmapTextureSubresource(staging_texture.RawPtr(), 0, 0);
}
```

Ta-da! This was the final piece of this tutorial.
The application is now capable of blurring JPEG files and may be run even on a headless server with GPU
(or even without GPU if you installed some fake CPU Vulkan drivers, for example [Mesa](https://www.mesa3d.org/)
has CPU-based Vulkan drivers).

I hope you enjoyed reading it and if you have any questions, fixes or other suggestions please reach out to me at `mekagem@gmail.com`.