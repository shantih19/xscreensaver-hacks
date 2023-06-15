/* suzannespin, Copyright (c) 2023 shantih19 <shantih19@mail.shantih19.xyz>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#define DEFAULTS	"*delay:	30000       \n" \
			"*showFPS:      False       \n" \
			"*wireframe:    False       \n" \

# define release_suzanne 0

#include "xlockmore.h"
#include "colors.h"
#include "rotator.h"
#include "gllist.h"
#include <ctype.h>

#ifdef USE_GL /* whole file */


extern const struct gllist *suzanne;

typedef struct {
  GLXContext *glx_context;
  rotator *rot;

  GLuint suzanne_list;

  GLfloat pos;

  int ncolors;
  XColor *colors;
  int ccolor;
  int color_shift;

} suzanne_configuration;

static suzanne_configuration *bps = NULL;

ENTRYPOINT ModeSpecOpt suzanne_opts = {0, NULL, 0, NULL, NULL};

/* Window management, etc
 */
ENTRYPOINT void
reshape_suzanne (ModeInfo *mi, int width, int height)
{
  GLfloat h = (GLfloat) height / (GLfloat) width;
  int y = 0;

  if (width > height * 5) {   /* tiny window: show middle */
    height = width * 9/16;
    y = -height/2;
    h = height / (GLfloat) width;
  }

  glViewport (0, y, (GLint) width, (GLint) height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective (30.0, 1/h, 1.0, 5.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt( 0.0, 0.0, 0.0,
             0.0, 0.0, 0.0,
             0.0, 1.0, 0.0);

  {
    GLfloat s = (MI_WIDTH(mi) < MI_HEIGHT(mi)
                 ? (MI_WIDTH(mi) / (GLfloat) MI_HEIGHT(mi))
                 : 1);
    glScalef (s, s, s);
  }

  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

ENTRYPOINT void
suzanne_handle_event (ModeInfo *mi, XEvent *event)
{
  return False;
}

ENTRYPOINT void 
init_suzanne (ModeInfo *mi)
{
  suzanne_configuration *bp;
  int wire = MI_IS_WIREFRAME(mi);

  MI_INIT (mi, bps);
  bp = &bps[MI_SCREEN(mi)];

  bp->glx_context = init_GL(mi);

  if (!wire)
    {
      GLfloat pos[4] = {0.0, 0.28, -1.0, 0.5};
      GLfloat amb[4] = {0.0, 0.0, 0.0, 1.0};
      GLfloat dif[4] = {1.0, 1.0, 1.0, 1.0};
      GLfloat spc[4] = {1.0, 1.0, 1.0, 1.0};

      glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0);
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_CULL_FACE);

      glCullFace(GL_FRONT);
      glShadeModel(GL_FLAT);

      glLightfv(GL_LIGHT0, GL_POSITION, pos);
      glLightfv(GL_LIGHT0, GL_AMBIENT,  amb);
      glLightfv(GL_LIGHT0, GL_DIFFUSE,  dif);
      glLightfv(GL_LIGHT0, GL_SPECULAR, spc);
    }

  {
    double spin_speed   = 6.0;
    double spin_accel   = 0.3;

    bp->rot = make_rotator (0,
                            spin_speed,
                            0,
                            spin_accel,
                            0,
                            False);
  }

  bp->ncolors = 256;
  bp->colors = (XColor *) calloc(bp->ncolors, sizeof(XColor));
  make_smooth_colormap (0, 0, 0,
                        bp->colors, &bp->ncolors,
                        False, 0, False);

  bp->suzanne_list = glGenLists (1);

  glNewList (bp->suzanne_list, GL_COMPILE);
  
  renderList (suzanne, wire);
  glEndList ();
}


ENTRYPOINT void
draw_suzanne (ModeInfo *mi)
{
  suzanne_configuration *bp = &bps[MI_SCREEN(mi)];
  Display *dpy = MI_DISPLAY(mi);
  Window window = MI_WINDOW(mi);
  int c2;

  static const GLfloat bspec[4]  = {1.0, 1.0, 1.0, 1.0};
  static const GLfloat bshiny    = 2.0;

  GLfloat bcolor[3] = {1.0, 1.0, 1.0, 1.0};
  GLfloat scolor[3] = {1.0, 1.0, 1.0, 1.0};

  if (!bp->glx_context)
    return;

  glXMakeCurrent(MI_DISPLAY(mi), MI_WINDOW(mi), *bp->glx_context);

  glShadeModel(GL_FLAT);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_NORMALIZE);
  glEnable(GL_CULL_FACE);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix ();


  {
    double x,y,z;
    get_position(bp->rot, &x, &y, &z, 1);
    glTranslatef(0,0,0);

    get_rotation (bp->rot, &x, &y, &z, 1);
    glRotatef (x * 360, 1.0, 0.0, 0.0);
    glRotatef (y * 360, 0.0, 1.0, 0.0);
    glRotatef (z * 360, 0.0, 0.0, 1.0);
  }

  bcolor[0] = bp->colors[bp->ccolor].red   / 65536.0;
  bcolor[1] = bp->colors[bp->ccolor].green / 65536.0;
  bcolor[2] = bp->colors[bp->ccolor].blue  / 65536.0;

  c2 = (bp->ccolor + bp->color_shift) % bp->ncolors;
  scolor[0] = bp->colors[c2].red   / 65536.0;
  scolor[1] = bp->colors[c2].green / 65536.0;
  scolor[2] = bp->colors[c2].blue  / 65536.0;

  bp->ccolor++;
  if (bp->ccolor >= bp->ncolors) bp->ccolor = 0;

  mi->polygon_count = 0;

  glScalef (0.7, 0.7, 0.7);


  glMaterialfv (GL_FRONT, GL_SPECULAR,            bspec);
  glMateriali  (GL_FRONT, GL_SHININESS,           bshiny);
  glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, bcolor);
  glCallList (bp->suzanne_list);
  mi->polygon_count += (suzanne->points);

  glPopMatrix ();

  if (mi->fps_p) do_fps (mi);
  glFinish();

  glXSwapBuffers(dpy, window);
}


ENTRYPOINT void
free_suzanne (ModeInfo *mi)
{
  suzanne_configuration *bp = &bps[MI_SCREEN(mi)];
  if (!bp->glx_context) return;
  glXMakeCurrent(MI_DISPLAY(mi), MI_WINDOW(mi), *bp->glx_context);
  if (bp->colors) free (bp->colors);
  if (bp->rot) free_rotator (bp->rot);
  if (glIsList(bp->suzanne_list)) glDeleteLists(bp->suzanne_list, 1);
}

XSCREENSAVER_MODULE_2 ("SuzanneSpin", suzannespin, suzanne)

#endif /* USE_GL */
