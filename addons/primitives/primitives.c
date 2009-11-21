/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Core primitive addon functions.
 *
 *
 *      By Pavel Sountsov.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro5.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/internal/aintern_prim_opengl.h"
#include "allegro5/internal/aintern_prim_directx.h"
#include "allegro5/platform/alplatf.h"
#include "allegro5/internal/aintern_prim_soft.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_prim.h"
#include "allegro5/internal/aintern.h"
#include <math.h>

#ifdef ALLEGRO_CFG_OPENGL
#include "allegro5/allegro_opengl.h"
#endif
#ifdef ALLEGRO_CFG_D3D
#include "allegro5/allegro_direct3d.h"
#endif

/*
TODO: This is a hack... I need to know the values of these without actually including the respective system headers
*/
#ifndef ALLEGRO_OPENGL
#define ALLEGRO_OPENGL   4
#endif
#ifndef ALLEGRO_DIRECT3D
#define ALLEGRO_DIRECT3D 8
#endif

static void temp_trans(float x, float y)
{
   ALLEGRO_TRANSFORM copy;
   al_copy_transform(al_get_current_transform(), &copy);
   al_translate_transform(&copy, x, y);
   al_use_transform(&copy);
}

/* Function: al_draw_prim
 */
int al_draw_prim(const void* vtxs, const ALLEGRO_VERTEX_DECL* decl,
   ALLEGRO_BITMAP* texture, int start, int end, int type)
{  
   ALLEGRO_BITMAP *target;
   int ret = 0;
 
   ASSERT(vtxs);
   ASSERT(end >= start);
   ASSERT(type >= 0 && type < ALLEGRO_PRIM_NUM_TYPES);

   target = al_get_target_bitmap();

   /* In theory, if we ever get a camera concept for this addon, the transformation into
    * view space should occur here
    */
   
   if (target->parent) {
      /*
       * For sub-bitmaps
       */
      temp_trans((float)target->xofs, (float)target->yofs);
   }
   
   if (target->flags & ALLEGRO_MEMORY_BITMAP || (texture && texture->flags & ALLEGRO_MEMORY_BITMAP)) {
      ret =  _al_draw_prim_soft(texture, vtxs, decl, start, end, type);
   } else {
      int flags = al_get_display_flags();
      if (flags & ALLEGRO_OPENGL) {
         ret =  _al_draw_prim_opengl(texture, vtxs, decl, start, end, type);
      } else if (flags & ALLEGRO_DIRECT3D) {
         ret =  _al_draw_prim_directx(texture, vtxs, decl, start, end, type);
      }
   }
   
   if (target->parent) {
      /*
       * Move it back, if rounding errors become an issue we can do a copy-restore instead
       */
      temp_trans(-(float)target->xofs, -(float)target->yofs);
   }
   
   return ret;
}

/* Function: al_draw_indexed_prim
 */
int al_draw_indexed_prim(const void* vtxs, const ALLEGRO_VERTEX_DECL* decl,
   ALLEGRO_BITMAP* texture, const int* indices, int num_vtx, int type)
{
   ALLEGRO_BITMAP *target;
   int ret = 0;
 
   ASSERT(vtxs);
   ASSERT(indices);
   ASSERT(num_vtx > 0);
   ASSERT(type >= 0 && type < ALLEGRO_PRIM_NUM_TYPES);

   target = al_get_target_bitmap();
   
   /* In theory, if we ever get a camera concept for this addon, the transformation into
    * view space should occur here
    */
   
   if (target->parent) {
      /*
       * For sub-bitmaps
       */
      temp_trans((float)target->xofs, (float)target->yofs);
   }
   
   if (target->flags & ALLEGRO_MEMORY_BITMAP || (texture && texture->flags & ALLEGRO_MEMORY_BITMAP)) {
      ret =  _al_draw_prim_indexed_soft(texture, vtxs, decl, indices, num_vtx, type);
   } else {
      int flags = al_get_display_flags();
      if (flags & ALLEGRO_OPENGL) {
         ret =  _al_draw_prim_indexed_opengl(texture, vtxs, decl, indices, num_vtx, type);
      } else if (flags & ALLEGRO_DIRECT3D) {
         ret =  _al_draw_prim_indexed_directx(texture, vtxs, decl, indices, num_vtx, type);
      }
   }
   
   if (target->parent) {
      /*
       * Move it back, if rounding errors become an issue we can do a copy-restore instead
       */
      temp_trans(-(float)target->xofs, -(float)target->yofs);
   }
   
   return ret;
}

/* Function: al_get_allegro_color
 */
ALLEGRO_COLOR al_get_allegro_color(ALLEGRO_PRIM_COLOR col)
{
   return al_map_rgba_f(col.r, col.g, col.b, col.a);
}

/* Function: al_get_prim_color
 */
ALLEGRO_PRIM_COLOR al_get_prim_color(ALLEGRO_COLOR col)
{
   ALLEGRO_PRIM_COLOR ret;
   ret.r = col.r;
   ret.g = col.g;
   ret.b = col.b;
   ret.a = col.a;
#ifdef ALLEGRO_CFG_D3D
   ret.d3d_color = D3DCOLOR_COLORVALUE(col.r, col.g, col.b, col.a);
#else
   ret.d3d_color = 0;
#endif
   return ret;
}

