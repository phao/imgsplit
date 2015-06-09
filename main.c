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
static char *output_prefix_allocated_storage;
static char *input_file;

static SDL_Surface *input_surf;

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
          if (i == argc) {
            Fail("Starting position unspecified (but -s was found).");
          }
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
          if (i == argc) {
            Fail("Width unspecified (but -w was found).");
          }
          if (ParseNonNegativeInt(argv[i], &cell_width, 0) == 0) {
            Fail("Invalid width value: %s.", argv[i]);
          }
          break;

        case 'h':
          i++;
          if (i == argc) {
            Fail("Height unspecified (but -h was found).");
          }
          if (ParseNonNegativeInt(argv[i], &cell_height, 0) == 0) {
            Fail("Invalid height value: %s.", argv[i]);
          }
          break;

        case 'r':
          i++;
          if (i == argc) {
            Fail("Rows unspecified (but -r was found).");
          }
          if (ParseNonNegativeInt(argv[i], &rows, 0) == 0) {
            Fail("Invalid rows value: %s.", argv[i]);
          }
          break;

        case 'c':
          i++;
          if (i == argc) {
            Fail("Columns unspecified (but -c was found).");
          }
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
          // Prefix was explicitly specified.
          i++;
          if (i < argc) {
            output_prefix = argv[i];
          }
          else if (i == argc) {
            Fail("Prefix string unspecified (but -p was found).");
          }
          break;

        default:
          Fail("Unknown command line option %s.", argv[i]);
      }
    }
    else {
      input_file = argv[i];
    }
  }

  assert(start_pos_x >= 0);
  assert(start_pos_y >= 0);

  if (input_file == 0) {
    Fail("Input file unspecified.");
  }
  else {
    input_surf = IMG_Load(input_file);
    if (input_surf == 0) {
      Fail("%s.", IMG_GetError());
    }
  }

  if (start_pos_x < 0) {
    Fail("Invalid start position (negative x): %d.", start_pos_x);
  }
  else if (start_pos_y < 0) {
    Fail("Invalid start position (negative y): %d.", start_pos_y);
  }
  else if (start_pos_x >= input_surf->w) {
    Fail("Start X position lies outside image width bounds: %d of %d.\n",
         start_pos_x, input_surf->w);
  }
  else if (start_pos_y >= input_surf->h) {
    Fail("Start Y position lies outside image height bounds: %d of %d.\n",
         start_pos_y, input_surf->h);
  }

  if (columns > 0 && rows > INT_MAX/columns) {
    Fail("There are too many rows (%d) and columns (%d).", rows, columns);
  }

  if (rows < 0 && cell_height > 0) {
    rows = (input_surf->h - start_pos_y)/cell_height;
  }
  if (columns < 0 && cell_width > 0) {
    columns = (input_surf->w - start_pos_x)/cell_width;
  }
  if (cell_height < 0 && rows > 0) {
    cell_height = (input_surf->h - start_pos_y)/rows;
  }
  if (cell_width < 0 && columns > 0) {
    cell_width = (input_surf->w - start_pos_x)/columns;
  }

  if (rows <= 0) {
    Fail("Invalid rows value: %d.", rows);
  }
  else if (columns <= 0) {
    Fail("Invalid columns value: %d.", columns);
  }
  else if (cell_width <= 0) {
    Fail("Invalid width value: %d.", cell_width);
  }
  else if (cell_height <= 0) {
    Fail("Invalid height value: %d.", cell_height);
  }


  // The output prefix should be set to the piece of the input file that comes
  // before the extension. If the input file doesn't have an extension, we
  // just make it be the whole input file.
  if (output_prefix == 0) {
    size_t const len = strlen(input_file);
    size_t ext_point = len;
    while (ext_point > 0) {
      ext_point--;
      if (input_file[ext_point] == '.') {
        break;
      }
    }
    if (ext_point == 0) {
      // Input file has no extension. Output prefix is the input file
      // string.
      output_prefix = input_file;
    }
    else {
      // Input file has an extension. Whatever comes before the extension is
      // the output prefix.
      //
      // ext_point is the index of the '.', which is also how many characters
      // there are before the '.'.
      output_prefix = malloc(ext_point+1);
      if (!output_prefix) {
        Fail("Memory error: %s.", strerror(errno));
      }
      output_prefix_allocated_storage = output_prefix;
      strncpy(output_prefix, input_file, ext_point);
      output_prefix[ext_point] = 0;
    }
  }

  assert(rows >= 1);
  assert(columns >= 1);
  assert(cell_width >= 1);
  assert(cell_height >= 1);
  assert(input_file != 0);
  assert(output_prefix != 0);
  assert(dim == 1 || dim == 2);
}

static void
Init(void) {
  // Do I need to initialize any of the subsystems in here? I only am
  // initializing SDL so that SDL2_image works (and I don't even know if I
  // need to call SDL_Init myself for that).
  if (SDL_Init(0) < 0) {
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
  assert(input_surf != 0);

  free(output_prefix_allocated_storage);
  SDL_FreeSurface(input_surf);
  IMG_Quit();
  SDL_Quit();
}

static void
SavePiece(SDL_Surface *piece, int row, int column) {
  /*
   * This function is a bit ugly. The overestimation by 100 is enough, but
   * it's pretty ugly. I also wonder if this malloc call is really needed.
   */
  char *piece_name = malloc(strlen(output_prefix) + 100);
  if (!piece_name) {
    Fail("Memory error: %s.", strerror(errno));
  }

  if (dim == 2) {
    sprintf(piece_name, "%s_%d_%d.png", output_prefix, row, column);
  }
  else {
    assert(dim == 1);
    sprintf(piece_name, "%s_%d.png", output_prefix, row*columns + column);
  }
  if (IMG_SavePNG(piece, piece_name) < 0) {
    Fail("%s.", IMG_GetError());
  }

  free(piece_name);
}

static void
OutputImagePieces(void) {
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
    SavePiece(piece, row, column);
    SDL_FreeSurface(piece);
  }
}

int
main(int argc, char **argv) {
  Init();
  ReadOptions(argc, argv);
  OutputImagePieces();
  Cleanup();
  return 0;
}
