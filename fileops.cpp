#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <stdio.h>
#include <unistd.h> // read, lseek
#include <fcntl.h>  // open
#include <sys/types.h> // 
#include <errno.h>
#include <stdlib.h> // malloc

#define PRId64 "ld"

#define LOGV
//#define LOGV printf

typedef off64_t int64_t;

// read operator abstruct layer
typedef struct io_read_ops *io_read_ops_t;
typedef ssize_t (*read_operator_t)   (io_read_ops_t, void    *buf,   size_t  length);
typedef off64_t (*seek_operator_t)   (io_read_ops_t, off64_t offset, int     whence);
typedef struct io_read_ops {
    read_operator_t read;
    seek_operator_t seek;
} ReaderOps;
// file read operator abstruct layer
typedef struct io_reader *ReaderHandle;
typedef void (*io_close_t) (ReaderHandle hdl);
typedef struct io_reader {
    io_read_ops data; // must be the first member
    io_close_t close;
} Reader;

/////////////////////////////////////////////////////////////////////////////
// binary file implementation
/////////////////////////////////////////////////////////////////////////////
typedef struct bfile_reader {
    struct io_reader file; // must be the first member
    int fd;
} *bfile_reader_t;

/////////////////////////////////////////////////////////////////////////////
static int bfile_io_open(bfile_reader_t f, char* path) {
    int fd_read_flag = O_RDONLY;
#ifdef O_LARGEFILE
    fd_read_flag |= O_LARGEFILE;
#endif
#ifdef O_BINARY
    fd_read_flag |= O_BINARY;
#endif
    LOGV("%s %d\n",__FUNCTION__,__LINE__);
    if (f) {
        f->fd = open(path, fd_read_flag);
        return f->fd;
    }
    return -1;
}

/////////////////////////////////////////////////////////////////////////////
static void bfile_io_close(ReaderHandle hdl) {
    bfile_reader* f = (bfile_reader*)hdl;
    LOGV("%s %d\n",__FUNCTION__,__LINE__);
    if (f) {
        if (f->fd != -1) {
            close(f->fd);
            f->fd = -1;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
static ssize_t bfile_io_read(io_read_ops_t fd, void *buf, size_t length) {
    bfile_reader* f = (bfile_reader*)fd;
    LOGV("%s %d\n",__FUNCTION__,__LINE__);
    if (f) {
        return read(f->fd, buf, length);
    }
    return -1;
}

/////////////////////////////////////////////////////////////////////////////
static off64_t bfile_io_seek(io_read_ops_t fd, off64_t offset, int whence) {
    bfile_reader* f = (bfile_reader*)fd;
    LOGV("%s %d\n",__FUNCTION__,__LINE__);
    if (f) {
        return lseek(f->fd, offset, whence);
    }
    return -1;
}

/////////////////////////////////////////////////////////////////////////////
// file reader operator instance
const struct io_read_ops bfile_io_ops {
    bfile_io_read,
    bfile_io_seek
};

/////////////////////////////////////////////////////////////////////////////
// File Reader
ReaderHandle FileReader_Open(char* path) {
    bfile_reader* hdl = (bfile_reader*)malloc(sizeof(bfile_reader));
    LOGV("%s %d\n",__FUNCTION__,__LINE__);
    if (hdl) {
        if (bfile_io_open(hdl, path) == -1) {
            // open fail
            free(hdl);
            return NULL;
        } else {
            hdl->file.data  = bfile_io_ops;
            hdl->file.close = bfile_io_close;
        }
    }
    return &(hdl->file);
}

/////////////////////////////////////////////////////////////////////////////
// Generic Reader interfaces
void Reader_Close(ReaderHandle hdl) {
    LOGV("%s %d\n",__FUNCTION__,__LINE__);
    if (hdl) {
        hdl->close(hdl);
    }
}

ssize_t Reader_Read(ReaderHandle hdl, void* buf, int length) {
    LOGV("%s %d\n",__FUNCTION__,__LINE__);
    if (hdl) {
        return hdl->data.read(&hdl->data, buf, length);
    }
    return -1;
}

off64_t Reader_Seek(ReaderHandle hdl, off64_t offset, int whence) {
    LOGV("%s %d\n",__FUNCTION__,__LINE__);
    if (hdl) {
        return hdl->data.seek(&hdl->data, offset, whence);
    }
    return -1;
}

/////////////////////////////////////////////////////////////////////////////
// test main
int main(int argc, char** argv) {
    char file_name[256] = {0};
    ReaderHandle reader;
    char buffer[4];
    int i;
    ssize_t read_len;
    off64_t pos, start_pos, end_pos;
    if (argc > 1) {
        strncpy(file_name, argv[1], 256);
    }
    reader = FileReader_Open(file_name);
    if (!reader) {
        fprintf(stderr,"ERROR: cannot open file '%s'\n", file_name);
        return -1;
    }
    
    printf("Open file name: %s\n", file_name);
    
    // read head 4 bytes
    pos = Reader_Seek(reader, 0, SEEK_CUR);
    printf("Current pos:%" PRId64 "\n", pos);
    if (pos!=0) {
        printf("Current position is not head, move to head now\n");
        pos = Reader_Seek(reader, 0, SEEK_SET);
        if (pos!=0) {
            fprintf(stderr,"seek to file head fail\n");
        }
    }
    read_len = Reader_Read(reader, buffer, sizeof(buffer));
    printf("head bytes: ");
    for (i=0; i<read_len; ++i) {
        printf("%02X ", buffer[i]&0xff);
    }
    puts("");
    
    // read meddle 4 bytes
    start_pos = Reader_Seek(reader, 0, SEEK_CUR);
    end_pos   = Reader_Seek(reader, 0, SEEK_END);
    if (start_pos<0 || end_pos<0) {
        fprintf(stderr,"cannot read head/tail posistion\n");
    }
    else {
        off64_t middle_pos = end_pos - start_pos;
        pos = Reader_Seek(reader, middle_pos, SEEK_SET);
        if (pos != middle_pos) {
            fprintf(stderr,"cannot seek to middle position\n");
        }
        else {
            printf("Move to middle pos %d\n",middle_pos);
        }
    }
    read_len = Reader_Read(reader, buffer, sizeof(buffer));
    printf("middle bytes: ");
    for (int i=0; i<read_len; ++i) {
        printf("%02X ", buffer[i]&0xff);
    }
    puts("");
    
    // read tail 4 bytes
    pos = Reader_Seek(reader, -sizeof(buffer), SEEK_END);
    if (pos != -1) printf("Move to tail pos %d\n",pos);
    read_len = Reader_Read(reader, buffer, sizeof(buffer));
    printf("final bytes: ");
    for (int i=0; i<read_len; ++i) {
        printf("%02X ", buffer[i]&0xff);
    }
    puts("");
    
    Reader_Close(reader);
    
    return 0;
}
