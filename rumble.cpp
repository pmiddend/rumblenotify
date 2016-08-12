// This file contains all the rumble code, nothing notification related here
#include "rumble.hpp"
#include <linux/input.h>
#include <sys/ioctl.h>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

// This is shamelessly copied from fftest.c
#define nBitsPerUchar          (sizeof(unsigned char) * 8)
#define bitOffsetInUchar(bit)  ((bit)%nBitsPerUchar)
#define ucharIndexForBit(bit)  ((bit)/nBitsPerUchar)
#define testBit(bit, array)    ((array[ucharIndexForBit(bit)] >> bitOffsetInUchar(bit)) & 1)

int rumblenotify::rumble() {
  // First, let's look for compatible game-pads in /dev/input
  DIR *const eventDir{opendir("/dev/input")};

  // ...if /dev/input exists, that is.
  if(eventDir == 0) {
    std::cerr << "couldn't open /dev/event: " << std::strerror(errno) << " (" << errno << ")\n";
    return EXIT_FAILURE;
  }

  // Loop through all non-directories in /dev/event and test for the specific effect we're using...
  int gamepad_fd = -1;
  dirent *current_dir_entry = readdir(eventDir);
  while(current_dir_entry) {
    // readdir always outputs "." and stuff, so filter those here.
    if(current_dir_entry->d_type != DT_DIR) {
      // C string concatenation for lazy C++ers
      std::string const cppfn{"/dev/input/"+std::string{current_dir_entry->d_name}};
      char const *fn = cppfn.c_str();
      gamepad_fd = open(fn,O_RDWR);
      if(gamepad_fd == -1) {
	std::cerr << "error opening “" << fn << "”: " << std::strerror(errno) << " (" << errno << ")\n";
	return EXIT_FAILURE;
      }

      // This is shamelessly copied from fftest.c
      unsigned char ffFeatures[1 + FF_MAX/8/sizeof(unsigned char)] = {0};
      if (ioctl(gamepad_fd, EVIOCGBIT(EV_FF, sizeof(ffFeatures)), ffFeatures) == -1) {
	close(gamepad_fd);
	gamepad_fd = -1;
      }

      if(!testBit(FF_PERIODIC,ffFeatures)) {
	close(gamepad_fd);
	gamepad_fd = -1;
      }

      // We've found a matching gamepad. Just take the first one.
      if(gamepad_fd != -1) {
	break;
      }
    }

    current_dir_entry = readdir(eventDir);
  }

  // Upload a new effect! All values are hard-coded because I can.
  __s16 const effect_id{0};
  __u16 const effect_direction{0};
  ff_effect effect{
    FF_PERIODIC,
      -1,
      effect_direction,
      ff_trigger{
      0,
	0
	},
      ff_replay{
	1000,
	  0
	  },
	.u = { .periodic = {
	  FF_SINE,
	  100,
	  0x7fff,
	  0,
	  0,
	  ff_envelope{
	    1000,
	    0x7fff,
	    1000,
	    0x7fff
	  },
	  0,
	  0
	}}
  };

  if(ioctl(gamepad_fd,EVIOCSFF,&effect) == -1) {
    std::cerr << "error uploading effect: " << std::strerror(errno) << " (" << errno << ")\n";
    return EXIT_FAILURE;
  }

  // Play it, then close the gamepad for reuse.
  input_event const play{
    timeval{
      0,
	0
	},
      EV_FF,
	effect_id,
	1
	};

  if(write(gamepad_fd,&play,sizeof(play)) == -1) {
    std::cerr << "error playing effect: " << std::strerror(errno) << " (" << errno << ")\n";
    return EXIT_FAILURE;
  }

  //close(gamepad_fd);

  return EXIT_SUCCESS;
}
