#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strlen
#include <ctype.h> // isxdigit

#define PRINT_LEN ((offset)+(length))

long get_file_length(FILE*);
int decimal_parser(const char*);
inline int xchar2int(char);

int main(int argc, char *argv[])
{
    int i,nl,ln,tmp,len,no_option_cnt=0,targetlen,ti,returnval=0;
    char filename[1024]={0};
    unsigned char* target = NULL;
    int offset=0, length=-1;
    FILE* fp=NULL;
    unsigned char c, ch[16];
    int isstrformat = 0;
    for(i=0; i<16; i++)
        ch[i]=255;
    if(argc<=1)
    {
        puts("findx find the target bytes in the file\n");
        puts("Usage: findx <filename> <target> [-s offset] [-n length]\n");
        puts("\tfilename \tThe file name or path to print.");
        puts("\ttarget   \tFind the target bytes in hex");
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
                        fprintf(stderr,"findx : need more arg for option '%s'\n",argv[i]);
                        returnval=1;
                        goto FreeMelloc;
                    }
                    offset=decimal_parser(argv[++i]); continue;
                case 'n':
                    if(i+1>=argc)
                    {
                        fprintf(stderr,"findx : need more arg for option '%s'\n",argv[i]);
                        returnval=1;
                        goto FreeMelloc;
                    }
                    length=decimal_parser(argv[++i]); continue;
                default: break;
            }
            fprintf(stderr,"findx : unknown option '%s'\n",argv[i]);
            returnval=1;
            goto FreeMelloc;
        }
        else
        {
            no_option_cnt++;
            if (no_option_cnt==1)
            {
                strncpy(filename, argv[i], sizeof(filename));
                fp=fopen(filename, "rb");
                if(!fp)
                {
                    fprintf(stderr,"findx : file '%s' is not exist.\n",filename);
                    returnval=1;
                    goto FreeMelloc;
                }
            }
            else if (no_option_cnt==2)
            {
                int c=0,prefix=0,isoddlen,ti=0;
                int slen=strlen(argv[i]);
                /* check format */
                if (slen>=2 && argv[i][0]=='0' && (argv[i][1]=='x' || argv[i][1]=='X'))
                {
                    prefix=2;
                    targetlen=slen-2;
                }
                else
                {
                    targetlen=slen;
                }
                for(c=prefix; c<slen; ++c)
                {
                    if (!isxdigit(argv[i][c])) {
                        fprintf(stderr,"findx : invalid hex/binary format for target '%s'\n",argv[i]);
                        fprintf(stderr,"findx : try string format\n");
                        break;
                        //returnval=1;
                        //goto FreeMelloc;
                    }
                }
                // string format
                if (c != slen)
                {
                    for(c=prefix; c<slen; ++c)
                    {
                        if (!isprint(argv[i][c])) {
                            fprintf(stderr,"findx : invalid string format(not printable) for target '%s'\n",argv[i]);
                            returnval=1;
                            goto FreeMelloc;
                        }
                    }
                    targetlen=slen;
                    target=(unsigned char*)malloc(targetlen);
                    memcpy(target, argv[i], targetlen);
                    isstrformat = 1;
                }
                else
                {
                    isoddlen=targetlen&0x01;
                    targetlen+=isoddlen;
                    targetlen/=2;
                    target=(unsigned char*)malloc(targetlen);
                    if (!target)
                    {
                        fprintf(stderr,"findx : cannot allocate the target bytes '%s'\n",argv[i]);
                        returnval=1;
                        goto FreeMelloc;
                    }
                    if (isoddlen)
                    {
                        target[0]=xchar2int(argv[i][prefix]);
                        ti=1;
                        prefix+=1;
                    }
                    for (c=prefix; c+1<slen; c+=2)
                    {
                        target[ti++]=(xchar2int(argv[i][c])<<4)+xchar2int(argv[i][c+1]);
                    }
                }
            }// end if no_option_cnt
        }
    }// end for
    if(!fp)
    {
        fprintf(stderr,"findx : you should specified a file for print.\n");
        returnval=1;
        goto FreeMelloc;
    }
    if(targetlen<=0)
    {
        fprintf(stderr,"findx : you should specified target bytes for find.\n");
        returnval=1;
        goto FreeMelloc;
    }
    if(length<0)
    {
        /* auto length */
        length=get_file_length(fp)-offset;
    }
    if(offset<0 || PRINT_LEN>get_file_length(fp))
    {
        fprintf(stderr,"findx : search range(%d~%d) out of file size(%ld).\n",offset,PRINT_LEN,get_file_length(fp));
        returnval=1;
        goto FreeMelloc;
    }
    fseek(fp,offset,SEEK_SET);
    printf("File name: %s, File size: %ld\nOffset: %d(%08Xh), search length: %d(%08Xh)\n",
        filename,get_file_length(fp),offset,offset,length,length);
    printf("Target bytes:");
    for(i=0; i<targetlen; ++i)
        printf("%02X ",target[i]);
    printf("\n");
    if (isstrformat)
    {
        printf("Target string:'%s'\n", target);
    }
    ti=0;
    for(i=0; i<length; i++)
    {
        c=fgetc(fp);
        if (c == EOF)
            break;
        if (c==target[ti])
        {
            ++ti;
            if (ti==targetlen)
            {
                printf("match @ %d (0x%08Xh)\n",i+offset-targetlen+1,i+offset-targetlen+1);
                ti = 0;
            }
        }
        else
        {
            if (ti)
            {
                /* when check fail, seek to first check pos */
                fseek(fp,-ti,SEEK_CUR);
                i = i-ti;
                ti = 0;
            }
        }
    }
    printf("%d bytes searched!\n",length);

FreeMelloc:
    if (fp)
    {
        fclose(fp);
        fp=NULL;
    }
    if (target)
    {
        free(target);
        target=NULL;
    }

    return returnval;
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

int decimal_parser(const char* str)
{
    int reval = 0;
    if(!str) return 0;
    else if(strlen(str)>2 && str[0]=='0' && (str[1]=='x'||str[1]=='X'))
        sscanf(str, "%x", &reval);
    else
        reval=atoi(str);
    return reval;
}

inline int xchar2int(char c)
{
    int v=c-'0';
    if (!isxdigit(c))
        return 0;
    if (v>9)
    {
        v=c-'A';
        if (v>5)
            v=c-'a';
        v+=10;
    }
    return v;
}
