#include <DiligentCore/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h>
#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Common/interface/BasicMath.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stb_image.h>
#include <stb_image_write.h>

#include <iostream>

///
#include <chrono>
///

class Render_Pass {
public:
    struct Constants {
        Diligent::float4x4 model_view{};
    };
    static constexpr uint64_t vertices_count = 4;
    static constexpr uint64_t indices_count = 6;

    Render_Pass(
        const Diligent::RefCntAutoPtr<Diligent::IRenderDevice>& render_device_,
        const Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory>& shader_source_stream_factory_,
        const Diligent::TEXTURE_FORMAT texture_format_
    ) : render_device(render_device_),
        shader_source_stream_factory(shader_source_stream_factory_),
        texture_format(texture_format_) {
        create_buffers();
        create_pipeline();
    }

    void create_buffers() {
        // Create Constants Buffer
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
            vertex_buffer_description.Size = sizeof(float) * vertices_count;
            vertex_buffer_description.BindFlags = Diligent::BIND_VERTEX_BUFFER;
            vertex_buffer_description.Usage = Diligent::USAGE_DYNAMIC;
            vertex_buffer_description.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
            render_device->CreateBuffer(vertex_buffer_description, nullptr, &vertex_buffer);
        }

        // Create Index Buffer
        {
            Diligent::BufferDesc index_buffer_description{};
            index_buffer_description.Name = "Index Buffer";
            index_buffer_description.Size = sizeof(float) * indices_count;
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

        Diligent::RenderTargetBlendDesc blend_description{};
        blend_description.BlendEnable = true;
        blend_description.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
        blend_description.DestBlend = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
        graphics_pipeline_ci.GraphicsPipeline.BlendDesc.RenderTargets[0] = blend_description;

        Diligent::ShaderCreateInfo shader_ci{};
        shader_ci.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
        shader_ci.UseCombinedTextureSamplers = true;
        shader_ci.pShaderSourceStreamFactory = shader_source_stream_factory;

        Diligent::RefCntAutoPtr<Diligent::IShader> vertex_shader{};
        {
            shader_ci.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
            shader_ci.EntryPoint = "main_vs";
            shader_ci.Desc.Name = "Tutorial Vertex Shader";
            // TODO: !!!change name!!!
            shader_ci.FilePath = "test.glsl";
            render_device->CreateShader(shader_ci, &vertex_shader);
        }

        Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader{};
        {
            shader_ci.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
            shader_ci.EntryPoint = "main_ps";
            shader_ci.Desc.Name = "Tutorial Pixel Shader";
            // TODO: !!!change name!!!
            shader_ci.FilePath = "test.glsl";
            render_device->CreateShader(shader_ci, &pixel_shader);
        }

        std::vector<Diligent::LayoutElement> layout_elements = {
            Diligent::LayoutElement{0, 0, 2, Diligent::VT_FLOAT32, false}, // position_xy
            Diligent::LayoutElement{1, 0, 2, Diligent::VT_FLOAT32, false}, // texture_uv
        };
        graphics_pipeline_ci.GraphicsPipeline.InputLayout.LayoutElements = layout_elements.data();
        graphics_pipeline_ci.GraphicsPipeline.InputLayout.NumElements = static_cast<uint32_t>(layout_elements.size());

        graphics_pipeline_ci.pVS = vertex_shader;
        graphics_pipeline_ci.pPS = pixel_shader;

        std::vector<std::string> textures{"input_texture"};
        std::vector<Diligent::ShaderResourceVariableDesc> shader_variable_descriptions{};
        std::vector<Diligent::ImmutableSamplerDesc> immutable_sampler_descriptions{};
        for (const auto& texture_name: textures) {
            shader_variable_descriptions.emplace_back(
                Diligent::SHADER_TYPE_PIXEL,
                texture_name.c_str(),
                Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC
            );

            Diligent::SamplerDesc sampler_description{
                Diligent::FILTER_TYPE_POINT,
                Diligent::FILTER_TYPE_POINT,
                Diligent::FILTER_TYPE_POINT,
                Diligent::TEXTURE_ADDRESS_CLAMP,
                Diligent::TEXTURE_ADDRESS_CLAMP,
                Diligent::TEXTURE_ADDRESS_CLAMP,
            };
            immutable_sampler_descriptions.emplace_back(
                Diligent::SHADER_TYPE_PIXEL,
                texture_name.c_str(),
                sampler_description
            );
        }

        auto& resource_layout = graphics_pipeline_ci.PSODesc.ResourceLayout;
        resource_layout.Variables = shader_variable_descriptions.data();
        resource_layout.NumVariables = static_cast<uint32_t>(shader_variable_descriptions.size());

        resource_layout.ImmutableSamplers = immutable_sampler_descriptions.data();
        resource_layout.NumImmutableSamplers = static_cast<uint32_t>(immutable_sampler_descriptions.size());

        render_device->CreateGraphicsPipelineState(graphics_pipeline_ci, &pipeline_state);

        {
            const auto constants = pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "Constants");
            if (constants) {
                constants->Set(constants_buffer);
            }
        }
        {
            const auto constants = pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "Constants");
            if (constants) {
                constants->Set(constants_buffer);
            }
        }

        pipeline_state->CreateShaderResourceBinding(&shader_resource_binding, true);
    }

    void render(
        const Diligent::RefCntAutoPtr<Diligent::IDeviceContext>& immediate_context,
        Diligent::ITextureView* const render_target
    ) {

    }

