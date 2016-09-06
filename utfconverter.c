#include "utfconverter.h"

#ifdef CSE320
    #define debug_hostname(msg) printf("CSE320: The hostname of this machine is: %s\n", msg)
    #define debug_input_file_name(msg) printf("CSE320: The name of the input file is: %s\n", msg)
    #define debug_inode_number(msg) printf("CSE320: The inode number of the input file is: %d\n", msg)
    #define debug_device_number(msg) printf("CSE320: The device number of the input file is: %d\n", msg)
    #define debug_file_size(msg) printf("CSE320: The file size of the input file is: %d\n", msg)
    #define debug_output_file_name(msg) printf("CSE320: The name of the output file is: %s\n", msg)
    #define debug_input_file_encoding(msg) printf("CSE320: The encoding of the input file is: %s\n", msg)
    #define debug_output_file_encoding(msg) printf("CSE320: The encoding of the output file is: %s\n", msg)
#else
    #define debug_hostname(msg)
    #define debug_input_file_name(msg)
    #define debug_inode_number(msg)
    #define debug_device_number(msg)
    #define debug_file_size(msg)
    #define debug_output_file_name(msg)
    #define debug_input_file_encoding(msg)
    #define debug_output_file_encoding(msg)
#endif


bool to_UTF8 = false;
bool to_UTF16_LE = false;
bool to_UTF16_BE = false;


char ASCII[1024];
int num_of_bytes[1024];
int codepoint[1024];
int input[1024];
int output[1024];

int array_index = 0; 

