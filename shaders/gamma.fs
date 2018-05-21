uniform sampler2D tex;
varying vec2 uv;

uniform float luminance = 256.0;
uniform float gamma_r = 4.0;
uniform float gamma_g = 3.0;
uniform float gamma_b = 1.4;

void main(void) {
    float scale = luminance / 256.0;

    vec4 color = texture(tex, uv);

    color.r = pow(color.r, gamma_r);
    color.g = pow(color.g, gamma_g);
    color.b = pow(color.b, gamma_b);

    color.rgb *= scale;

    gl_FragColor = color;
}