private:
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> render_device{};
    Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> shader_source_stream_factory{};
    Diligent::TEXTURE_FORMAT texture_format{};

    Diligent::RefCntAutoPtr<Diligent::IBuffer> constants_buffer{};
    Diligent::RefCntAutoPtr<Diligent::IBuffer> vertex_buffer{};
    Diligent::RefCntAutoPtr<Diligent::IBuffer> index_buffer{};

    Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state{};
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> shader_resource_binding{};
};

//auto create_texture(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> render_device, )

auto load_texture(Diligent::RefCntAutoPtr<Diligent::IRenderDevice>& render_device) {
    Diligent::RefCntAutoPtr<Diligent::ITexture> texture{};

    int32_t w{};
    int32_t h{};
    int32_t channels{};
    auto data = stbi_load("fox.jpg", &w, &h, &channels, 4);
    if (!data) {
        std::cerr << "Failed to load image" << std::endl;
        std::exit(1);
    }

    Diligent::TextureDesc texture_description{};
    texture_description.Type = Diligent::RESOURCE_DIM_TEX_2D;
    texture_description.Width = w;
    texture_description.Height = h;
    texture_description.MipLevels = 1;
    texture_description.Format = Diligent::TEX_FORMAT_RGBA8_UNORM;
    texture_description.BindFlags = Diligent::BIND_SHADER_RESOURCE;
    texture_description.Usage = Diligent::USAGE_IMMUTABLE;
//    texture_description.CPUAccessFlags = Diligent::CPU_ACCESS_READ | Diligent::CPU_ACCESS_WRITE;

    Diligent::TextureSubResData texture_sub_res_data{data, static_cast<Diligent::Uint32>(w * 4)};
    Diligent::TextureData texture_data{&texture_sub_res_data, 1};

    render_device->CreateTexture(texture_description, &texture_data, &texture);

    stbi_image_free(data);

//    internal->texture_view = texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);

//    size = size_;

    return texture;
}

void save_image(
    Diligent::RefCntAutoPtr<Diligent::ITexture>& texture,
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext>& immediate_context,
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice>& render_device
) {
    Diligent::RefCntAutoPtr<Diligent::ITexture> staging_texture{};
    Diligent::RefCntAutoPtr<Diligent::IFence> fence{};

    const auto& description = texture->GetDesc();

    {
        Diligent::TextureDesc texture_description{};
        texture_description.Type = Diligent::RESOURCE_DIM_TEX_2D;
        texture_description.Width = description.Width;
        texture_description.Height = description.Height;
        texture_description.MipLevels = 1;
        texture_description.Format = Diligent::TEX_FORMAT_RGBA8_UNORM;
        texture_description.Usage = Diligent::USAGE_STAGING;
        texture_description.CPUAccessFlags = Diligent::CPU_ACCESS_READ;
        render_device->CreateTexture(texture_description, nullptr, &staging_texture);

        render_device->CreateFence(Diligent::FenceDesc{}, &fence);
        std::cerr << "Fence starting value is: " << fence->GetCompletedValue() << std::endl;
    }

    for (int i = 0; i < 1000; ++i) {
        Diligent::CopyTextureAttribs copy_attributes{
            texture,
            Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
            staging_texture,
            Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
        };
        immediate_context->CopyTexture(copy_attributes);
    }
    immediate_context->EnqueueSignal(fence, 1);
    immediate_context->Flush();
    immediate_context->WaitForIdle();
    render_device->IdleGPU();

    std::cerr << "Fence value before Wait is: " << fence->GetCompletedValue() << std::endl;
    const auto start = std::chrono::high_resolution_clock::now();
    fence->Wait(1);
    const auto end = std::chrono::high_resolution_clock::now();
    std::cerr << "Wait time is: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;
    std::cerr << "Fence value after Wait is: " << fence->GetCompletedValue() << std::endl;

    Diligent::MappedTextureSubresource texture_subresource{};
    immediate_context->MapTextureSubresource(
        staging_texture.RawPtr(),
        0,
        0,
        Diligent::MAP_READ,
        Diligent::MAP_FLAG_NONE,
        nullptr,
        texture_subresource
    );

    if (texture_subresource.Stride != description.Width * 4) {
        std::cerr << "Stride value must be w * 4" << std::endl;
        std::exit(1);
    }

    stbi_write_jpg(
        "output.jpg",
        static_cast<int32_t>(description.Width),
        static_cast<int32_t>(description.Height),
        4,
        texture_subresource.pData,
        100
    );

    immediate_context->UnmapTextureSubresource(staging_texture.RawPtr(), 0, 0);
}

int main() {
    const auto engine_factory = Diligent::GetEngineFactoryVk();

    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> render_device{};
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> immediate_context{};
    Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> shader_source_stream_factory{};

    Diligent::EngineVkCreateInfo engine_ci{};
    engine_factory->CreateDeviceAndContextsVk(engine_ci, &render_device, &immediate_context);
    if (!render_device || !immediate_context) {
        std::cerr << "Failed to CreateDeviceAndContextsVk" << std::endl;
        return 1;
    }

    engine_factory->CreateDefaultShaderSourceStreamFactory(nullptr, &shader_source_stream_factory);
    if (!shader_source_stream_factory) {
        std::cerr << "Failed to CreateDefaultShaderSourceStreamFactory" << std::endl;
        return 1;
    }

    auto texture = load_texture(render_device);
    save_image(texture, immediate_context, render_device);

    Diligent::IFence* x;
//    x->Wait()
//immediate_context->EnqueueSignal()

    immediate_context->Flush();
    return 0;
}
