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
```
cmake_minimum_required(VERSION 3.15)
```
name of the project
```
project(diligent_engine_tutorial)
```
C++ standard version
```
set(CMAKE_CXX_STANDARD 20)
```
initializes conan build
```
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()
```
and finally adds an executable and link **all** the Conan libraries to it
```
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

### Engine Initialization

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
