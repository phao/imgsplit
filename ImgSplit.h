#ifndef IMG_SPLIT_H
#define IMG_SPLIT_H

#include <limits.h>

#include <SDL.h>

enum ImgSplit_ErrorCode {
  IMG_SPLIT_NO_ERROR = 0,
  IMG_SPLIT_SDL_ERROR = INT_MIN
};

struct ImgSplit_Options {
  size_t start_x, start_y;
  size_t rows, columns;

  // The pixel width and height of each image.
  size_t width, height;
};

struct ImgSplit_Context {
  struct ImgSplit_Options opts;
  size_t row, column;
  SDL_Surface *input;
};

typedef struct ImgSplit_Context ImgSplit_Context;

/**
 * @note This function cannot fail and there is no need to call any sort of
 * cleanup function.
 *
 * @note ImgSplit doesn't hold on to your options data. You can do whatever
 * you want with it after calling this function.
 */
void
ImgSplit_Start(ImgSplit_Context *context,
               SDL_Surface *input,
               struct ImgSplit_Options const *opts);

/**
 * @note Whenever ImgSplit_Next returns 0 and *out is null, it's because
 * you have no more images to process.
 *
 * @note If an error is returned, the value of *out_surf is garbage. You should
 * not look at it.
 *
 * @note The caller is responsible for freeing the SDL_Surface created by
 * this function.
 *
 * @return 0 (for success) or a negative value indicating an error has
 * happened. Check the enum ImgSplit_ErrorCode for the error codes.
 */
enum ImgSplit_ErrorCode
ImgSplit_Next(ImgSplit_Context *context,
              SDL_Surface **out_surf,
              size_t *out_row,
              size_t *out_column);

/**
 * Returns the error message of the last error that happened.
 */
const char*
ImgSplit_GetError(void);

#endif
