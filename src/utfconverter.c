#include "../include/utfconverter.h"
#define PATH_MAX 4096

unsigned char buf[4];
char file_path[PATH_MAX];
static int verbose_level;
char *endian_convert;
Glyph* glyph;
void *memset_return;


/** calculate time */
int fd, rv, surrogate_count, bytes_count, glyph_count, ascii_count;
double clock_time_read, clock_time_write, clock_time_convert;
double usercpu_time_read, usercpu_time_write, usercpu_time_convert;
double syscpu_time_read, syscpu_time_write, syscpu_time_convert;
struct tms t_read_start, t_read_end;
struct tms t_write_start, t_write_end;
struct tms t_convert_start, t_convert_end;

extern int gethostname(char *, int);
void realpath(char*, char*);
char* strdup(char*);
int 
main(int argc, char** argv)
{
	clock_t tstart; 
	/* After calling parse_args(), filename and conversion should be set. */
	parse_args(argc, argv);
	fd = open(filename, O_RDONLY);
	realpath(filename, file_path);
	surrogate_count = bytes_count = glyph_count = ascii_count = rv = 0;
	clock_time_convert = clock_time_write = clock_time_read = 0;
	usercpu_time_read = usercpu_time_write = usercpu_time_convert = 0;
	syscpu_time_read = syscpu_time_write = syscpu_time_convert = 0;
	glyph = malloc(sizeof(Glyph)); 
	buf[0] = 0;
	buf[1] = 0;
	/* Handle BOM bytes for UTF16 specially. 
         * Read our values into the first and second elements. */
	if((rv = read(fd, &buf[0], 1)) == 1 && 
			(rv = read(fd, &buf[1], 1)) == 1){
		bytes_count += 2; 
		if(buf[0] == 0xff && buf[1] == 0xfe){
			/*file is little endian*/
			source = LITTLE;
		} else if(buf[0] == 0xfe && buf[1] == 0xff){
			/*file is big endian*/
			source = BIG;
		} 
		else {
			if((rv = read(fd, &buf[2], 1)) == 1){
				if(buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf){
					bytes_count++;
					source = utf8;
				}
				else{
					/*file has no BOM*/
					free(&glyph->bytes); 
					fprintf(stderr, "File has no BOM.\n");
					quit_converter(NO_FD); 
				}
			}
		}
		memset_return = memset(glyph, 0, sizeof(Glyph));
		/* Memory write failed, recover from it: */
		if(memset_return == NULL){
			/* Now make the request again. */
			/*memset(glyph, 0, sizeof(Glyph));*/
			fprintf(stderr, "ERROR! Memory allocate issue.\n");
			quit_converter(NO_FD);
		}
	}
	else{
		fprintf(stderr, "ERROR! Invaild File.\n");
		print_help();
		quit_converter(NO_FD);
	}

	/** write the BOM to ouput file*/
	if (conversion == LITTLE){
		buf[0] = 0xff; 
		buf[1] = 0xfe;
	}
	else{
		buf[0] = 0xfe;
		buf[1] = 0xff; 
		/** reversed because swap function */
	}
	/** add the BOM */
	glyph->bytes[0] = buf[0];
	glyph->bytes[1] = buf[1];
	glyph->surrogate = false;
	write_glyph(glyph);
	ascii_count--;
	tstart = clock();
	/**  UTF 16 */
	if(source != utf8){ 
		times(&t_read_start);
		/* Now deal with the rest of the bytes.*/
		while((rv = read(fd, &buf[0], 1)) == 1 && 
				(rv = read(fd, &buf[1], 1)) == 1){
			bytes_count+=2;
			write_glyph(fill_glyph(glyph, buf, source, &fd));
			glyph_count++;
			memset_return = memset(glyph, 0, sizeof(Glyph));
	    	/* Memory write failed, recover from it: */
	    	if(memset_return == NULL){
		    	/*memset(glyph, 0, sizeof(Glyph));*/
				fprintf(stderr, "ERROR! Memory allocate issue.\n");
				quit_converter(NO_FD);
	    	}
	    	times(&t_read_start);
	   		tstart = clock();
		}

	}
	else{ /** UTF8 !!!!! */
		unsigned int msb;
		clock_t tend;
		times(&t_read_start);
		/** since it is utf8, read a byte at a time */
		while((rv=read(fd,&buf[0],1))==1){
			times(&t_read_end);
			usercpu_time_read += (double)((t_read_end.tms_utime - t_read_start.tms_utime) / sysconf(_SC_CLK_TCK));
			syscpu_time_read += (double)((t_read_end.tms_stime - t_read_start.tms_stime) / sysconf(_SC_CLK_TCK));
			tend = clock();
			clock_time_read += (double)((tend - tstart)/sysconf(_SC_CLK_TCK));
			bytes_count++;
			/** chek for how many bytes does the letter  need*/
			msb = (buf[0] >> (sizeof(buf[0])*8-1));
			if (msb == 0){
				ascii_count++;
				glyph->bytes[0] = 0x00;
				glyph->bytes[1] = buf[0];
				glyph->surrogate = false;
				write_glyph(convert(glyph, conversion));
				glyph_count++;
			} /** it is an ascii character */
			else{
				/** check top 3 bits*/
				unsigned int top_three = (buf[0] >> 5);
				if(top_three == 6){
					if((rv=read(fd,&buf[1],1))==1){
						unsigned int vaildf;
						unsigned int cat;
						bytes_count++;
						vaildf = (buf[1] >> (sizeof(buf[1])*8-2));
						if (vaildf != 2){
							fprintf(stderr, "Invaild File\n");
							quit_converter(NO_FD);
						}
						cat = buf[0] & 0x1f; /** I just want the 5 least singnificant bits*/
						cat = cat << 6;
						cat |= buf[1] & 0x3f;
						if(cat > 255){
							glyph->bytes[1] = (unsigned char) cat;
							glyph->bytes[0] = (unsigned char) (cat >> 8);
						}
						else{
							glyph->bytes[0] = 0x00;
							glyph->bytes[1] = cat;
						}
						glyph->surrogate = false;
						write_glyph(convert(glyph, conversion));
						glyph_count++;;
					}
					else{
						fprintf(stderr, "Invaild File\n");
						quit_converter(NO_FD);
					}
				} /** this glyph will take 2 bytes*/
				else{
					unsigned int top_four; 
					top_four = (buf[0] >> (sizeof(buf[0])*8-4));
					if (top_four == 14){
						if((rv=read(fd,&buf[1],1))==1 && (rv=read(fd,&buf[2],1))==1){
							unsigned int cat;
							bytes_count+=2;
							if (buf[1] >> 6 != 2 || buf[2] >> 6 != 2){
								fprintf(stderr, "Invaild File\n");
								quit_converter(NO_FD);
							}
							cat = buf[0] & 0xf; /** I just want the 5 least singnificant bits*/
							cat = cat << 6;
							cat |= buf[1] & 0x3f;
							cat = cat << 6;
							cat |= buf[2] & 0x3f;
							/** now i have the unicode */
							if (cat >= 0x10000){
								unsigned int high, low;
								unsigned char ab, cd, ef, gh;
								surrogate_count++;
								cat = cat - 0x10000;	
								high = cat >> 10;
								low = cat & 0x3ff;
								high += 0xD800;	/** high code point*/
								low += 0xDC00;	/** low code point*/
								ab = (unsigned char)high;
								cd = (unsigned char)(high >> 8);
								ef = (unsigned char)low;
								gh = (unsigned char)(low >> 8);
								glyph->bytes[1] = ab;
								glyph->bytes[0] = cd;
								glyph->bytes[3] = ef;
								glyph->bytes[2] = gh;
								glyph->surrogate = true;
								/** check for surrogate pair*/
							}
							else{
								unsigned char ab, cd;
								ab = (unsigned char)cat;
								cd = (unsigned char)(cat >> 8);
								glyph->bytes[0] = cd;
								glyph->bytes[1] = ab;
								glyph->surrogate = false;
							}
							write_glyph(convert(glyph, conversion));
							glyph_count++;;
						}
						else{
							fprintf(stderr, "Invaild File\n");
							quit_converter(NO_FD);
						}
					}
					else{
						unsigned int top_five;
						top_five = (buf[0] >> (sizeof(buf[0])*8-5));
						if (top_five == 30){
							if((rv=read(fd,&buf[1],1))==1 && (rv=read(fd,&buf[2],1))==1 && (rv=read(fd,&buf[3],1))==1){
								unsigned int cat;
								bytes_count+=3;
								if (buf[1] >> 6 != 2 || buf[2] >> 6 != 2 || buf[3] >> 6 != 2){
									fprintf(stderr, "Invaild File\n");
									quit_converter(NO_FD);
								}
								/** now handle 4 bytes*/
								cat = buf[0] & 0x7; /** I just want the 5 least singnificant bits*/
								cat = cat << 6;
								cat |= buf[1] & 0x3f;
								cat = cat << 6;
								cat |= buf[2] & 0x3f;
								cat = cat << 6;
								cat |= buf[3] & 0x3f;
								/** now i have the unicode */
								if (cat >= 0x10000){
									unsigned int high, low;
									unsigned char ab, cd, ef, gh;
									surrogate_count++;
									cat = cat - 0x10000;
									high = cat >> 10;	
									low = cat & 0x3ff;
									high += 0xD800;	/** high code point*/
									low += 0xDC00;	/** low code point*/
									ab = (unsigned char)high;
									cd = (unsigned char)(high >> 8);
									ef = (unsigned char)low;
									gh = (unsigned char)(low >> 8);
									glyph->bytes[1] = ab;
									glyph->bytes[0] = cd;
									glyph->bytes[3] = ef;
									glyph->bytes[2] = gh;
									glyph->surrogate = true;
									/** check for surrogate pair*/
								}
								else{
									unsigned char ab, cd;
									ab = (unsigned char)cat;
									cd = (unsigned char)(cat >> 8);
									glyph->bytes[0] = cd;
									glyph->bytes[1] = ab;
									glyph->surrogate = false;
									}	
								write_glyph(convert(glyph, conversion));
								glyph_count++;
								
							}
							else{
								fprintf(stderr, "Invaild File\n");
								quit_converter(NO_FD);
							}
						}
					}
				}
			}
			memset_return = memset(glyph, 0, sizeof(Glyph));
	    	/* Memory write failed, recover from it: */
	    	if(memset_return == NULL){
		    	/*memset(glyph, 0, sizeof(Glyph));*/
				fprintf(stderr, "ERROR! Memory allocate issue.\n");
				quit_converter(NO_FD);
	    	}
			times(&t_read_start);
	   		tstart = clock();
		}
	}
	check_verbose_level();
	quit_converter(0);
	return 0;
}

