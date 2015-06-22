#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <SDL.h>
#include <libfreenect.h>

#define DEFAULT_WIDTH  640
#define DEFAULT_HEIGHT 480
#define MMAP_BUFFERS 10

unsigned int frames = 0;

freenect_context *f_ctx;
freenect_device *f_dev;

SDL_Surface* camSurf, *display;
SDL_Rect rect;

SDL_Surface* initSDL(int w, int h) {
  if( SDL_Init(SDL_INIT_VIDEO) ) {
    printf("Couldn't initialize SDL: %s\n", SDL_GetError());
    return 0;
  }

  SDL_Surface* display = SDL_SetVideoMode(w, h, 0, SDL_SWSURFACE);
  if( !display ) {
    printf("Couldn't initialize surface: %s\n", SDL_GetError());
    return 0;
  }

  SDL_FillRect(display, 0, SDL_MapRGB(display->format, 0, 0, 0));
  SDL_UpdateRect(display, 0, 0, 0, 0);

  return display;
}

void usage(char* name) {
  fprintf(stderr, "Usage: %s\n", name);
  exit(EXIT_FAILURE);
}

void callback_video(freenect_device *dev, void *video, uint32_t timestamp) {
  SDL_BlitSurface(camSurf, &rect, display, 0); //maybe implement multi-buffering to reduce callback delay
  SDL_UpdateRect(display, rect.x, rect.y, rect.w, rect.h);
  frames++;
printf("video callback\n");
}

int main(int argc, char** argv) {
  if( argc > 1 )
    usage(*argv);

  int targetWidth = DEFAULT_WIDTH, targetHeight = DEFAULT_HEIGHT;
  printf("Targeting %dx%d resolution\n", targetWidth, targetWidth);


  if( freenect_init(&f_ctx, 0) < 0 ) {
    printf("freenect_init() failed\n");
    exit(EXIT_FAILURE);
  }

  freenect_set_log_level(f_ctx, FREENECT_LOG_DEBUG);
  freenect_select_subdevices(f_ctx, (freenect_device_flags)(FREENECT_DEVICE_CAMERA)); //FREENECT_DEVICE_MOTOR |

  int nr_devices = freenect_num_devices (f_ctx);
  printf ("Number of devices found: %d\n", nr_devices);

  if( nr_devices < 1 ) {
    freenect_shutdown(f_ctx);
    exit(EXIT_FAILURE);
  }

  if( freenect_open_device(f_ctx, &f_dev, 0) < 0 ) { //TODO: add support for user-defined device number != 0
    printf("Could not open device\n");
    freenect_shutdown(f_ctx);
    exit(EXIT_FAILURE);
  }

  rect.x = 0; rect.y = 0; rect.w = targetWidth; rect.h = targetHeight;
  display = initSDL(targetWidth, targetHeight);
  camSurf = SDL_CreateRGBSurface(SDL_SWSURFACE, targetWidth, targetHeight, 24, 0x0000FF, 0x00FF00, 0xFF0000, 0);

  freenect_set_led(f_dev,LED_RED);
//  freenect_set_depth_callback(f_dev, depth_cb);
  freenect_set_video_callback(f_dev, callback_video);
  freenect_set_video_mode(f_dev, freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB));
//  freenect_set_depth_mode(f_dev, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT));
  freenect_set_video_buffer(f_dev, camSurf->pixels);

  freenect_start_video(f_dev);

  SDL_Event event;
  bool run = true;
  time_t startTime = time(0);
  time_t currentTime = startTime;
  time_t newTime = startTime;

  while( run ) {
printf("mainloop\n");
    while( SDL_PollEvent(&event) ) {
      switch( event.type ) {
        case SDL_QUIT:
          printf("Quit.\n");
          run = false;
          break;
        /*default:
          printf("Unhandled SDL_Event\n");*/
      }
    }

    if( freenect_process_events(f_ctx) < 0 ) //TODO: sleep to save cpu time
      printf("freenect_process_events < 0!\n");

    newTime = time(0);
    if( currentTime != newTime ) { //only every second
      float fps = (float)frames/(newTime-startTime);
      printf("\rfps: %#6.2f", fps);
      fflush(stdout);
      currentTime = newTime;
    }
  }

//  freenect_stop_depth(f_dev);
  freenect_stop_video(f_dev);
  freenect_close_device(f_dev);
  freenect_shutdown(f_ctx);

  if( camSurf )
    SDL_FreeSurface(camSurf);
  SDL_FreeSurface(display);
  SDL_Quit();
  exit(EXIT_SUCCESS);
}
