#include "renderer.hpp"
#include "config.hpp"
#include "map.hpp"

// GLAD must be included BEFORE any OpenGL headers
#include <glad/glad.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

// ---- Shader: Fullscreen ray-traced tilemap lighting ----
static const char *light_vert_src = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;

out vec2 vUV;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vUV = aUV;
}
)";

static const char *light_frag_src = R"(
#version 330 core
in vec2 vUV;

uniform vec2 uResolution;
uniform vec2 uLightPixel;    // Player / light position in window pixels
uniform vec2 uGoalPixel;     // Goal position in window pixels
uniform float uRadius;       // Max visibility radius in pixels
uniform sampler2D uMapTex;   // 480x270 map texture (1 = solid '#', 0 = air '.')

out vec4 fragColor;

bool is_solid(vec2 pos) {
    if (pos.x < 0.0 || pos.x >= uResolution.x || pos.y < 0.0 || pos.y >= uResolution.y) {
        return true;
    }
    vec2 mapUV = pos / uResolution;
    float val = texture(uMapTex, mapUV).r;
    return val > 0.5;
}

void main() {
    // Current pixel coordinate in window space (top-left is 0, 0)
    vec2 pixel = vUV * uResolution;

    vec2 toPixel = pixel - uLightPixel;
    float dist = length(toPixel);

    // If beyond player visibility circle, pitch black
    if (dist > uRadius) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Ray march from player light to pixel to check for shadow occlusion
    vec2 dir = toPixel / max(dist, 0.001);
    bool in_shadow = false;

    float resScale = uResolution.y / 1080.0;
    float stepSize = 8.0 * resScale;
    float travel = 16.0 * resScale;
    while (travel < dist - 6.0 * resScale) {
        vec2 checkPos = uLightPixel + dir * travel;
        if (is_solid(checkPos)) {
            in_shadow = true;
            break;
        }
        travel += stepSize;
    }

    if (in_shadow) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Player light illumination falloff:
    float norm_dist = dist / uRadius;
    float falloff = 1.0 - smoothstep(0.0, 1.0, norm_dist);
    falloff = pow(falloff, 1.3);
    float light_val = 0.70 * falloff;

    // --- Goal black circle influence ---
    // The goal has the same radius of illumination around it, but it creates a black circle / dark void
    float goalDist = length(pixel - uGoalPixel);
    if (goalDist < uRadius) {
        float goalFalloff = 1.0 - smoothstep(0.0, 1.0, goalDist / uRadius);
        goalFalloff = pow(goalFalloff, 1.2);
        // Suppress / darken the light within the goal's black circle aura
        light_val *= (1.0 - goalFalloff * 0.95);
    }

    // Solid platform tiles vs air
    bool solid_target = is_solid(pixel);

    vec3 base_color;
    if (solid_target) {
        base_color = vec3(0.65, 0.65, 0.70); // platform color
    } else {
        base_color = vec3(0.95, 0.95, 0.95); // air / background
    }

    vec3 final_color = base_color * light_val;
    fragColor = vec4(final_color, 1.0);
}
)";

// ---- Shader: Simple box drawing ----
static const char *box_vert_src = R"(
#version 330 core
layout (location = 0) in vec2 aPos;

uniform vec2 uResolution;

void main() {
    vec2 clip = (aPos / uResolution) * 2.0 - 1.0;
    clip.y = -clip.y;
    gl_Position = vec4(clip, 0.0, 1.0);
}
)";

static const char *box_frag_src = R"(
#version 330 core
uniform vec3 uColor;
out vec4 fragColor;

void main() {
    fragColor = vec4(uColor, 1.0);
}
)";

// ---- OpenGL objects ----
static GLuint lightProgram = 0;
static GLuint boxProgram = 0;
static GLuint lightVAO = 0, lightVBO = 0;
static GLuint boxVAO = 0, boxVBO = 0;
static GLuint mapTexture = 0;

// Light shader uniforms
static GLint light_uResolution = -1;
static GLint light_uLightPixel = -1;
static GLint light_uGoalPixel = -1;
static GLint light_uRadius = -1;
static GLint light_uMapTex = -1;

// Box shader uniforms
static GLint box_uResolution = -1;
static GLint box_uColor = -1;

static GLuint compile_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::fprintf(stderr, "Shader compile error: %s\n", log);
        std::exit(1);
    }
    return shader;
}

static GLuint link_program(GLuint vertShader, GLuint fragShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::fprintf(stderr, "Shader link error: %s\n", log);
        std::exit(1);
    }
    return program;
}