Glyph* 
swap_endianness(Glyph* glyph) {
	unsigned int temp;
	if(source != conversion){
		temp = 0;
		temp = glyph->bytes[0];
		glyph->bytes[0] = glyph->bytes[1];
		glyph->bytes[1] = temp;
		if(glyph->surrogate){
			temp = 0;
			temp = glyph->bytes[2];
			glyph->bytes[2] = glyph->bytes[3];
			glyph->bytes[3] = temp;}
	}
	glyph->end = conversion;
	return glyph;
}


Glyph* convert (Glyph* glyph, endianness end){
	if(end == LITTLE){
		unsigned int temp;
		temp = 0;
		temp = glyph->bytes[0];
		glyph->bytes[0] = glyph->bytes[1];
		glyph->bytes[1] = temp;
		if(glyph->surrogate){
			temp = 0;
			temp = glyph->bytes[2];
			glyph->bytes[2] = glyph->bytes[3];
			glyph->bytes[3] = temp;}
	}
	return glyph;
}

Glyph* fill_glyph(Glyph* glyph, unsigned char data[4], endianness end, int* fd) 
{
	clock_t tstart;
	clock_t tend;
	unsigned int bits;
	times(&t_convert_start);
	tstart = clock();
	glyph->bytes[0] = data[0];
	glyph->bytes[1] = data[1];
	bits = 0; 
	if(end == LITTLE){
		bits |= (data[FIRST] + (data[SECOND] << 8));
	}
	else{
		bits |= ((data[FIRST] <<8) + data[SECOND]);
	}
	if(bits <= 127){
		ascii_count++;
	}
	/* Check high surrogate pair using its special value range.*/
	/* #include "utfconverter.c" */
	if(bits > 0xD800 && bits < 0xDBFF){ 
		if(read(*fd, &data[THIRD], 1) == 1 && 
			read(*fd, (&data[FOURTH]), 1) == 1){
			bits = 0;
			if(end == LITTLE){
				bits |= (data[THIRD] + (data[FOURTH]<< 8));
			}
			else{
				bits |= ((data[THIRD] << 8)+ data[FOURTH]);
			}
			if(bits > 0xDC00 && bits < 0xDFFF){ /* Check low surrogate pair.*/
				glyph->surrogate = true; 
				surrogate_count++;
			} else {
				lseek(*fd, -OFFSET, SEEK_CUR); 
				glyph->surrogate = false;
			}
		}
		else{
			glyph->surrogate = false;
		}
	}

	if(!glyph->surrogate){
		glyph->bytes[THIRD]=0;
		glyph->bytes[FOURTH]=0;
	}
	else{
		glyph->bytes[THIRD] = data[THIRD];
		glyph->bytes[FOURTH] = data[FOURTH];
	}
	swap_endianness(glyph);
	glyph->end = end;
	times(&t_convert_end);
	usercpu_time_convert += (double)((t_convert_end.tms_utime - t_convert_start.tms_utime)/ sysconf(_SC_CLK_TCK));
	syscpu_time_convert += (double)((t_convert_end.tms_stime - t_convert_start.tms_stime)/ sysconf(_SC_CLK_TCK));
	tend = clock();
	clock_time_convert += (double)((tend - tstart)/ sysconf(_SC_CLK_TCK));

	return glyph;
}