int _al_bitmap_region_is_locked(ALLEGRO_BITMAP* bmp, int x1, int y1, int w, int h)
{
   ASSERT(bmp);
   
   if (!al_is_bitmap_locked(bmp))
      return 0;
   if (x1 + w > bmp->lock_x && y1 + h > bmp->lock_y && x1 < bmp->lock_x + bmp->lock_w && y1 < bmp->lock_y + bmp->lock_h)
      return 1;
   return 0;
}

/* Function: al_get_allegro_primitives_version
 */
uint32_t al_get_allegro_primitives_version(void)
{
   return ALLEGRO_VERSION_INT;
}

/* Function: al_create_vertex_decl
 */
ALLEGRO_VERTEX_DECL* al_create_vertex_decl(const ALLEGRO_VERTEX_ELEMENT* elements, int stride)
{
   ALLEGRO_VERTEX_DECL* ret = malloc(sizeof(ALLEGRO_VERTEX_DECL));
   ret->elements = malloc(sizeof(ALLEGRO_VERTEX_ELEMENT) * ALLEGRO_PRIM_ATTR_NUM);
   memset(ret->elements, 0, sizeof(ALLEGRO_VERTEX_ELEMENT) * ALLEGRO_PRIM_ATTR_NUM);
   while(elements->attribute) {
      ret->elements[elements->attribute] = *elements;
      elements++;
   }
   
#ifdef ALLEGRO_CFG_D3D
   {
      int flags = al_get_display_flags();
      if (flags & ALLEGRO_DIRECT3D) {
         ALLEGRO_DISPLAY *display;
         LPDIRECT3DDEVICE9 device;
         D3DVERTEXELEMENT9 d3delements[ALLEGRO_PRIM_ATTR_NUM + 1];
         int idx = 0;
         ALLEGRO_VERTEX_ELEMENT* e;
         D3DCAPS9 caps;
         
         display = al_get_current_display();
         device = al_d3d_get_device(display);
         
         IDirect3DDevice9_GetDeviceCaps(device, &caps);
         if(caps.PixelShaderVersion < D3DPS_VERSION(3, 0)) {
            ret->d3d_decl = 0;
         } else {
            e = &ret->elements[ALLEGRO_PRIM_POSITION];
            if(e->attribute) {
               int type = 0;
               switch(e->storage) {
                  case ALLEGRO_PRIM_FLOAT_2:
                     type = D3DDECLTYPE_FLOAT2;
                  break;
                  case ALLEGRO_PRIM_FLOAT_3:
                     type = D3DDECLTYPE_FLOAT3;
                  break;
                  case ALLEGRO_PRIM_SHORT_2:
                     type = D3DDECLTYPE_SHORT2;
                  break;
               }
               d3delements[idx].Stream = 0;
               d3delements[idx].Offset = e->offset;
               d3delements[idx].Type = type;
               d3delements[idx].Method = D3DDECLMETHOD_DEFAULT;
               d3delements[idx].Usage = D3DDECLUSAGE_POSITION;
               d3delements[idx].UsageIndex = 0;
               idx++;
            }

            e = &ret->elements[ALLEGRO_PRIM_TEX_COORD];
            if(!e->attribute)
               e = &ret->elements[ALLEGRO_PRIM_TEX_COORD_PIXEL];
            if(e->attribute) {
               int type = 0;
               switch(e->storage) {
                  case ALLEGRO_PRIM_FLOAT_2:
                  case ALLEGRO_PRIM_FLOAT_3:
                     type = D3DDECLTYPE_FLOAT2;
                  break;
                  case ALLEGRO_PRIM_SHORT_2:
                     type = D3DDECLTYPE_SHORT2;
                  break;
               }
               d3delements[idx].Stream = 0;
               d3delements[idx].Offset = e->offset;
               d3delements[idx].Type = type;
               d3delements[idx].Method = D3DDECLMETHOD_DEFAULT;
               d3delements[idx].Usage = D3DDECLUSAGE_TEXCOORD;
               d3delements[idx].UsageIndex = 0;
               idx++;
            }

            e = &ret->elements[ALLEGRO_PRIM_COLOR_ATTR];
            if(e->attribute) {
               d3delements[idx].Stream = 0;
               d3delements[idx].Offset = e->offset;
               d3delements[idx].Type = D3DDECLTYPE_D3DCOLOR;
               d3delements[idx].Method = D3DDECLMETHOD_DEFAULT;
               d3delements[idx].Usage = D3DDECLUSAGE_COLOR;
               d3delements[idx].UsageIndex = 0;
               idx++;
            }
            
            d3delements[idx].Stream = 0xFF;
            d3delements[idx].Offset = 0;
            d3delements[idx].Type = D3DDECLTYPE_UNUSED;
            d3delements[idx].Method = 0;
            d3delements[idx].Usage = 0;
            d3delements[idx].UsageIndex = 0;
            
            IDirect3DDevice9_CreateVertexDeclaration(device, d3delements, (IDirect3DVertexDeclaration9**)&ret->d3d_decl);
         }
      }
   }
#else
   ret->d3d_decl = 0;
#endif
   
   ret->stride = stride;
   return ret;
}

/* Function: al_destroy_vertex_decl
 */
void al_destroy_vertex_decl(ALLEGRO_VERTEX_DECL* decl)
{
   free(decl->elements);
   /*
    * TODO: Somehow free the d3d_decl
    */
   free(decl);
}
