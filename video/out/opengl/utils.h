/*
 * This file is part of mpv.
 * Parts based on MPlayer code by Reimar Döffinger.
 *
 * mpv is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * mpv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with mpv.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MP_GL_UTILS_
#define MP_GL_UTILS_

#include "common.h"
#include "math.h"

struct mp_log;

void gl_check_error(GL *gl, struct mp_log *log, const char *info);

void gl_upload_tex(GL *gl, GLenum target, GLenum format, GLenum type,
                   const void *dataptr, int stride,
                   int x, int y, int w, int h);

mp_image_t *gl_read_window_contents(GL *gl, int w, int h);

const char* mp_sampler_type(GLenum texture_target);

// print a multi line string with line numbers (e.g. for shader sources)
// log, lev: module and log level, as in mp_msg()
void mp_log_source(struct mp_log *log, int lev, const char *src);

struct gl_vao_entry {
    // used for shader / glBindAttribLocation
    const char *name;
    // glVertexAttribPointer() arguments
    int num_elems;      // size (number of elements)
    GLenum type;
    bool normalized;
    int offset;
};

struct gl_vao {
    GL *gl;
    GLuint vao;     // the VAO object, or 0 if unsupported by driver
    GLuint buffer;  // GL_ARRAY_BUFFER used for the data
    int stride;     // size of each element (interleaved elements are assumed)
    const struct gl_vao_entry *entries;
};

void gl_vao_init(struct gl_vao *vao, GL *gl, int stride,
                 const struct gl_vao_entry *entries);
void gl_vao_uninit(struct gl_vao *vao);
void gl_vao_bind(struct gl_vao *vao);
void gl_vao_unbind(struct gl_vao *vao);
void gl_vao_draw_data(struct gl_vao *vao, GLenum prim, void *ptr, size_t num);

struct fbotex {
    GL *gl;
    GLuint fbo;
    GLuint texture;
    GLenum iformat;
    GLenum tex_filter;
    int rw, rh; // real (texture) size
    int lw, lh; // logical (configured) size
};

bool fbotex_init(struct fbotex *fbo, GL *gl, struct mp_log *log, int w, int h,
                 GLenum iformat);
void fbotex_uninit(struct fbotex *fbo);
bool fbotex_change(struct fbotex *fbo, GL *gl, struct mp_log *log, int w, int h,
                   GLenum iformat, int flags);
#define FBOTEX_FUZZY_W 1
#define FBOTEX_FUZZY_H 2
#define FBOTEX_FUZZY (FBOTEX_FUZZY_W | FBOTEX_FUZZY_H)
void fbotex_set_filter(struct fbotex *fbo, GLenum gl_filter);
void fbotex_invalidate(struct fbotex *fbo);

// A 3x2 matrix, with the translation part separate.
struct gl_transform {
    // row-major, e.g. in mathematical notation:
    //  | m[0][0] m[0][1] |
    //  | m[1][0] m[1][1] |
    float m[2][2];
    float t[2];
};

static const struct gl_transform identity_trans = {
    .m = {{1.0, 0.0}, {0.0, 1.0}},
    .t = {0.0, 0.0},
};

void gl_transform_ortho(struct gl_transform *t, float x0, float x1,
                        float y0, float y1);

// This treats m as an affine transformation, in other words m[2][n] gets
// added to the output.
static inline void gl_transform_vec(struct gl_transform t, float *x, float *y)
{
    float vx = *x, vy = *y;
    *x = vx * t.m[0][0] + vy * t.m[0][1] + t.t[0];
    *y = vx * t.m[1][0] + vy * t.m[1][1] + t.t[1];
}

struct mp_rect_f {
    float x0, y0, x1, y1;
};

// Semantic equality (fuzzy comparison)
static inline bool mp_rect_f_seq(struct mp_rect_f a, struct mp_rect_f b)
{
    return fabs(a.x0 - b.x0) < 1e-6 && fabs(a.x1 - b.x1) < 1e-6 &&
           fabs(a.y0 - b.y0) < 1e-6 && fabs(a.y1 - b.y1) < 1e-6;
}

static inline void gl_transform_rect(struct gl_transform t, struct mp_rect_f *r)
{
    gl_transform_vec(t, &r->x0, &r->y0);
    gl_transform_vec(t, &r->x1, &r->y1);
}

static inline bool gl_transform_eq(struct gl_transform a, struct gl_transform b)
{
    for (int x = 0; x < 2; x++) {
        for (int y = 0; y < 2; y++) {
            if (a.m[x][y] != b.m[x][y])
                return false;
        }
    }

    return a.t[0] == b.t[0] && a.t[1] == b.t[1];
}

void gl_transform_trans(struct gl_transform t, struct gl_transform *x);

void gl_set_debug_logger(GL *gl, struct mp_log *log);

struct gl_shader_cache;

struct gl_shader_cache *gl_sc_create(GL *gl, struct mp_log *log);
void gl_sc_destroy(struct gl_shader_cache *sc);
bool gl_sc_error_state(struct gl_shader_cache *sc);
void gl_sc_reset_error(struct gl_shader_cache *sc);
void gl_sc_add(struct gl_shader_cache *sc, const char *text);
void gl_sc_addf(struct gl_shader_cache *sc, const char *textf, ...);
void gl_sc_hadd(struct gl_shader_cache *sc, const char *text);
void gl_sc_haddf(struct gl_shader_cache *sc, const char *textf, ...);
void gl_sc_hadd_bstr(struct gl_shader_cache *sc, struct bstr text);
void gl_sc_uniform_sampler(struct gl_shader_cache *sc, char *name, GLenum target,
                           int unit);
void gl_sc_uniform_tex(struct gl_shader_cache *sc, char *name, GLenum target,
                       GLuint texture);
void gl_sc_uniform_tex_ui(struct gl_shader_cache *sc, char *name, GLuint texture);
void gl_sc_uniform_f(struct gl_shader_cache *sc, char *name, GLfloat f);
void gl_sc_uniform_i(struct gl_shader_cache *sc, char *name, GLint f);
void gl_sc_uniform_vec2(struct gl_shader_cache *sc, char *name, GLfloat f[2]);
void gl_sc_uniform_vec3(struct gl_shader_cache *sc, char *name, GLfloat f[3]);
void gl_sc_uniform_mat2(struct gl_shader_cache *sc, char *name,
                        bool transpose, GLfloat *v);
void gl_sc_uniform_mat3(struct gl_shader_cache *sc, char *name,
                        bool transpose, GLfloat *v);
void gl_sc_set_vao(struct gl_shader_cache *sc, struct gl_vao *vao);
void gl_sc_enable_extension(struct gl_shader_cache *sc, char *name);
void gl_sc_generate(struct gl_shader_cache *sc);
void gl_sc_reset(struct gl_shader_cache *sc);
void gl_sc_unbind(struct gl_shader_cache *sc);

struct gl_timer;

struct gl_timer *gl_timer_create(GL *gl);
void gl_timer_free(struct gl_timer *timer);
void gl_timer_start(struct gl_timer *timer);
void gl_timer_stop(struct gl_timer *timer);

int gl_timer_sample_count(struct gl_timer *timer);
uint64_t gl_timer_last_us(struct gl_timer *timer);
uint64_t gl_timer_avg_us(struct gl_timer *timer);
uint64_t gl_timer_peak_us(struct gl_timer *timer);

#define NUM_PBO_BUFFERS 3

struct gl_pbo_upload {
    GL *gl;
    int index;
    GLuint buffers[NUM_PBO_BUFFERS];
    size_t buffer_size;
};

void gl_pbo_upload_tex(struct gl_pbo_upload *pbo, GL *gl, bool use_pbo,
                       GLenum target, GLenum format,  GLenum type,
                       int tex_w, int tex_h, const void *dataptr, int stride,
                       int x, int y, int w, int h);
void gl_pbo_upload_uninit(struct gl_pbo_upload *pbo);

#endif
