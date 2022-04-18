# Diligent Engine Vulkan Tutorial
This repo contains a very short but complete example about how you can use the 
[Diligent Engine](https://github.com/DiligentGraphics/DiligentEngine) library to blur an image using a headless
`Vulkan` renderer. The project uses [Conan](https://conan.io/) package manager and `CMake` and should be highly 
reproducible. You will need a relatively modern compiler supporting C++20 as I use some newer features
(e.g. [Designated initializers](https://en.cppreference.com/w/cpp/language/aggregate_initialization)) and see no reason 
to lower this requirement.

Motivation
---
There are at least a handful of different `Vulkan` tutorials out there, including 
[Vulkan Tutorial](https://vulkan-tutorial.com/), 
[Sascha Willems Vulkan](https://github.com/SaschaWillems/Vulkan),
[LunarG Tutorial](https://vulkan.lunarg.com/doc/view/1.2.154.1/windows/tutorial/html/index.html)
and of course official [documentation and samples](https://www.vulkan.org/learn). These are definitely very good 
resources to learn about Vulkan and modern GPUs pipeline in general, but this is a lot to read and digest especially
if you haven't touched any GPU APIs before. Modern GPU APIs like `Vulkan`, `DirectX 12`, `Metal` and `WebGPU` are often 
viewed (not without reason) as **advanced** compared to OpenGL for example, and this immediately repels many, making 
them learn `OpenGL` instead. And while OpenGL looks much simpler on the surface, if you start asking questions like
**why?** or **how?** this particular `glReadPixels` function works on a deeper level, then you face a much more severe 
problem - opaqueness. And to solve this problem you need to play around with the API, read the docs, stackoverflow, etc. 
regardless of the API you have chosen.

But! I will not cover such detailed deep dive here in this tutorial because I'm not an expert and this is not my goal.
My goal here is to give a very simple and extensible example of how to implement a headless Vulkan renderer without
writing 1000+ lines of code and to leave some room for changes and experimentation if you want to. And here is where a
brilliant graphics abstraction layer `Diligent Engine` comes into play.

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
**You can also specify the location where the `.conan` caches directory will be created by setting environment variable
`CONAN_USER_HOME` see [reference](https://docs.conan.io/en/latest/reference/env_vars.html#conan-user-home). 
This may be useful if you have another `.conan` cache and don't want to interfere with it.**

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

And the `BlurFilterApplication` will look like this
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