int main(int argc, char *argv[])
{
    char hostname[1024];
    gethostname(hostname, sizeof hostname);
    debug_hostname(hostname);

    int opt, return_code = EXIT_FAILURE;
    char *input_path = NULL;
    char *output_path = NULL;

    int v_counter = 0;

    struct stat buf;

    /* open output channel */
    FILE* standardout = fopen("stdout", "w");
    /* Parse short options */
    while((opt = getopt(argc, argv, "hve:")) != -1) {
        switch(opt) {
            case 'h':
                /* The help menu was selected */
                USAGE(argv[0]);
                exit(EXIT_SUCCESS);
                break;

            case 'v':
                v_counter++;
                break;

            case 'e':
                if(!strcmp(optarg, "UTF-16LE")){
                    debug_output_file_encoding("UTF-16LE");
                    to_UTF16_LE = true;
                }

                else if(!strcmp(optarg, "UTF-16BE")){
                    debug_output_file_encoding("UTF-16BE");
                    to_UTF16_BE = true;
                }

                else if(!strcmp(optarg, "UTF-8")){
                    debug_output_file_encoding("UTF-8");
                    to_UTF8 = true;
                }

                else{
                    exit(EXIT_FAILURE);
                }
                break;

            case '?':
                /* Let this case fall down to default;
                 * handled during bad option.
                 */
            default:
                /* A bad option was provided. */
                USAGE(argv[0]);
                exit(EXIT_FAILURE);
                break;
        }
    }
    /* Get position arguments */
    if(optind < argc && (argc - optind) == 2) {
        input_path = argv[optind++];
        output_path = argv[optind++];
    } else {
        if((argc - optind) <= 0) {
            fprintf(standardout, "Missing INPUT_FILE and OUTPUT_FILE.\n");
        } else if((argc - optind) == 1) {
            fprintf(standardout, "Missing OUTPUT_FILE.\n");
        } else {
            fprintf(standardout, "Too many arguments provided.\n");
        }
        USAGE(0[argv]);
        exit(EXIT_FAILURE);
    }
    /* Make sure all the arguments were provided */
    if(input_path != NULL || output_path != NULL) {
        int input_fd = -1, output_fd = -1;
        bool success = false;
        switch(validate_args(input_path, output_path)) {
                case VALID_ARGS:
                    /* Attempt to open the input file */
                    if((input_fd = open(input_path, O_RDONLY)) < 0) {
                        fprintf(standardout, "Failed to open the file %s\n", input_path);
                        perror(NULL);
                        goto conversion_done;
                    }
                    /* Delete the output file if it exists; Don't care about return code. */
                    unlink(output_path);
                    /* Attempt to create the file */
                    if((output_fd = open(output_path, O_CREAT | O_WRONLY,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) < 0) {
                        /* Tell the user that the file failed to be created */
                        fprintf(standardout, "Failed to open the file %s\n", input_path);
                        perror(NULL);
                        goto conversion_done;
                    }

                    stat(input_path, &buf);

                    debug_input_file_name(basename(input_path));
                    debug_inode_number((int)buf.st_ino);
                    debug_device_number((int)buf.st_dev);
                    debug_file_size((int)buf.st_size);
                    debug_output_file_name(basename(output_path));

                    /* Start the conversion */
                    success = convert(input_fd, output_fd);
conversion_done:
                    if(success) {
                        /* We got here so it must of worked right? */
                        return_code = EXIT_SUCCESS;
                    } else {
                        /* Conversion failed; clean up */
                        if(output_fd < 0 && input_fd >= 0) {
                            close(input_fd);
                        }
                        if(output_fd >= 0) {
                            close(output_fd);
                            unlink(output_path);
                        }
                        /* Just being pedantic... */
                        return_code = EXIT_FAILURE;
                        
                    }
                    break;
                case SAME_FILE:
                    fprintf(standardout, "The output file %s was not created. Same as input file.\n", output_path);
                    break;
                case FILE_DNE:
                    fprintf(standardout, "The input file %s does not exist.\n", input_path);
                    break;
                default:
                    fprintf(standardout, "An unknown error occurred\n");
                    break;
        }
    } else {
        /* Alert the user what was not set before quitting. */
        if((input_path  == NULL)) {
            fprintf(standardout, "INPUT_FILE was not set.\n");
        }
        if((output_path  == NULL)) {
            fprintf(standardout, "OUTPUT_FILE was not set.\n");
        }
        // Print out the program usage
        USAGE(argv[0]);
    }

    /* this part is for verbose print */
    if(v_counter == 0){
        fprintf(stderr, "%s\n", "The output file was successfully created" );
    }

    if(v_counter == 1){
        v_print();
    }

    else if(v_counter == 2){
        vv_print();
    }

    else if(v_counter >= 3){
        vvv_print();
    }

    return return_code;
}

int validate_args(const char *input_path, const char *output_path)
{
    int return_code = FAILED;
    struct stat buf_input;
    struct stat buf_output;

    stat(input_path, &buf_input);
    stat(output_path, &buf_output);

    /* Make sure both strings are not NULL */
    if(input_path != NULL && output_path != NULL) {
        /* Check to see if the the input and output are two different files. */
        if((buf_input.st_dev==buf_output.st_dev && buf_input.st_ino != buf_output.st_ino) || (buf_input.st_dev!=buf_output.st_dev)){
        //if(strcmp(input_path, output_path) != 0) {
            /* Check to see if the input file exists */
            struct stat *sb = malloc(sizeof(struct stat));
            /* zero out the memory */
            memset(sb, 0, sizeof(struct stat));
            /* now check to see if the file exists */
            if(stat(input_path, sb) == -1) {
                /* something went wrong */
                if(errno == ENOENT) {
                    /* File does not exist. */
                    return_code = FILE_DNE;
                } else {
                    /* No idea what the error is. */
                    perror(NULL);
                }
            } else {
                return_code = VALID_ARGS;
            }

            free(sb);
        }

    }
    return return_code;
}

bool convert(const int input_fd, const int output_fd)
{
    bool success = false;
    if(input_fd >= 0 && output_fd >= 0) {
        /* UTF-8 encoded text can be at most 4-bytes */
        unsigned char bytes[4];
        auto unsigned char read_value;
        auto size_t count = 0; // # of bytes!!!
        auto int safe_param = SAFE_PARAM;// DO NOT DELETE, PROGRAM WILL BE UNSAFE //
        void* saftey_ptr = &safe_param;
        auto ssize_t bytes_read;
        bool encode = false;

        unsigned char bom_bytes[3];
        bool is_UTF8 = false;
        bool is_UTF16LE = false;
        bool is_UTF16BE = false;

        /* check the BOM of input file */
        if(read(input_fd, &bom_bytes, 3)){
            if(bom_bytes[0] == 0xEF && bom_bytes[1] == 0xBB && bom_bytes[2] == 0xBF){
                is_UTF8 = true;
                debug_input_file_encoding("UTF-8");
            }
            else if(bom_bytes[0] == 0xFF && bom_bytes[1] == 0xFE){
                is_UTF16LE = true;
                debug_input_file_encoding("UTF-16LE");
            }
            else if(bom_bytes[0] == 0xFE && bom_bytes[1] == 0xFF){
                is_UTF16BE = true;
                debug_input_file_encoding("UTF-16BE");
            }
            else{
                fprintf(stderr, "%s\n", "Invalid file.");
                exit(EXIT_FAILURE);
            }
        }
        else{
            fprintf(stderr, "%s\n", "Invalid file.");
            exit(EXIT_FAILURE);
        }
        
        lseek(input_fd, -3, SEEK_CUR);

        if(is_UTF8){
            /* Read in UTF-8 Bytes */
            while((bytes_read = read(input_fd, &read_value, 1)) == 1) {
                /* Mask the most significant bit of the byte */
                unsigned char masked_value = read_value & 0x80;
                if(masked_value == 0x80) { //means more than one byte
                    if((read_value & UTF8_4_BYTE) == UTF8_4_BYTE ||
                       (read_value & UTF8_3_BYTE) == UTF8_3_BYTE ||
                       (read_value & UTF8_2_BYTE) == UTF8_2_BYTE) {
                        // Check to see which byte we have encountered
                        if(count == 0) {
                            bytes[count++] = read_value;
                        } else {
                            /* Set the file position back 1 byte */
                            if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                                /*Unsafe action! Increment! */
                                safe_param = *(int*)++saftey_ptr;
                                /* failed to move the file pointer back */
                                perror("NULL");
                                goto conversion_done;
                            }
                            /* Encode the current values into UTF-16LE */
                            encode = true;
                        }
                    } else if((read_value & UTF8_CONT) == UTF8_CONT) {
                        /* continuation byte */
                        bytes[count++] = read_value;
                    }
                } else { //one byte
                    if(count == 0) {
                        /* US-ASCII */
                        bytes[count++] = read_value;
                        encode = true;
                    } else {
                        /* Found an ASCII character but theres other characters
                         * in the buffer already.
                         * Set the file position back 1 byte.
                         */
                        if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                            /*Unsafe action! Increment! */
                            safe_param = *(int*) ++saftey_ptr;
                            /* failed to move the file pointer back */
                            perror("NULL");
                            goto conversion_done;
                        }
                        /* Encode the current values into UTF-16LE */
                        encode = true;
                    }
                }
                /* If its time to encode do it here */
                if(encode) {
                    int i, value = 0; //value is codepoint
                    bool isAscii = false;
                    for(i=0; i < count; i++) {
                        if(i == 0) {
                            if((bytes[i] & UTF8_4_BYTE) == UTF8_4_BYTE) {
                                value = bytes[i] & 0x7;
                                input[array_index] = (bytes[0]<<24) | (bytes[1]<<16)| (bytes[2]<<8) | bytes[3];
                            } else if((bytes[i] & UTF8_3_BYTE) == UTF8_3_BYTE) {
                                value =  bytes[i] & 0xF;
                                input[array_index] = (bytes[0]<<16) | (bytes[1]<<8)| bytes[2];
                            } else if((bytes[i] & UTF8_2_BYTE) == UTF8_2_BYTE) {
                                value =  bytes[i] & 0x1F;
                                input[array_index] = (bytes[0]<<8) | bytes[1];
                            } else if((bytes[i] & 0x80) == 0) {
                                /* Value is an ASCII character */
                                value = bytes[i];
                                input[array_index] = bytes[0];
                                isAscii = true;

                                /* Store ASCII char into ASCII array */
                                ASCII[array_index] = value;

                            } else {
                                /* Marker byte is incorrect */
                                goto conversion_done;
                            }
                        } else {
                            if(!isAscii) {
                                value = (value << 6) | (bytes[i] & 0x3F);

                                /* Store ASCII char into ASCII array */
                                ASCII[array_index] = '\0';

                            } else {
                                /* How is there more bytes if we have an ascii char? */
                                goto conversion_done;
                            }
                        }
                    }
                    //so far, we got code point

                    /* if machine is LE */
                    /* if to utf8, then just copy the file bytes by bytes into output file*/
                    if(check_indianness()==0 && to_UTF8){
                        output[array_index] = read_value;
                        if(!safe_write(output_fd, &read_value, 1)){
                            goto conversion_done;
                        }
                    }

                    /* if encoding the value to UTF-16LE */
                    if(check_indianness()==0 && to_UTF16_LE){
                        /* Handle the value if its a surrogate pair*/
                        if(value >= SURROGATE_PAIR) {
                            int vprime = value - SURROGATE_PAIR;
                            int w1 = (vprime >> 10) + 0xD800;
                            int w2 = (vprime & 0x3FF) + 0xDC00;

                             /* reverse all the bytes of w1, w2 */
                            int w1_reverse = reverse_bytes(w1);
                            int w2_reverse = reverse_bytes(w2);

                            output[array_index] = (w1_reverse << 16) | w2_reverse;

                            /* write the surrogate pair to file */
                            if(!safe_write(output_fd, &w1, CODE_UNIT_SIZE)) {
                                
                                goto conversion_done;
                            }
                            if(!safe_write(output_fd, &w2, CODE_UNIT_SIZE)) {
                                
                                goto conversion_done;
                            }
                        } else {
                            /* reverse the bytes of value */
                            int value_reverse = reverse_bytes(value);

                            output[array_index] = value_reverse;

                            /* write the code point to file */
                            if(!safe_write(output_fd, &value, CODE_UNIT_SIZE)) {
                                
                                goto conversion_done;
                            }
                        }
                        /* Done encoding the value to UTF-16LE */
                    }

                        

                    /* if encoding the value to UTF-16BE */
                    //do sth here
                    if(check_indianness()==0 && to_UTF16_BE){
                        /* Handle the value if its a surrogate pair*/
                        if(value >= SURROGATE_PAIR) {
                            int vprime = value - SURROGATE_PAIR;
                            int w1 = (vprime >> 10) + 0xD800;
                            int w2 = (vprime & 0x3FF) + 0xDC00;

                            /* reverse all the bytes of w1, w2 */
                            int w1_reverse = reverse_bytes(w1);
                            int w2_reverse = reverse_bytes(w2);

                            output[array_index] = (w1 << 16) | w2;

                            /* write the surrogate pair to file */
                            if(!safe_write(output_fd, &w1_reverse, CODE_UNIT_SIZE)) {
                                
                                goto conversion_done;
                            }
                            if(!safe_write(output_fd, &w2_reverse, CODE_UNIT_SIZE)) {
                                
                                goto conversion_done;
                            }
                        } else {
                            /* reverse the bytes of value */
                            int value_reverse = reverse_bytes(value);

                            output[array_index] = value;

                            /* write the code point to file */
                            if(!safe_write(output_fd, &value_reverse, CODE_UNIT_SIZE)) {
                                
                                goto conversion_done;
                            }
                        }
                        /* Done encoding the value to UTF-16BE */
                    }


                    /* if machine is BE */
                    /* if encoding the value to UTF-16LE */
                    //do sth
                    /* Done encoding the value to UTF-16LE */

                    /* if encoding the value to UTF-16BE */
                    //do sth here
                    /* Done encoding the value to UTF-16BE */

                    /* Store num of bytes and codepoint */
                    num_of_bytes[array_index] = count;
                    codepoint[array_index] = value;

                    encode = false;
                    count = 0;
                    array_index++;
                }
            }
        }

        if(is_UTF16LE){
            
            while((bytes_read = read(input_fd, &read_value, 1)) == 1) {
                bytes[count++] = read_value;

                if(to_UTF16_LE){
                        
                    if(!safe_write(output_fd, &read_value, 1)) {                                                     
                        goto conversion_done;
                    }
                    
                }

                if(to_UTF16_BE){
                    char first_char = read_value;
                    read(input_fd, &read_value, 1);
                    if(!safe_write(output_fd, &read_value, 1)) {
                        
                        goto conversion_done;
                    }
                    if(!safe_write(output_fd, &first_char, 1)) {
                        
                        goto conversion_done;
                    }
                        
                    
                }
                
                if(count == 2){
                    //check whether it is a surrogate pair
                    int value_16 = bytes[1]<<8 | bytes[0];
                    if(value_16 >= 0xD800 && value_16 <= 0xDBFF){
                        //it is surrogate pair, let the program keep running
                    } else {
                        //it is a two-bytes char, do sth

                        encode = true;
                    }
                }

                if(count == 4){
                    //do sth to deal with the surrogate pair
                    encode = true;

                }

                if(encode){
                    int value = 0; //value is codepoint
                    int A = 0x0;
                    int A_prime = 0x0;
                    int B = 0x0;
                    int Z = 0x0;
                    int letter = 0x0;

                    if(count == 2){
                        value = bytes[1]<<8 | bytes[0];
                    }

                    else if(count == 4){
                        int w1 = bytes[1]<<8 | bytes[0];
                        int w2 = bytes[3]<<8 | bytes[2];

                        int vprime_high = (w1 - 0xD800) << 10;
                        int vprime_low = w2 - 0xDC00;
                        int vprime = vprime_high | vprime_low;
                        value = vprime + SURROGATE_PAIR;
                    }


                    //ok.... now i have codepoint


                    if(to_UTF8){
                        //1 byte
                        if(value >= 0x0 && value <= 0x7F){
                            /* write the code point to file */
                            if(!safe_write(output_fd, &value, 1)) {
                                
                                goto conversion_done;
                            }
                        }

                        //2 bytes
                        else if(value >= 0x80 && value <= 0x7FF){
                            Z = value;
                            A_prime = Z & 0xFFC0;
                            A = A_prime >> 6;
                            B = Z & 0x3F;
                            letter = ((A | 0xC0) << 8) | (B | 0x80);
                            //so far we got the UTF-8 code unit, next write to file

                            //if little indian, we need to reverse the letter bytes first then write
                            if(check_indianness() == 0){
                                int letter_reverse = ((B | 0x80) << 8) | (A | 0xC0);
                                /* write the code point to file */
                                if(!safe_write(output_fd, &letter_reverse, 2)) {
                                    
                                    goto conversion_done;
                                }
                            } else { //big indianness
                                if(!safe_write(output_fd, &letter, 2)) {
                                    
                                    goto conversion_done;
                                }
                            }

                        }

                        //3 bytes
                        else if(value >= 0x800 && value <= 0xFFFF){
                            Z = value;
                            A_prime = Z & 0xFFC0;
                            A = A_prime >> 6;
                            B = Z & 0x3F;
                            letter = B | 0x80; //LSB
                            char LSB = letter;

                            Z = A;
                            A_prime = Z & 0xFFC0;
                            A = A_prime >> 6;
                            B = Z & 0x3F;
                            letter = ((A | 0xE0) << 16) | ((B | 0x80) << 8) | LSB;

                            if(check_indianness() == 0){
                                int letter_reverse = (LSB << 16) | ((B | 0x80) << 8) | (A | 0xE0);
                                /* write the code point to file */
                                if(!safe_write(output_fd, &letter_reverse, 3)) {
                                    
                                    goto conversion_done;
                                }
                            } else { //big indianness
                                if(!safe_write(output_fd, &letter, 3)) {
                                    
                                    goto conversion_done;
                                }
                            }


                        }

                        //4 bytes
                        else if(value >= 0x10000 && value <= 0x1FFFFF){
                            Z = value;
                            A_prime = Z & 0xFFFFC0;
                            A = A_prime >> 6;
                            B = Z & 0x3F;
                            letter = B | 0x80; //LSB
                            char LSB = (char)letter;

                            Z = A;
                            A_prime = Z & 0xFFC0;
                            A = A_prime >> 6;
                            B = Z & 0x3F;
                            letter = B | 0x80; //2nd LSB

                            Z = A;
                            A_prime = Z & 0xFFC0;
                            A = A_prime >> 6;
                            B = Z & 0x3F;
                            letter = ((A | 0xF0) << 24) | ((B | 0x80) << 16) | (letter << 8) | LSB;

                            if(check_indianness() == 0){
                                int letter_reverse = ((LSB << 24) | (letter << 16) | ((B | 0x80) << 8) | (A | 0xF0));
                                /* write the code point to file */
                                if(!safe_write(output_fd, &letter_reverse, 4)) {
                                    
                                    goto conversion_done;
                                }
                            } else { //big indianness
                                if(!safe_write(output_fd, &letter, 4)) {
                                    
                                    goto conversion_done;
                                }
                            }
                        }

                    
                    }

                    
                    /* Store num of bytes and codepoint */
                    num_of_bytes[array_index] = count;
                    codepoint[array_index] = value;  

                    /*dont forget to do sth when you get here*/
                    encode = false;
                    count = 0;
                    array_index++;
                }

            }
            
        }

        if(is_UTF16BE){

            while((bytes_read = read(input_fd, &read_value, 1)) == 1) {
                bytes[count++] = read_value;

                if(to_UTF16_BE){
                        
                    if(!safe_write(output_fd, &read_value, 1)) {                                                     
                        goto conversion_done;
                    }
                    
                }

                if(to_UTF16_LE){
                    char first_char = read_value;
                    read(input_fd, &read_value, 1);
                    if(!safe_write(output_fd, &read_value, 1)) {
                        
                        goto conversion_done;
                    }
                    if(!safe_write(output_fd, &first_char, 1)) {
                        
                        goto conversion_done;
                    }
                        
                    
                }
                
                if(count == 2){
                    //check whether it is a surrogate pair
                    int value_16 = bytes[0]<<8 | bytes[1];
                    if(value_16 >= 0xD800 && value_16 <= 0xDBFF){
                        //it is surrogate pair, let the program keep running
                    } else {
                        //it is a two-bytes char, do sth

                        encode = true;
                    }
                }

                if(count == 4){
                    //do sth to deal with the surrogate pair
                    encode = true;

                }

                if(encode){
                    int value = 0; //value is codepoint
                    int A = 0x0;
                    int A_prime = 0x0;
                    int B = 0x0;
                    int Z = 0x0;
                    int letter = 0x0;

                    if(count == 2){
                        value = bytes[0]<<8 | bytes[1];
                    }

                    else if(count == 4){
                        int w1 = bytes[0]<<8 | bytes[1];
                        int w2 = bytes[2]<<8 | bytes[3];

                        int vprime_high = (w1 - 0xD800) << 10;
                        int vprime_low = w2 - 0xDC00;
                        int vprime = vprime_high | vprime_low;
                        value = vprime + SURROGATE_PAIR;
                    }


                    //ok.... now i have codepoint
                    //printf("0x%04x\n", value);


                    if(to_UTF8){
                        //1 byte
                        if(value >= 0x0 && value <= 0x7F){
                            /* write the code point to file */
                            if(!safe_write(output_fd, &value, 1)) {
                                
                                goto conversion_done;
                            }
                        }

                        //2 bytes
                        else if(value >= 0x80 && value <= 0x7FF){
                            Z = value;
                            A_prime = Z & 0xFFC0;
                            A = A_prime >> 6;
                            B = Z & 0x3F;
                            letter = ((A | 0xC0) << 8) | (B | 0x80);
                            //so far we got the UTF-8 code unit, next write to file

                            //if little indian, we need to reverse the letter bytes first then write
                            if(check_indianness() == 0){
                                int letter_reverse = ((B | 0x80) << 8) | (A | 0xC0);
                                /* write the code point to file */
                                if(!safe_write(output_fd, &letter_reverse, 2)) {
                                    
                                    goto conversion_done;
                                }
                            } else { //big indianness
                                if(!safe_write(output_fd, &letter, 2)) {
                                    
                                    goto conversion_done;
                                }
                            }

                        }

                        //3 bytes
                        else if(value >= 0x800 && value <= 0xFFFF){
                            Z = value;
                            A_prime = Z & 0xFFC0;
                            A = A_prime >> 6;
                            B = Z & 0x3F;
                            letter = B | 0x80; //LSB
                            char LSB = letter;

                            Z = A;
                            A_prime = Z & 0xFFC0;
                            A = A_prime >> 6;
                            B = Z & 0x3F;
                            letter = ((A | 0xE0) << 16) | ((B | 0x80) << 8) | LSB;

                            if(check_indianness() == 0){
                                int letter_reverse = (LSB << 16) | ((B | 0x80) << 8) | (A | 0xE0);
                                /* write the code point to file */
                                if(!safe_write(output_fd, &letter_reverse, 3)) {
                                    
                                    goto conversion_done;
                                }
                            } else { //big indianness
                                if(!safe_write(output_fd, &letter, 3)) {
                                    
                                    goto conversion_done;
                                }
                            }


                        }

                        //4 bytes
                        else if(value >= 0x10000 && value <= 0x1FFFFF){
                            Z = value;
                            A_prime = Z & 0xFFFFC0;
                            A = A_prime >> 6;
                            B = Z & 0x3F;
                            letter = B | 0x80; //LSB
                            char LSB = (char)letter;

                            Z = A;
                            A_prime = Z & 0xFFC0;
                            A = A_prime >> 6;
                            B = Z & 0x3F;
                            letter = B | 0x80; //2nd LSB

                            Z = A;
                            A_prime = Z & 0xFFC0;
                            A = A_prime >> 6;
                            B = Z & 0x3F;
                            letter = ((A | 0xF0) << 24) | ((B | 0x80) << 16) | (letter << 8) | LSB;

                            if(check_indianness() == 0){
                                int letter_reverse = ((LSB << 24) | (letter << 16) | ((B | 0x80) << 8) | (A | 0xF0));
                                /* write the code point to file */
                                if(!safe_write(output_fd, &letter_reverse, 4)) {
                                    
                                    goto conversion_done;
                                }
                            } else { //big indianness
                                if(!safe_write(output_fd, &letter, 4)) {
                                    
                                    goto conversion_done;
                                }
                            }
                        }

                    
                    }

                    
                        

                    /* Store num of bytes and codepoint */
                    num_of_bytes[array_index] = count;
                    codepoint[array_index] = value;  

                    /*dont forget to do sth when you get here*/
                    encode = false;
                    count = 0;
                    array_index++;
                }

            }
            
        }
            
        /* If we got here the operation was a success! */
        success = true;
    }
