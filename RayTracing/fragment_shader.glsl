#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

vec3 rotate(vec3 pos, float x, float y, float z) {
    mat3 rotX = mat3(1.0, 0.0, 0.0,
                     0.0, cos(x), -sin(x),
                     0.0, sin(x), cos(x));
    mat3 rotY = mat3(cos(y), 0.0, sin(y),
                     0.0, 1.0, 0.0,
                     -sin(y), 0.0, cos(y));
    mat3 rotZ = mat3(cos(z), -sin(z), 0.0,
                     sin(z), cos(z), 0.0,
                     0.0, 0.0, 1.0);
    return rotZ * rotY * rotX * pos;
}

float hit(vec3 r) {
    r = rotate(r, 0.2, 0.5, 0.0);
    vec3 zn = vec3(r.xyz);
    float rad = 0.0;
    float hit = 0.0;
    float p = 30.0; // detail
    float d = 1.0;
    for(int i = 0; i < 45; i++) {
        rad = length(zn);
        if(rad > 2.0){
            hit = 0.5 * log(rad) * rad / d;
            break;
        } else {
            float th = atan(length(zn.xy), zn.z);
            float phi = atan(zn.y, zn.x);
            float rado = pow(rad, 8.0);
            d = pow(rad, 7.0) * 7.0 * d + 1.0;

            float sint = sin(th * p);
            zn.x = rado * sint * cos(phi * p);
            zn.y = rado * sint * sin(phi * p);
            zn.z = rado * cos(th * p);
        }
    }
    return hit;
}

const vec3 eps = vec3(0.001, 0.0, 0.0);

void main() {
    vec2 pos = fragUV * 2.0 - 1.0;
    pos.x *= 1.0 / 1.0;

    vec3 ro = vec3(pos, -1.2);
    vec3 la = vec3(0.0, 0.0, 1.0);

    vec3 cameraDir = normalize(la - ro);
    vec3 cameraRight = normalize(cross(cameraDir, vec3(0.0, 1.0, 0.0)));
    vec3 cameraUp = normalize(cross(cameraRight, cameraDir));

    vec3 rd = normalize(cameraDir + vec3(pos, 0.0));

    float t = 0.0;
    float d = 200.0;

    vec3 r;
    vec3 color = vec3(0.0);

    for(int i = 0; i < 100; i++) {
        if(d > .001){
            r = ro + rd * t;
            d = hit(r);
            t += d;
        }
    }

    vec3 n = normalize(vec3(hit(r + eps) - hit(r - eps),
                            hit(r + eps.yxz) - hit(r - eps.yxz),
                            hit(r + eps.zyx) - hit(r - eps.zyx)));

    vec3 mat = vec3(.1, .5, .3);
    vec3 light = vec3(.4, .5, -2.0);
    vec3 lightCol = vec3(.6, .3, .5);

    vec3 ldir = normalize(light - r);
    vec3 diff = max(dot(ldir, n), 0.0) * lightCol * 80.0;

    color = diff * mat;

    outColor = vec4(color, 1.0);
}
