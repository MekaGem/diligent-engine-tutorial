#include <DiligentCore/Common/interface/BasicMath.hpp>
#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h>
#include <DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stb_image.h>
#include <stb_image_write.h>

#include <array>
#include <iostream>
#include <numbers>
#include <string>
#include <vector>

void ERROR(std::string_view message) {
    std::cerr << message << std::endl;
    std::abort();
}

struct Image {
    int32_t width{};
    int32_t height{};
    std::vector<uint8_t> pixels{};

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
};

class BlurFilterApplication {
public:
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
    static constexpr std::string_view texture_uniform{"input_texture"};
    static constexpr auto texture_format = Diligent::TEX_FORMAT_RGBA8_UNORM;
    static constexpr auto transition_mode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

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

    ~BlurFilterApplication() {
        immediate_context->Flush();
    }

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

private:
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
        texture_description.Format = Diligent::TEX_FORMAT_RGBA8_UNORM;
        texture_description.MipLevels = 1;
        texture_description.BindFlags = bind_flags;
        texture_description.Usage = usage;
        texture_description.CPUAccessFlags = cpu_access_flags;
        render_device->CreateTexture(texture_description, texture_data, &texture);

        return texture;
    }

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
        graphics_pipeline_ci.GraphicsPipeline.InputLayout.NumElements = static_cast<uint32_t>(layout_elements.size());

        Diligent::RenderTargetBlendDesc blend_description{};
        blend_description.BlendEnable = false;
        graphics_pipeline_ci.GraphicsPipeline.BlendDesc.RenderTargets[0] = blend_description;

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

        render_device->CreateGraphicsPipelineState(graphics_pipeline_ci, &pipeline_state);

        const auto constants = pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "constants");
        if (!constants) {
            ERROR("Failed to find 'constants' variable");
        }
        constants->Set(constants_buffer);

        pipeline_state->CreateShaderResourceBinding(&shader_resource_binding, true);
    }

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

        immediate_context->SetRenderTargets(1, &render_target, nullptr, transition_mode);

        immediate_context->SetPipelineState(pipeline_state);

        Diligent::IBuffer* buffers[] = {vertex_buffer};
        Diligent::Uint64 offset = 0;
        immediate_context->SetVertexBuffers(
            0,
            1,
            buffers,
            &offset,
            transition_mode,
            Diligent::SET_VERTEX_BUFFERS_FLAG_RESET
        );
        immediate_context->SetIndexBuffer(index_buffer, 0, transition_mode);

        const auto var = shader_resource_binding->GetVariableByName(
            Diligent::SHADER_TYPE_PIXEL,
            texture_uniform.data()
        );
        if (!var) {
            ERROR("Failed to find 'input_texture' variable");
        }
        var->Set(input_texture);

        immediate_context->CommitShaderResources(shader_resource_binding, transition_mode);

        Diligent::DrawIndexedAttribs draw_indexed_attribs{};
        draw_indexed_attribs.NumIndices = static_cast<uint32_t>(indices.size());
        draw_indexed_attribs.IndexType = Diligent::VT_UINT32;
        draw_indexed_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
        immediate_context->DrawIndexed(draw_indexed_attribs);
    }

    void read_pixels(
        Diligent::RefCntAutoPtr<Diligent::ITexture>& texture,
        Diligent::RefCntAutoPtr<Diligent::ITexture>& staging_texture,
        Image& result
    ) {
        // Reset FrameBuffers
        immediate_context->SetRenderTargets(0, nullptr, nullptr, transition_mode);

        Diligent::RefCntAutoPtr<Diligent::IFence> fence{};
        render_device->CreateFence(Diligent::FenceDesc{}, &fence);

        Diligent::CopyTextureAttribs copy_attributes{
            texture,
            transition_mode,
            staging_texture,
            transition_mode,
        };
        immediate_context->CopyTexture(copy_attributes);
        immediate_context->EnqueueSignal(fence, 1);
        immediate_context->Flush();

        // Wait for copying to finish
        fence->Wait(1);

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

    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> render_device{};
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> immediate_context{};
    Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> shader_source_stream_factory{};

    Diligent::RefCntAutoPtr<Diligent::IBuffer> constants_buffer{};
    Diligent::RefCntAutoPtr<Diligent::IBuffer> vertex_buffer{};
    Diligent::RefCntAutoPtr<Diligent::IBuffer> index_buffer{};

    Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state{};
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> shader_resource_binding{};

    uint32_t width{};
    uint32_t height{};
};

int main() {
    BlurFilterApplication blurFilterApplication{};

    Image image{};
    image.load_from_file("fox.jpg");

    auto result = blurFilterApplication.apply(image);
    result.save_to_file("output.jpg");

    return 0;
}
