/*********************************************************************************************************************/
/*                                                  /===-_---~~~~~~~~~------____                                     */
/*                                                 |===-~___                _,-'                                     */
/*                  -==\\                         `//~\\   ~~~~`---.___.-~~                                          */
/*              ______-==|                         | |  \\           _-~`                                            */
/*        __--~~~  ,-/-==\\                        | |   `\        ,'                                                */
/*     _-~       /'    |  \\                      / /      \      /                                                  */
/*   .'        /       |   \\                   /' /        \   /'                                                   */
/*  /  ____  /         |    \`\.__/-~~ ~ \ _ _/'  /          \/'                                                     */
/* /-'~    ~~~~~---__  |     ~-/~         ( )   /'        _--~`                                                      */
/*                   \_|      /        _)   ;  ),   __--~~                                                           */
/*                     '~~--_/      _-~/-  / \   '-~ \                                                               */
/*                    {\__--_/}    / \\_>- )<__\      \                                                              */
/*                    /'   (_/  _-~  | |__>--<__|      |                                                             */
/*                   |0  0 _/) )-~     | |__>--<__|     |                                                            */
/*                   / /~ ,_/       / /__>---<__/      |                                                             */
/*                  o o _//        /-~_>---<__-~      /                                                              */
/*                  (^(~          /~_>---<__-      _-~                                                               */
/*                 ,/|           /__>--<__/     _-~                                                                  */
/*              ,//('(          |__>--<__|     /                  .----_                                             */
/*             ( ( '))          |__>--<__|    |                 /' _---_~\                                           */
/*          `-)) )) (           |__>--<__|    |               /'  /     ~\`\                                         */
/*         ,/,'//( (             \__>--<__\    \            /'  //        ||                                         */
/*       ,( ( ((, ))              ~-__>--<_~-_  ~--____---~' _/'/        /'                                          */
/*     `~/  )` ) ,/|                 ~-_~>--<_/-__       __-~ _/                                                     */
/*   ._-~//( )/ )) `                    ~~-'_/_/ /~~~~~~~__--~                                                       */
/*    ;'( ')/ ,)(                              ~~~~~~~~~~                                                            */
/*   ' ') '( (/                                                                                                      */
/*     '   '  `                                                                                                      */
/*********************************************************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <prof.h>
#include <linalgb.h>
#include <gfxwnd/window.h>
#include <glad/glad.h>
#include <assets/assetload.h>
#include "mainloop.h"

#define WND_WIDTH 1280
#define WND_HEIGHT 720

/*-----------------------------------------------------------------
 * Geometry loading
 *-----------------------------------------------------------------*/
struct mesh_hndl {
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;
    unsigned int indice_count;
};

struct model_hndl {
    struct mesh_hndl* meshes;
    unsigned int num_meshes;
    struct mesh_group** mesh_groups;
    unsigned int num_mesh_groups;
};

static struct model_hndl* model_to_gpu(struct model* m)
{
    struct model_hndl* model = calloc(1, sizeof(struct model_hndl));
    /* Allocate handle memory */
    model->num_meshes = m->num_meshes;
    model->meshes = malloc(m->num_meshes * sizeof(struct mesh_hndl));
    memset(model->meshes, 0, model->num_meshes * sizeof(struct mesh_hndl));

    unsigned int total_verts = 0;
    unsigned int total_indices = 0;
    for (unsigned int i = 0; i < model->num_meshes; ++i) {
        struct mesh* mesh = m->meshes[i];
        struct mesh_hndl* mh = model->meshes + i;

        /* Create vao */
        glGenVertexArrays(1, &mh->vao);
        glBindVertexArray(mh->vao);

        /* Create vertex data vbo */
        glGenBuffers(1, &mh->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, mh->vbo);
        glBufferData(GL_ARRAY_BUFFER,
                mesh->num_verts * sizeof(struct vertex),
                mesh->vertices,
                GL_STATIC_DRAW);

        GLuint pos_attrib = 0;
        glEnableVertexAttribArray(pos_attrib);
        glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (GLvoid*)offsetof(struct vertex, position));
        GLuint uv_attrib = 1;
        glEnableVertexAttribArray(uv_attrib);
        glVertexAttribPointer(uv_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (GLvoid*)offsetof(struct vertex, uvs));
        GLuint nm_attrib = 2;
        glEnableVertexAttribArray(nm_attrib);
        glVertexAttribPointer(nm_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (GLvoid*)offsetof(struct vertex, normal));
        GLuint tn_attrib = 3;
        glEnableVertexAttribArray(tn_attrib);
        glVertexAttribPointer(tn_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (GLvoid*)offsetof(struct vertex, tangent));
        GLuint btn_attrib = 4;
        glEnableVertexAttribArray(btn_attrib);
        glVertexAttribPointer(btn_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (GLvoid*)offsetof(struct vertex, binormal));

        /* Create indice ebo */
        glGenBuffers(1, &mh->ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mh->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                mesh->num_indices * sizeof(GLuint),
                mesh->indices,
                GL_STATIC_DRAW);
        mh->indice_count = mesh->num_indices;