static GLuint build_program(const char *vert_src, const char *frag_src) {
    GLuint vert = compile_shader(GL_VERTEX_SHADER, vert_src);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, frag_src);
    GLuint prog = link_program(vert, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

void renderer_update_map() {
    if (mapTexture == 0) return;

    std::vector<unsigned char> texData(MAP_COLS * MAP_ROWS);
    const char *buf = get_map_buffer();
    for (int i = 0; i < MAP_COLS * MAP_ROWS; ++i) {
        texData[i] = (buf[i] == '#') ? 255 : 0;
    }

    glBindTexture(GL_TEXTURE_2D, mapTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MAP_COLS, MAP_ROWS, GL_RED, GL_UNSIGNED_BYTE, texData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void renderer_init() {
    // --- Light program ---
    lightProgram = build_program(light_vert_src, light_frag_src);
    light_uResolution = glGetUniformLocation(lightProgram, "uResolution");
    light_uLightPixel = glGetUniformLocation(lightProgram, "uLightPixel");
    light_uGoalPixel = glGetUniformLocation(lightProgram, "uGoalPixel");
    light_uRadius = glGetUniformLocation(lightProgram, "uRadius");
    light_uMapTex = glGetUniformLocation(lightProgram, "uMapTex");

    // Fullscreen quad
    float lightQuad[] = {
        -1.0f,  1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 1.0f,

        -1.0f,  1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
    };

    glGenVertexArrays(1, &lightVAO);
    glGenBuffers(1, &lightVBO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lightQuad), lightQuad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // --- Box program ---
    boxProgram = build_program(box_vert_src, box_frag_src);
    box_uResolution = glGetUniformLocation(boxProgram, "uResolution");
    box_uColor = glGetUniformLocation(boxProgram, "uColor");

    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(1, &boxVBO);
    glBindVertexArray(boxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- Create 480x270 Map Texture ---
    glGenTextures(1, &mapTexture);
    glBindTexture(GL_TEXTURE_2D, mapTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, MAP_COLS, MAP_ROWS, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    renderer_update_map();
}

void renderer_cleanup() {
    glDeleteTextures(1, &mapTexture);
    glDeleteProgram(lightProgram);
    glDeleteProgram(boxProgram);
    glDeleteVertexArrays(1, &lightVAO);
    glDeleteBuffers(1, &lightVBO);
    glDeleteVertexArrays(1, &boxVAO);
    glDeleteBuffers(1, &boxVBO);
}

void render(SDL_Window *window, game_state const &state) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // --- Pass 1: Draw ray-traced tilemap lighting & visibility ---
    glUseProgram(lightProgram);
    glUniform2f(light_uResolution, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
    glUniform2f(light_uLightPixel, state.player_x, state.player_y);
    glUniform2f(light_uGoalPixel, state.goal_x, state.goal_y);
    glUniform1f(light_uRadius, 700.0f * SCALE); // Light radius

    // Bind map texture to unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mapTexture);
    glUniform1i(light_uMapTex, 0);

    glBindVertexArray(lightVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // --- Pass 2: Draw Goal Box and Player Box ---
    glUseProgram(boxProgram);
    glUniform2f(box_uResolution, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);

    glBindVertexArray(boxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);

    // 1. Draw Goal Box (golden core)
    {
        glUniform3f(box_uColor, 1.0f, 0.84f, 0.0f); // Gold finish point
        float hw = TILE_SIZE_X * 1.0f;
        float hh = TILE_SIZE_Y * 1.0f;
        float gx = state.goal_x;
        float gy = state.goal_y;
        float goalVerts[] = {
            gx - hw, gy - hh,
            gx + hw, gy - hh,
            gx + hw, gy + hh,

            gx - hw, gy - hh,
            gx + hw, gy + hh,
            gx - hw, gy + hh,
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(goalVerts), goalVerts, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // 2. Draw Player Box (same size as a platform block box!)
    {
        glUniform3f(box_uColor, 1.0f, 1.0f, 1.0f); // white player box
        float hw = TILE_SIZE_X * 1.0f;
        float hh = TILE_SIZE_Y * 1.0f;
        float px = state.player_x;
        float py = state.player_y;
        float playerVerts[] = {
            px - hw, py - hh,
            px + hw, py - hh,
            px + hw, py + hh,

            px - hw, py - hh,
            px + hw, py + hh,
            px - hw, py + hh,
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(playerVerts), playerVerts, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);

    SDL_GL_SwapWindow(window);
}

game_state interpolate(game_state const &current, game_state const &previous,
                       float alpha) {
    game_state result;
    result.player_x =
        current.player_x * alpha + previous.player_x * (1.0f - alpha);
    result.player_y =
        current.player_y * alpha + previous.player_y * (1.0f - alpha);
    result.velocity_x = current.velocity_x;
    result.velocity_y = current.velocity_y;
    result.goal_x = current.goal_x;
    result.goal_y = current.goal_y;
    result.level = current.level;
    return result;
}