conversion_done:
    return success;
}

int check_indianness(){
    unsigned int i = 1;
    char *c = (char*)&i; //Convert the LSB into a char
    // if(*c) {
    //     printf("%s\n", "little indian");
    //     return 0;
    // } else {
    //     printf("%s\n", "big indian");
    //     return 1;
    // }
    return (*c ? 0 : 1);
}

int reverse_bytes(int value){
    int tmp = value & 0x1100;
    value = value << 8;
    tmp = tmp >> 8;
    value = value | tmp;
    return value;
}

void v_print(){
    int i;

    fprintf(stderr, "%s\n", "+---------------+-----------------------+-----------------------+");
    fprintf(stderr, "%s\t%s\t%s\t%s\t%s\t%s\t%s\n", "|", "ASCII", "|", "# of bytes", "|", "codepoint", "|");
    fprintf(stderr, "%s\n", "+---------------+-----------------------+-----------------------+");

    for(i=0; i<array_index; i++){
        if(ASCII[i] == '\0'){
            fprintf(stderr, "%s\t%s\t%s\t%d\t\t%s\tU+%04x\t\t%s\n", "|", "NONE", "|", num_of_bytes[i], "|", codepoint[i], "|");
            fprintf(stderr, "%s\n", "+---------------+-----------------------+-----------------------+");
        }
        else if(ASCII[i] != '\n'){
            fprintf(stderr, "%s\t%c\t%s\t%d\t\t%s\tU+%04x\t\t%s\n", "|", ASCII[i], "|", num_of_bytes[i], "|", codepoint[i], "|");
            fprintf(stderr, "%s\n", "+---------------+-----------------------+-----------------------+");
        }
    }
}

