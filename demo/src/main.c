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
#include <stdio.h>
#include <assert.h>
#include <gfxwnd/window.h>
#include <glad/glad.h>
#include "mainloop.h"

struct preview_context {
    int* should_terminate;
    struct window* wnd;
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
    struct preview_context* ctx = window_get_userdata(wnd);
    if (action == 0 && key == KEY_ESCAPE)
        *(ctx->should_terminate) = 1;
}

static void preview_init(struct preview_context* ctx)
{
    /* Create window */
    const char* title = "MeshProc";
    int width = 1280, height = 720, mode = 0;
    ctx->wnd = window_create(title, width, height, mode);

    /* Assosiate context to be accessed from callback functions */
    window_set_userdata(ctx->wnd, ctx);

    /* Set event callbacks */
    struct window_callbacks wnd_callbacks;
    memset(&wnd_callbacks, 0, sizeof(struct window_callbacks));
    wnd_callbacks.key_cb = on_key;
    window_set_callbacks(ctx->wnd, &wnd_callbacks);

    /* Setup OpenGL debug handler */
    glDebugMessageCallback(gl_debug_proc, ctx);
}

static void preview_update(void* userdata, float dt)
{
    (void) dt;
    struct preview_context* ctx = userdata;

    /* Process input events */
    window_update(ctx->wnd);
}

static void preview_render(void* userdata, float dt)
{
    (void) dt;
    struct preview_context* ctx = userdata;
    window_swap_buffers(ctx->wnd);
}

static void preview_shutdown(struct preview_context* ctx)
{
    /* Close window */
    window_destroy(ctx->wnd);
}

int main(int argc, char* argv[])
{
    (void) argc;
    (void) argv;

    /* Initialize */
    struct preview_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    preview_init(&ctx);

    /* Setup mainloop parameters */
    struct mainloop_data mld;
    memset(&mld, 0, sizeof(struct mainloop_data));
    mld.max_frameskip = 5;
    mld.updates_per_second = 60;
    mld.update_callback = preview_update;
    mld.render_callback = preview_render;
    mld.userdata = &ctx;
    ctx.should_terminate = &mld.should_terminate;

    /* Run mainloop */
    mainloop(&mld);

    /* De-initialize */
    preview_shutdown(&ctx);

    return 0;
}
