#include <SDL.h>

#include "ImgSplit.h"

void
ImgSplit_Start(ImgSplit_Context *context,
               SDL_Surface *surf,
               struct ImgSplit_Options const *opts)
{
  (void) context;
  (void) surf;
  (void) opts;
}

enum ImgSplit_ErrorCode
ImgSplit_Next(ImgSplit_Context *context,
              SDL_Surface **out_surf,
              size_t *out_row,
              size_t *out_column)
{
  (void) context;
  (void) out_row;
  (void) out_column;

  *out_surf = 0;
  return IMG_SPLIT_NO_ERROR;
}

const char*
ImgSplit_GetError(void) {
  return SDL_GetError();
}
