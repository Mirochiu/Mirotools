#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strlen
#include <ctype.h> // isxdigit

#define PRINT_LEN ((offset)+(length))

long get_file_length(FILE*);
int decimal_parser(const char*);

int main(int argc, char *argv[])
{
    int i,nl,ln,tmp,len,no_option_cnt=0;
    int targetlen,ti,replacementlen,returnVal=0;
    char filename[1024]={0};
    unsigned char* target=NULL;
    unsigned char* replacement=NULL;
    int offset=0,length=-1;
    FILE* fp=NULL;
    unsigned char c, ch[16];
    for(i=0; i<16; i++)
        ch[i]=255;
    if(argc<=1)
    {
        puts("replacex find the target bytes in the file\n");
        puts("Usage: replacex <filename> <target> <replacement> [-s offset] [-n length]\n");
        puts("\tfilename   \tThe file name or path to print.");
        puts("\ttarget     \tFind the target string in the file");
        puts("\treplacement\tSpecified the replacement");
        puts("\t-s offset  \tSkip first <offset> bytes in the file.");
        puts("\t-n length  \tHow many bytes should be printed.\n");
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
                        fprintf(stderr,"replacex : need more arg for option '%s'\n",argv[i]);
                        returnVal=1;
                        goto FreeMelloc;
                    }
                    offset=decimal_parser(argv[++i]); continue;
                case 'n':
                    if(i+1>=argc)
                    {
                        fprintf(stderr,"replacex : need more arg for option '%s'\n",argv[i]);
                        returnVal=1;
                        goto FreeMelloc;
                    }
                    length=decimal_parser(argv[++i]); continue;
                default: break;
            }
            fprintf(stderr,"replacex : unknown option '%s'\n",argv[i]);
            return 1;
        }
        else
        {
            no_option_cnt++;
            if (no_option_cnt==1)
            {
                strncpy(filename,argv[i],sizeof(filename));
                fp=fopen(filename,"rb+");
                if(!fp)
                {
                    fprintf(stderr,"replacex : file '%s' is not exist.\n",filename);
                    returnVal=1;
                    goto FreeMelloc;
                }
            }
            else if (no_option_cnt==2)
            {
                targetlen=strlen(argv[i]);
                target=(unsigned char*)malloc(targetlen);
                memcpy(target,argv[i],targetlen);
            }
            else if (no_option_cnt==3)
            {
                replacementlen=strlen(argv[i]);
                replacement=(unsigned char*)malloc(targetlen); // replace way=memcpy
                memset(replacement,0,targetlen); // clear data
                memcpy(replacement,argv[i],replacementlen);
            }
            // end if no_option_cnt
        }
    }// end for
    if(!fp)
    {
        fprintf(stderr,"replacex : you should specified a file for print.\n");
        returnVal=1;
        goto FreeMelloc;
    }
    if(targetlen<=0)
    {
        fprintf(stderr,"replacex : you should specified target bytes for find.\n");
        returnVal=1;
        goto FreeMelloc;
    }
    if (replacementlen>targetlen)
    {
        fprintf(stderr,"replacex : cannot replace the replacement, because the length(%d) is larger than specified string length(%d).\n",
            replacementlen,targetlen);
        returnVal=1;
        goto FreeMelloc;
    }
    if(length<0)
    {
        /* auto length */
        length=get_file_length(fp)-offset;
    }
    if(offset<0 || PRINT_LEN>get_file_length(fp))
    {
        fprintf(stderr,"replacex : search range(%d~%d) out of file size(%ld).\n",offset,PRINT_LEN,get_file_length(fp));
        returnVal=1;
        goto FreeMelloc;
    }
    fseek(fp,offset,SEEK_SET);
    printf("File name: %s, File size: %ld\nOffset: %d(%08Xh), search length: %d(%08Xh)\n",
        filename,get_file_length(fp),offset,offset,length,length);
    printf("Target bytes:");
    for(i=0; i<targetlen; ++i)
        printf("%c(%02Xh) ",target[i],target[i]);
    printf("\n");
    ti=0;
    replaced_cnt=0;
    for(i=0; i<length; i++)
    {
        c = fgetc(fp);
        if (c == EOF)
            break;
        if (c==target[ti])
        {
            ++ti;
            if (ti==targetlen) {
                printf("match @ %d (0x%08Xh)\n",i+offset-targetlen+1,i+offset-targetlen+1);
                replaced_cnt++;
                printf("%d", ftell(fp));
                fseek(fp,-ti,SEEK_CUR);
                fwrite(replacement,1,ti,fp);
                printf("%d", ftell(fp));
                ti = 0;
            }
        }
        else
        {
            if (ti>1)
            {
                /* when check fail, seek to first check pos */
                fseek(fp,-ti,SEEK_CUR);
                i = i-ti;
            }
            ti = 0;
        }
    }
    printf("%d bytes checked, %d replaeced!\n",length,replaced_cnt);
    fflush(fp);
    fclose(fp);

FreeMelloc:
    if (target) {
        free(target);
        target = NULL;
    }
    if (replacement) {
        free(replacement);
        replacement = NULL;
    }    

    return returnVal;
}

long get_file_length(FILE* fp)
{
    long curpos,length;
    curpos=ftell(fp);
    fseek(fp,0,SEEK_END);
    length=ftell(fp);
    fseek(fp,curpos,SEEK_SET);
    return length;
}

int decimal_parser(const char* str)
{
    int reval = 0;
    if(!str) return 0;
    else if(strlen(str)>2 && str[0]=='0' && (str[1]=='x'||str[1]=='X'))
        sscanf(str,"%x",&reval);
    else
        reval=atoi(str);
    return reval;
}
