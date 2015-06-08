#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <stdio.h>

#include <SDL.h>
#include <SDL_image.h>

#include "ImgSplit.h"

static int rows = -1,
           columns = -1,
           cell_width = -1,
           cell_height = -1,
           start_pos_x,
           start_pos_y,
           dim = 2;

static char *output_prefix;

static char *input_file;

static void
Fail(char const *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fputs("Error: ", stderr);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fputc('\n', stderr);
  exit(EXIT_FAILURE);
}

static char*
ParseNonNegativeInt(char *str, int *out, char expected_end) {
  char *ended_at;
  long aux = strtol(str, &ended_at, 10);
  if (*ended_at == expected_end && aux >= 0L && aux <= INT_MAX) {
    *out = aux;
    return ended_at;
  }
  return 0;
}

static void
ReadOptions(int argc, char **argv) {
  int reading_opts = 1;

  for (int i = 1; i < argc; i++) {
    if (reading_opts && *argv[i] == '-' && argv[i][1] && argv[i][2] == 0) {
      switch (argv[i][1]) {
        case 's': {
          i++;
          char *ended_at = ParseNonNegativeInt(argv[i], &start_pos_x, ',');
          if (ended_at) {
            ended_at++;
            if (ParseNonNegativeInt(ended_at, &start_pos_y, 0) == 0) {
              Fail("Invalid starting position: %s.\n", argv[i]);
            }
          }
          else {
            Fail("Invalid starting position: %s.", argv[i]);
          }
          break;
        }
        case 'w':
          i++;
          if (ParseNonNegativeInt(argv[i], &cell_width, 0) == 0) {
            Fail("Invalid width value: %s.", argv[i]);
          }
          break;
        case 'h':
          i++;
          if (ParseNonNegativeInt(argv[i], &cell_height, 0) == 0) {
            Fail("Invalid height value: %s.", argv[i]);
          }
          break;
        case 'r':
          i++;
          if (ParseNonNegativeInt(argv[i], &rows, 0) == 0) {
            Fail("Invalid rows value: %s.", argv[i]);
          }
          break;
        case 'c':
          i++;
          if (ParseNonNegativeInt(argv[i], &columns, 0) == 0) {
            Fail("Invalid columns value: %s.", argv[i]);
          }
          break;
        case '1':
          dim = 1;
          break;
        case '2':
          dim = 2;
          break;
        case '-':
          reading_opts = 0;
          break;
        case 'p':
          i++;
          if (i < argc) {
            output_prefix = argv[i];
          }
          else if (i == argc) {
            Fail("Prefix string unspecified.");
          }
          break;
        default:
          Fail("Unknown command line option %c.", argv[i]);
      }
    }
    else {
      input_file = argv[i];
    }
  }

  if (rows <= 0) {
    Fail("Invalid rows.");
  }
  else if (columns <= 0) {
    Fail("Invalid columns.");
  }
  else if (cell_width <= 0) {
    Fail("Invalid width.");
  }
  else if (cell_height <= 0) {
    Fail("Invalid height.");
  }
  else if (input_file == 0) {
    Fail("Input file unspecified.");
  }
  else if (start_pos_x < 0) {
    Fail("Invalid start position (negative x): %d.", start_pos_x);
  }
  else if (start_pos_y < 0) {
    Fail("Invalid start position (negative y): %d.", start_pos_y);
  }

  if (rows > INT_MAX/columns) {
    Fail("There are too many rows (%d) and columns (%d).", rows, columns);
  }

  // The output prefix should be set to the piece of the input file that comes
  // before the extension. If the input file doesn't have an extension, we
  // just make it be the whole input file.
  if (output_prefix == 0) {
    size_t len = strlen(input_file);
    size_t ext_point = len;
    while (ext_point > 0) {
      ext_point--;
      if (input_file[ext_point] == '.') {
        break;
      }
    }
    if (ext_point == 0) {
      output_prefix = input_file;
    }
    else {
      // ext_point is the index of the '.', which is also how many characters
      // there are before the '.'.
      output_prefix = malloc(ext_point+1);
      if (!output_prefix) {
        Fail("Memory error: %s.", strerror(errno));
      }
      strncpy(output_prefix, input_file, ext_point);
      output_prefix[ext_point] = 0;
    }
  }

  assert(rows >= 1);
  assert(columns >= 1);
  assert(cell_width >= 1);
  assert(cell_height >= 1);
  assert(start_pos_x >= 0);
  assert(start_pos_y >= 0);
  assert(input_file != 0);
  assert(output_prefix != 0);
  assert(dim == 1 || dim == 2);
}

static void
Init(void) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    Fail("%s.", SDL_GetError());
  }
  int img_init_flags = (IMG_INIT_JPG | IMG_INIT_PNG |
                        IMG_INIT_TIF | IMG_INIT_WEBP);
  if ((IMG_Init(img_init_flags) & img_init_flags) != img_init_flags) {
    Fail("%s.", IMG_GetError());
  }
}

static void
Cleanup(void) {
  if (output_prefix != input_file) {
    // The output prefix was allocated through a malloc call.
    free(output_prefix);
  }
  IMG_Quit();
  SDL_Quit();
}

static char*
GenOutputFileName(int row, int column) {
  size_t const len = strlen(output_prefix) + 100;
  char * const name = malloc(len);
  if (name == 0) {
    Fail("Memory error: %s.", strerror(errno));
  }
  if (dim == 1) {
    sprintf(name, "%s_%d_%d.png", output_prefix, row, column);
  }
  else {
    assert(dim == 2);
    sprintf(name, "%s_%d.png", output_prefix, row*column + column);
  }
  return name;
}

static void
OutputImagePieces(void) {
  SDL_Surface * const input_surf = IMG_Load(input_file);
  if (input_surf == 0) {
    Fail("%s.", IMG_GetError());
  }
  struct ImgSplit_Options const opts = {
    .start_x = start_pos_x,
    .start_y = start_pos_y,
    .rows = rows,
    .columns = columns,
    .width = cell_width,
    .height = cell_height
  };
  ImgSplit_Context img_split_context;
  ImgSplit_Start(&img_split_context, input_surf, &opts);

  for (;;) {
    size_t row, column;
    SDL_Surface *piece;
    if (ImgSplit_Next(&img_split_context, &piece, &row, &column) < 0) {
      Fail("%s.", ImgSplit_GetError());
    }
    if (piece == 0) {
      break;
    }

    char * const output_file_name = GenOutputFileName(row, column);
    if (IMG_SavePNG(piece, output_file_name) < 0) {
      Fail("%s.", IMG_GetError());
    }
    free(output_file_name);
    SDL_FreeSurface(piece);
  }

  SDL_FreeSurface(input_surf);
}

int
main(int argc, char **argv) {
  Init();
  ReadOptions(argc, argv);
  OutputImagePieces();
  Cleanup();
  return 0;
}
