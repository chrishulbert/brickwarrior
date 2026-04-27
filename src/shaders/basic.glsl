// --- Vertex Shader ---
@vs vs
in vec4 position;
in vec2 texcoord;

out vec2 uv;

void main() {
    gl_Position = position;
    uv = texcoord;
}
@end

// --- Fragment Shader ---
@fs fs
layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;
in vec2 uv;
out vec4 frag_color;

void main() {
    frag_color = texture(sampler2D(tex, smp), uv);
}
@end

// --- Shader Program ---
@program basic vs fs
