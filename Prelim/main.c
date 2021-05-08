#include "hello.h"

/******************************************************************************
 * 
 * Prelim lab header
 * 
 * Author: Mark Angelo Gameng 
 * Email: mgameng1@hawk.iit.edu 
 * AID:   A20419026 
 * Date:   1/25/21
 * 
 * By signing above, I pledge on my honor that I neither gave nor received any
 * unauthorized assistance on the code contained in this repository.
 * 
 *****************************************************************************/

int main(int argc, char *argv[]) {
  if (argc > 1) {
    say_hello_to(argv[1]);
  } else {
    say_hello_to("world");
  }
  return 0;
}