        total_verts += mesh->num_verts;
        total_indices += mesh->num_indices;
    }

    /* Move mesh groups */
    model->mesh_groups = m->mesh_groups;
    model->num_mesh_groups = m->num_mesh_groups;
    m->skeleton = 0;
    m->frameset = 0;
    m->mesh_groups = 0;

    return model;
}

#define TIMED_LOAD_BEGIN(fname) \
    do { \
        printf("Loading: %-85s", fname); \
        timepoint_t t1 = millisecs();

#define TIMED_LOAD_END \
        timepoint_t t2 = millisecs(); \
        printf("[ %3lu ms ]\n", (unsigned long)(t2 - t1)); \
    } while (0);

static struct model_hndl* model_from_file_to_gpu(const char* filename)
{
    /* Load and parse model file */
    struct model* m = 0;
    TIMED_LOAD_BEGIN(filename)
    m = model_from_file(filename);
    TIMED_LOAD_END
    /* Transfer data to gpu */
    struct model_hndl* h = model_to_gpu(m);
    /* Free memory data */
    model_delete(m);
    return h;
}

static void model_free_from_gpu(struct model_hndl* mdlh)
{
    for (unsigned int i = 0; i < mdlh->num_meshes; ++i) {
        struct mesh_hndl* mh = mdlh->meshes + i;
        glDeleteBuffers(1, &mh->ebo);
        glDeleteBuffers(1, &mh->vbo);
        glDeleteVertexArrays(1, &mh->vao);
    }
    for (unsigned int i = 0; i < mdlh->num_mesh_groups; ++i)
        mesh_group_delete(mdlh->mesh_groups[i]);
    free(mdlh->mesh_groups);
    free(mdlh->meshes);
    free(mdlh);
}

/*-----------------------------------------------------------------
 * Shader loading
 *-----------------------------------------------------------------*/
/* Convenience macro */
#define GLSRC(src) "#version 330 core\n" #src

static void gl_check_last_compile_error(GLuint id, const char** err)
{
    /* Check if last compile was successful */
    GLint compileStatus;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
        /* Gather the compile log size */
        GLint logLength;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength != 0) {
            /* Fetch and print log */
            GLchar* buf = malloc(logLength * sizeof(GLchar));
            glGetShaderInfoLog(id, logLength, 0, buf);
            *err = buf;
        }
    }
}

static void gl_check_last_link_error(GLuint id, const char** err)
{
    /* Check if last link was successful */
    GLint status;
    glGetProgramiv(id, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        /* Gather the link log size */
        GLint logLength;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength != 0) {
            /* Fetch and print log */
            GLchar* buf = malloc(logLength * sizeof(GLchar));
            glGetProgramInfoLog(id, logLength, 0, buf);
            *err = buf;
        }
    }
}

#define NSHDR_TYPES 3

static unsigned int shader_from_srcs(const char* vs_src, const char* gs_src, const char* fs_src)
{
    const char* shader_sources[NSHDR_TYPES] = {vs_src, gs_src, fs_src};
    const GLenum shader_types[NSHDR_TYPES]  = {GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER};

    GLuint shader_objs[NSHDR_TYPES] = {};
    GLuint prog = glCreateProgram();

    const char** err = 0;
    for (unsigned int i = 0; i < NSHDR_TYPES; ++i) {
        if (!shader_sources[i])
            continue;
        shader_objs[i] = glCreateShader(shader_types[i]);
        glShaderSource(shader_objs[i], 1, &shader_sources[i], 0);
        glCompileShader(shader_objs[i]);
        gl_check_last_compile_error(shader_objs[i], err);
        if (!err)
            glAttachShader(prog, shader_objs[i]);
        else {
            for (unsigned int j = i; j > 0; --j)
                if (shader_objs[j])
                    glDeleteShader(shader_objs[j]);
            goto shader_error;
        }
    }

    glLinkProgram(prog);
    gl_check_last_link_error(prog, err);
    for (unsigned int i = 0; i < NSHDR_TYPES; ++i)
        if (shader_objs[i])
            glDeleteShader(shader_objs[i]);
    if (!err)
        return prog;

shader_error:
    glDeleteProgram(prog);
    fprintf(stderr, "Shader error: %s", *err);
    free(err);
    return 0;
}

static const char* vs_src = GLSRC(
layout (location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
    gl_Position = proj * view * model * vec4(position, 1.0);
}
);

static const char* fs_src = GLSRC(
out vec4 color;

uniform vec3 col;

void main()
{
    color = vec4(col, 1.0);
}
);

/*-----------------------------------------------------------------
 * App Context
 *-----------------------------------------------------------------*/
struct app_context {
    int* should_terminate;
    struct window* wnd;
    /* GPU Handles */
    struct model_hndl* mdl;
    unsigned int shdr;
};