void 
write_glyph(Glyph* glyph){
	clock_t tstart, tend;
	tstart = clock();
	times(&t_write_start);
	if(glyph->surrogate){
		write(STDOUT_FILENO, glyph->bytes, SURROGATE_SIZE);
	} else {
		write(STDOUT_FILENO, glyph->bytes, NON_SURROGATE_SIZE);
	}
	tend = clock();
	clock_time_write += (double)((tend - tstart)/ sysconf(_SC_CLK_TCK));
	times(&t_write_end);
	usercpu_time_write += (double)((t_write_end.tms_utime - t_write_start.tms_utime)/ sysconf(_SC_CLK_TCK));
	syscpu_time_write += (double)((t_write_end.tms_stime - t_write_start.tms_stime)/ sysconf(_SC_CLK_TCK));
}

void 
parse_args(int argc, char** argv){
	int option_index, c;
	#include "../include/struct.txt"
	endian_convert = NULL;
	/* If getopt() returns with a valid (its working correctly)*/
	/* return code, then process the args! */
	while((c = getopt_long(argc, argv, "hvu:", long_options, &option_index)) 
			!= -1){
		switch(c){ 
			case 'h':
				print_help();
				if(argc == 2){exit(0);}
				break;
			case 'u':
				if((strcmp(optarg, "16LE") == 0) || (strcmp(optarg, "16BE") == 0))
					endian_convert = optarg;
				else{
					print_help();
					exit(NO_FD);}
				break;
			case 'v':
				if(verbose_level < 2){
					verbose_level++;
				}
				break;
			case '?':
				fprintf(stderr, "Unrecognized argument.\n");
				print_help();
				exit(NO_FD);
				break;
			default:
				abort();
		}
	}
	if(argc == 1){
		print_help();
		exit(NO_FD);}
	if(optind < argc){
		filename = strdup(argv[optind]);
	} else {
		fprintf(stderr, "Filename not given.\n");
		print_help();
		exit(NO_FD);
	}
	if(argv[optind+1] != NULL)
	{
		char fpath[PATH_MAX]; 
		char f1path[PATH_MAX];
		filename1 = strdup(argv[optind+1]);
		realpath(filename, fpath);
		realpath(filename1,f1path);
		if (strcmp(fpath,f1path) == 0){
			fprintf(stderr, "Input file CAN NOT be the same as output file.\n");
			print_help();
			exit(NO_FD);
		}
		freopen(filename1, "w", stdout);	
	}

	if(endian_convert == NULL){
		fprintf(stderr, "Converson mode not given.\n");
		print_help();
		exit(NO_FD);
	}
	if(strcmp(endian_convert, "16LE") == 0){ 
		conversion = LITTLE;
	} else if(strcmp(endian_convert, "16BE") == 0){
		conversion = BIG;
	} else {
		fprintf(stderr,"invild argument \n");
		print_help();
		exit(NO_FD);
	}	
}

