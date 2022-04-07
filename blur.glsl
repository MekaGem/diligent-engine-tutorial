#ifdef VERTEX_SHADER
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 tex_coords;

layout(location = 0) out vec2 uv;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    uv = tex_coords;
}
#endif

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
            out_color += texture(input_texture, uv + vec2(x, y) * reversed_size.xy) * cf;
            sum += cf;
        }
    }

    out_color /= sum;
}
#endif