static void APIENTRY gl_debug_proc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param)
{
    (void) source;
    (void) id;
    (void) severity;
    (void) length;
    (void) user_param;

    if (type == GL_DEBUG_TYPE_ERROR) {
        fprintf(stderr, "%s", message);
        assert(0);
    }
}

static void on_key(struct window* wnd, int key, int scancode, int action, int mods)
{
    (void)scancode; (void)mods;
    struct app_context* ctx = window_get_userdata(wnd);
    if (action == 0 && key == KEY_ESCAPE)
        *(ctx->should_terminate) = 1;
}

static void app_init(struct app_context* ctx)
{
    /* Create window */
    const char* title = "MeshProc";
    int width = WND_WIDTH, height = WND_HEIGHT, mode = 0;
    ctx->wnd = window_create(title, width, height, mode, (struct context_params){OPENGL, {3, 3}, 1});

    /* Assosiate context to be accessed from callback functions */
    window_set_userdata(ctx->wnd, ctx);

    /* Set event callbacks */
    struct window_callbacks wnd_callbacks;
    memset(&wnd_callbacks, 0, sizeof(struct window_callbacks));
    wnd_callbacks.key_cb = on_key;
    window_set_callbacks(ctx->wnd, &wnd_callbacks);

    /* Setup OpenGL debug handler */
    glDebugMessageCallback(gl_debug_proc, ctx);

    /* Load shader */
    ctx->shdr = shader_from_srcs(vs_src, 0, fs_src);

    /* Load model */
    ctx->mdl = model_from_file_to_gpu("ext/cow.ply");
}

static void app_update(void* userdata, float dt)
{
    (void) dt;
    struct app_context* ctx = userdata;

    /* Process input events */
    window_update(ctx->wnd);
}

static void app_render(void* userdata, float dt)
{
    (void) dt;
    struct app_context* ctx = userdata;
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    mat4 proj = mat4_perspective(radians(60.0), 0.1, 300.0, (float)WND_WIDTH / WND_HEIGHT);
    mat4 view = mat4_view_look_at((vec3){{0.0, 5.0, 10.0}}, (vec3){{-1.0, 4.7, 0.0}}, (vec3){{0.0, 1.0, 0.0}});
    float scale = 0.5;
    mat4 model = mat4_scale((vec3){{scale, scale, scale}});

    GLuint shdr = ctx->shdr;
    glUseProgram(shdr);
    glUniformMatrix4fv(glGetUniformLocation(shdr, "proj"), 1, GL_FALSE, proj.m);
    glUniformMatrix4fv(glGetUniformLocation(shdr, "view"), 1, GL_FALSE, view.m);
    glUniformMatrix4fv(glGetUniformLocation(shdr, "model"), 1, GL_FALSE, model.m);
    const struct {
        float col[3];
        GLenum pmode;
        GLfloat ofs_fact;
        GLfloat ofs_units;
    } pass_params[] = {
        {
            .col = {1.0, 1.0, 1.0},
            .pmode = GL_FILL,
            .ofs_fact = 0.0f,
            .ofs_units = 0.0f
        }, {
            .col = {0.4, 0.4, 0.4},
            .pmode = GL_LINE,
            .ofs_fact = -1.0f,
            .ofs_units = 0.0f
        }
    };
    glEnable(GL_POLYGON_OFFSET_LINE);
    for (unsigned int pass = 0; pass < sizeof(pass_params) / sizeof(pass_params[0]); ++pass) {
        glPolygonMode(GL_FRONT_AND_BACK, pass_params[pass].pmode);
        glPolygonOffset(pass_params[pass].ofs_fact, pass_params[pass].ofs_units);
        glUniform3fv(glGetUniformLocation(shdr, "col"), 1, pass_params[pass].col);
        for (unsigned int i = 0; i < ctx->mdl->num_meshes; ++i) {
            struct mesh_hndl* mh = ctx->mdl->meshes + i;
            glBindVertexArray(mh->vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mh->ebo);
            glDrawElements(GL_TRIANGLES, mh->indice_count, GL_UNSIGNED_INT, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
    }
    glDisable(GL_POLYGON_OFFSET_LINE);
    glUseProgram(0);
    window_swap_buffers(ctx->wnd);
}

static void app_shutdown(struct app_context* ctx)
{
    model_free_from_gpu(ctx->mdl);
    glDeleteProgram(ctx->shdr);
    /* Close window */
    window_destroy(ctx->wnd);
}

int main(int argc, char* argv[])
{
    (void) argc;
    (void) argv;

    /* Initialize */
    struct app_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    app_init(&ctx);

    /* Setup mainloop parameters */
    struct mainloop_data mld;
    memset(&mld, 0, sizeof(struct mainloop_data));
    mld.max_frameskip = 5;
    mld.updates_per_second = 60;
    mld.update_callback = app_update;
    mld.render_callback = app_render;
    mld.userdata = &ctx;
    ctx.should_terminate = &mld.should_terminate;

    /* Run mainloop */
    mainloop(&mld);

    /* De-initialize */
    app_shutdown(&ctx);

    return 0;
}