void print_help() {
	int i;
	for(i = 0; i < 10; i++){
		printf("%s",USAGE[i]); 
	}
}

void 
quit_converter(int something)
{
	close(STDERR_FILENO);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	if(something != NO_FD){
		close(fd);
		exit(0);
	}
	else{
		close(fd);
		exit(NO_FD);
	}

}

double my_round(double d){
	d = d*10.0f;
	d = (d>(floor(d)+0.5f)) ? ceil(d) : floor(d);
	d = d/10.0f;
	return d;
}

void check_verbose_level(){
	struct utsname myname;
	char hostname[PATH_MAX];
	if(verbose_level != 0){
		double calculator;
		calculator = ((double)bytes_count/(double)1000);
		calculator = my_round(calculator);
		fprintf(stderr,"    Input file size: %.0f kb\n",calculator);
		fprintf(stderr,"    Input file path: %s\n", file_path);
		if(source == LITTLE){
			fprintf(stderr,"    Input file encoding: %s\n", UTF16LE);
		}
		else if(source == BIG){
			fprintf(stderr,"    Input file encoding: %s\n", UTF16BE);
		}
		else if(source == utf8){
			fprintf(stderr,"    Input file encoding: %s\n", UTF8);
		}
		else{
			fprintf(stderr,"I dont know what file encode is this\n");
			quit_converter(NO_FD);
		}
		if(conversion == LITTLE){
			fprintf(stderr,"    Output encoding: %s\n", UTF16LE);
		}
		else if(conversion == BIG){
			fprintf(stderr,"    Output encoding: %s\n", UTF16BE);
		}
		gethostname(hostname, sizeof(hostname));
		fprintf(stderr,"    Hostmachine: %s\n", hostname);
		if(uname(&myname) == -1){
			fprintf(stderr, "ERROR! FAIL CALLING UNAME\n");
			quit_converter(NO_FD);
		}
		fprintf(stderr,"    Operating System: %s\n", myname.sysname);
		if (verbose_level == 2){
			fprintf(stderr,"    Reading: real=%.1f, user=%.1f, sys=%.1f\n",clock_time_read, usercpu_time_read, syscpu_time_read);
			fprintf(stderr,"    Converting: real=%.1f, user=%.1f, sys=%.1f\n",clock_time_convert,usercpu_time_convert,syscpu_time_convert);
			fprintf(stderr,"    Writing: real=%.1f, user=%.1f, sys=%.1f\n",clock_time_write, usercpu_time_write, syscpu_time_write);
			calculator = ((double)ascii_count/(double)glyph_count)*100;
			calculator = my_round(calculator);
			fprintf(stderr,"    ASCII: %0.0f%c\n",(calculator),37);
			calculator = ((double)surrogate_count/(double)glyph_count)*100;
			calculator = my_round(calculator);
			fprintf(stderr,"    Surrogates: %0.0f%c\n",(calculator),37);
			fprintf(stderr,"    Glyphs: %d\n", glyph_count);
		}
	}
}
