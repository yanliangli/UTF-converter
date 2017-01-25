#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h> 
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>

#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <stdint.h>
#include <sys/utsname.h>


#define MAX_BYTES 4
#define SURROGATE_SIZE 4
#define NON_SURROGATE_SIZE 2
#define NO_FD -1
#define OFFSET 2

#define FIRST  0
#define SECOND 1
#define THIRD  2
#define FOURTH 3

#ifdef __STDC__
#define P(x) x
#else
#define P(x) ()
#endif

/** The enum for endianness. */
typedef enum {LITTLE, BIG, utf8} endianness;

/** The struct for a codepoint glyph. */
typedef struct Glyph {
	unsigned char bytes[MAX_BYTES];
	endianness end;
	bool surrogate;
} Glyph;

/** The given filename. */
char* filename;
char* filename1;

/** The usage statement. */
const char* USAGE[] = {
"  ./utf [-h|--help] [-v|-vv] -u OUT_ENC | --UTF=OUT_ENC IN_FILE [OUT_FILE]\n",
"\n  Option arguments:\n",
"    -h, --help\t    Displays this usage.\n",
"    -v, -vv\t    Toggles the verbosity of the program to level 1 or 2.\n",
"\n  Mandatory argument:\n",
"    -u OUT_ENC, --UTF=OUT_ENC\tSets the output encoding.\n",
"\t\t\t\t  Valid values for OUT_ENC: 16LE, 16BE\n\t\t",
"\n  Positional Arguments:\n",
"    IN_FILE\t    The file to convert.\n",
"    [OUT_FILE]\t    Output file name. If not present, defaults to stdout.\n",
};

const char* UTF8 = {"UTF-8"};
const char* UTF16LE = {"UTF-16LE"};
const char* UTF16BE = {"UTF-16BE"};

/** Which endianness to convert to. */
endianness conversion;

/** Which endianness the source file is in. */
endianness source;

/**
 * A function that swaps the endianness of the bytes of an encoding from
 * LE to BE and vice versa.
 *
 * @param glyph The pointer to the glyph struct to swap.
 * @return Returns a pointer to the glyph that has been swapped.
 */
Glyph* swap_endianness P((Glyph*));

/**
 * Fills in a glyph with the given data in data[2], with the given endianness 
 * by end.
 *
 * @param glyph 	The pointer to the glyph struct to fill in with bytes.
 * @param data[2]	The array of data to fill the glyph struct with.
 * @param end	   	The endianness enum of the glyph.
 * @param fd 		The int pointer to the file descriptor of the input 
 * 			file.
 * @return Returns a pointer to the filled-in glyph.
 */
Glyph* fill_glyph P((Glyph* glyph, unsigned char data[2], endianness end, int* fd));


/**
* A function that converts a UTF-8 glyph to a UTF-16LE or UTF-16BE
* glyph, and returns the result as a pointer to the converted glyph.
*
* @param glyph The UTF-8 glyph to convert.
* @param end   The endianness to convert to (UTF-16LE or UTF-16BE).
* @return The converted glyph.
*/
Glyph* convert P((Glyph* glyph, endianness end));


/**
 * Writes the given glyph's contents to stdout.
 *
 * @param glyph The pointer to the glyph struct to write to stdout.
 */
void write_glyph P((Glyph*));

/**
 * Calls getopt() and parses arguments.
 *
 * @param argc The number of arguments.
 * @param argv The arguments as an array of string.
 */
void parse_args P((int, char**));

/**
 * Prints the usage statement.
 */
void print_help P(());

/**
 * Closes file descriptors and frees list and possibly does other
 * bookkeeping before exiting.
 *
 * @param The fd int of the file the program has opened. Can be given
 * the macro value NO_FD (-1) to signify that we have no open file
 * to close.
 */
void quit_converter P((int));

double my_round P((double));

void check_verbose_level P(());

