#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strlen
#include <ctype.h> // isprint

#define PRINT_LEN ((offset)+(length))

long get_file_length(FILE*);
int decimal_parser(const char*);

int main(int argc, char *argv[])
{
    int i, nl, ln, tmp, len;
    char filename[1024]={0};
    int offset=0, length=-1;
    FILE* fp=NULL;
    unsigned char c, ch[16];
    for(i=0; i<16; i++)
        ch[i]=255;
    if(argc<=1)
    {
        puts("dumpx print the file in one-byte hex format.\n");
        puts("Usage: dumpx <filename> [-s offset] [-n length]\n");
        puts("\tfilename\tThe file name or path to print.");
        puts("\t-s offset\tSkip first <offset> bytes in the file.");
        puts("\t-n length\tHow many bytes should be printed.\n");
        return 0;
    }
    for(i=1; i<argc; ++i)
    {
        if(argv[i]!=NULL && strlen(argv[i])==2 && argv[i][0]=='-')
        {
            switch(argv[i][1])
            {
                case 's':
                    if(i+1>=argc)
                    {
                        fprintf(stderr, "dumpx : need more arg for option '%s'\n", argv[i]);
                        return 1;
                    }
                    offset=decimal_parser(argv[++i]); continue;
                case 'n':
                    if(i+1>=argc)
                    {
                        fprintf(stderr, "dumpx : need more arg for option '%s'\n", argv[i]);
                        return 1;
                    }
                    length=decimal_parser(argv[++i]); continue;
                default: break;
            }
            fprintf(stderr, "dumpx : unknown option '%s'\n", argv[i]);
            return 1;
        }
        else
        {
            strncpy(filename, argv[i], sizeof(filename));
            fp=fopen(filename, "rb");
            if(!fp)
            {
                fprintf(stderr, "dumpx : file '%s' is not exist.\n",filename);
                return 1;
            }
        }
    }
    if(!fp)
    {
        fprintf(stderr, "dumpx : you should specified a file for print.\n");
        return 1;
    }
    /* auto length */
    if(length<0)
    {
        length=get_file_length(fp)-offset;
    }
    if(PRINT_LEN>get_file_length(fp))
    {
        fprintf(stderr, "dumpx : print range(%d~%d) out of file size(%ld).\n",offset,PRINT_LEN,get_file_length(fp));
        return 1;
    }
    fseek(fp,offset,SEEK_SET);
    printf("File name: %s, File size: %ld\nOffset: %d(%08Xh), print length: %d(%08Xh)\n",
        filename, get_file_length(fp), offset, offset, length, length);
    printf("--offset-- ");
    for(i=0x0; i<=0xF; i++)
        printf("%02X ", i);
    printf("  ");
    for(i=0x0; i<=0xF; i++)
        printf("%X", i);
    nl=16;
    ln=(offset/16)*0x10;
    tmp=offset%16;
    len=length+16-((PRINT_LEN%16==0)?16:(PRINT_LEN%16));
    for(i=0; i<len; i++)
    {
        if(nl==16)
        {
            printf("\n%08Xh| ", ln);
            nl=tmp, ln+=0x10;
        }
        for(; tmp>0; tmp--)
            printf("   ");
        if(i<length)
        {
            c = fgetc(fp);
            if (c == EOF)
                break;
            printf("%02X ", c);
        }
        else
        {
            c = ' ';
            printf("   ");
        }
        ch[nl++] = c;
        if(nl==16)
        {
            printf("| ");
            for(nl=0; nl<16; nl++)
                // printable character in ASCII code
                //if(ch[nl]>0x7E || ch[nl]<0x20)
                if(isprint(ch[nl]))
                    putchar(ch[nl]);
                else
                    putchar(' ');
        }
    }
    puts(""); // nexline
    fclose(fp);
    return 0;
}

long get_file_length(FILE* fp)
{
    long curpos, length;
    curpos=ftell(fp);
    fseek(fp, 0, SEEK_END);
    length=ftell(fp);
    fseek(fp, curpos, SEEK_SET);
    return length;
}

int decimal_parser(const char* str) {
    int reval = 0;
    if(!str) return 0;
    else if(strlen(str)>2 && str[0]=='0' && (str[1]=='x'||str[1]=='X'))
        sscanf(str, "%x", &reval);
    else
        reval=atoi(str);
    return reval;
}

