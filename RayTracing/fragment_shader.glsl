#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;
void main() {
    vec2 uv = (fragUV * 2.0) - 1.0;

    // normalize and adjust for ratio
    uv *= .8;

    // initialize some colors
    vec4 color1 = vec4(.4, .6, .7, 1.0);
    vec4 color2 = vec4(.9, .7, .6, 1.0);

    // shade with 2 faux lights
    color1 *= .8 - distance(uv, vec2(-.1, -.1));
    color2 *= .6 - distance(uv, vec2(.25, .3));
    vec4 sphere = color1 + color2;

    // limit edges to circle shape
    float d = distance(uv, vec2(0.0));
    // smooth edges
    float t = 1.0 - smoothstep(.6, .61, d);
    // apply shape to color
    sphere *= t + 0.2 * uv.y;

    // output final color and brighten
    vec4 fragColor = sphere * 1.6;
    outColor = vec4(fragColor.xyz, 1.0);
}