void vv_print(){
    int i;

    fprintf(stderr, "%s\n", "+---------------+-----------------------+-----------------------+---------------+");
    fprintf(stderr, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", "|", "ASCII", "|", "# of bytes", "|", "codepoint", "|", "input", "|");
    fprintf(stderr, "%s\n", "+---------------+-----------------------+-----------------------+---------------+");

    for(i=0; i<array_index; i++){
        if(ASCII[i] == '\0'){
            fprintf(stderr, "%s\t%s\t%s\t%d\t\t%s\tU+%04x\t\t%s\t0x%x\t%s\n", "|", "NONE", "|", num_of_bytes[i], "|", codepoint[i], "|", input[i], "|");
            fprintf(stderr, "%s\n", "+---------------+-----------------------+-----------------------+---------------+");
        }
        else if(ASCII[i] != '\n'){
            fprintf(stderr, "%s\t%c\t%s\t%d\t\t%s\tU+%04x\t\t%s\t0x%x\t%s\n", "|", ASCII[i], "|", num_of_bytes[i], "|", codepoint[i], "|", input[i], "|");
            fprintf(stderr, "%s\n", "+---------------+-----------------------+-----------------------+---------------+");
        }
    }
}

void vvv_print(){
    int i;

    fprintf(stderr, "%s\n", "+---------------+-----------------------+-----------------------+---------------+---------------+");
    fprintf(stderr, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", "|", "ASCII", "|", "# of bytes", "|", "codepoint", "|", "input", "|", "output", "|");
    fprintf(stderr, "%s\n", "+---------------+-----------------------+-----------------------+---------------+---------------+");

    for(i=0; i<array_index; i++){
        if(ASCII[i] == '\0'){
            fprintf(stderr, "%s\t%s\t%s\t%d\t\t%s\tU+%04x\t\t%s\t0x%x\t%s\t0x%x\t%s\n", "|", "NONE", "|", num_of_bytes[i], "|", codepoint[i], "|", input[i], "|", output[i], "|");
            fprintf(stderr, "%s\n", "+---------------+-----------------------+-----------------------+---------------+---------------+");
        }
        else if(ASCII[i] != '\n'){
            fprintf(stderr, "%s\t%c\t%s\t%d\t\t%s\tU+%04x\t\t%s\t0x%x\t%s\t0x%x\t%s\n", "|", ASCII[i], "|", num_of_bytes[i], "|", codepoint[i], "|", input[i], "|", output[i], "|");
            fprintf(stderr, "%s\n", "+---------------+-----------------------+-----------------------+---------------+---------------+");
        }
    }
}

bool safe_write(int output_fd, void *value, size_t size){
    bool success = true;
    ssize_t bytes_written;

    if((bytes_written = write(output_fd, value, size)) != size) {
        /* The write operation failed */
        fprintf(stdout, "Write to file failed. Expected %zu bytes but got %zd\n", size, bytes_written);
    }
    return success;

}
