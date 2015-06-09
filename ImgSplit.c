#include <assert.h>

#include <SDL.h>

#include "ImgSplit.h"

void
ImgSplit_Start(ImgSplit_Context *cx,
               SDL_Surface *input,
               struct ImgSplit_Options const *opts)
{
  // We're going to assert that we have enough rows and columns here.
  assert(opts->start_x + opts->columns*opts->width <= (size_t) input->w);
  assert(opts->start_y + opts->rows*opts->height <= (size_t) input->h);

  cx->opts = *opts;
  cx->row = 0;
  cx->column = 0;
  cx->input = input;
}

enum ImgSplit_ErrorCode
ImgSplit_Next(ImgSplit_Context *cx,
              SDL_Surface **out_surf,
              size_t *out_row,
              size_t *out_column)
{
  if (cx->row == cx->opts.rows) {
    // We have nothing else to process.
    *out_surf = 0;
    return IMG_SPLIT_NO_ERROR;
  }

  int const x = cx->opts.start_x + cx->column*cx->opts.width;
  int const y = cx->opts.start_y + cx->row*cx->opts.height;

  Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  rmask = 0xff000000;
  gmask = 0x00ff0000;
  bmask = 0x0000ff00;
  amask = 0x000000ff;
#else
  rmask = 0x000000ff;
  gmask = 0x0000ff00;
  bmask = 0x00ff0000;
  amask = 0xff000000;
#endif

  *out_surf = SDL_CreateRGBSurface(0, cx->opts.width, cx->opts.height, 32,
                                   rmask, gmask, bmask, amask);

  if (!*out_surf) {
    return IMG_SPLIT_SDL_ERROR;
  }

  SDL_Rect const src_rect = {x, y, cx->opts.width, cx->opts.height};
  if (SDL_BlitSurface(cx->input, &src_rect, *out_surf, 0) < 0) {
    SDL_FreeSurface(*out_surf);
    return IMG_SPLIT_SDL_ERROR;
  }

  *out_row = cx->row;
  *out_column = cx->column;

  cx->column++;
  if (cx->column == cx->opts.columns) {
    cx->row++;
    cx->column = 0;
  }

  return IMG_SPLIT_NO_ERROR;
}

const char*
ImgSplit_GetError(void) {
  return SDL_GetError();
}
