#include "libso_stdio.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#define BUFF_SIZE 4096
#define CITIRE 2
#define SCRIERE 3
// pentru capetele pipe-ului
#define READ_END 0
#define WRITE_END 1

struct _so_file
{
    int FD;     // file descriptor
    int OFFSET; // pozitia cursorului in buffer
    char BUFFER[BUFF_SIZE];
    int OFFSET_FILE; // pozitia cursorului in fisier
    int LENGTH;      // lungimea bufferului
    int EndOF;       // sunt la final
    int CAN_READ;
    int CAN_WRITE;
    int LASTOP; // PROTOCOL: 1-citire 2-scriere 0-nimic
    int ERROR;
    int PID_CH;
};

SO_FILE *so_fopen(const char *pathname, const char *mode)
{

    SO_FILE *file = (SO_FILE *)malloc(sizeof(SO_FILE));
    if (file == NULL)
    {
        free(file);
        // file->ERROR = 1;
        return NULL;
    }
    if (!strcmp(mode, "r"))
    {
        file->FD = open(pathname, O_RDONLY);

        file->CAN_READ = 1;
        file->CAN_WRITE = 0;
    }
    else if (!strcmp(mode, "w"))
    {
        file->FD = open(pathname, O_WRONLY | O_TRUNC | O_CREAT);
        file->CAN_READ = 0;
        file->CAN_WRITE = 1;
    }
    else if (!strcmp(mode, "r+"))
    {
        file->FD = open(pathname, O_RDWR);
        file->CAN_READ = 1;
        file->CAN_WRITE = 1;
    }
    else if (!strcmp(mode, "w+"))
    {
        file->FD = open(pathname, O_RDWR | O_TRUNC | O_CREAT);
        file->CAN_READ = 1;
        file->CAN_WRITE = 1;
    }
    else if (!strcmp(mode, "a"))
    {
        file->FD = open(pathname, O_APPEND | O_CREAT | O_WRONLY);
        file->CAN_READ = 0;
        file->CAN_WRITE = 1;
    }
    else if (!strcmp(mode, "a+"))
    {
        file->FD = open(pathname, O_APPEND | O_CREAT | O_RDWR);
        file->CAN_READ = 1;
        file->CAN_WRITE = 1;
    }
    else
    {
        free(file);
        // file->ERROR = 1;
        return NULL;
    }

    if (file->FD < 0)
    {
        free(file);
        // file->ERROR = 1;
        return NULL;
    }
    strcpy(file->BUFFER, "");
    file->LASTOP = 0;
    file->EndOF = 0;
    file->OFFSET = 0;
    file->OFFSET_FILE = 0;
    file->LENGTH = 0;
    file->ERROR = 0;
    file->PID_CH = 0;
    return file;
}

int so_fclose(SO_FILE *stream)
{
    int f = stream->FD;
    if (stream->LASTOP == SCRIERE)
        if (so_fflush(stream) == SO_EOF)
            stream->ERROR = 1;
    free(stream);
    stream = NULL;
    if (close(f) == 0)
        return 0;
    else
        return SO_EOF;
}

int so_fileno(SO_FILE *stream)
{
    return stream->FD;
}

int so_fgetc(SO_FILE *stream)
{
    if (stream->EndOF == 1)
    {
        stream->ERROR = 1;
        return SO_EOF;
    }

    if (stream->CAN_READ == 1)
    {
        if (stream->OFFSET == 0 || stream->OFFSET == BUFF_SIZE || stream->LASTOP == SCRIERE)
        {
            int b = read(stream->FD, stream->BUFFER, BUFF_SIZE);
            if (b == 0)
            {
                stream->EndOF = 1;
                return SO_EOF;
            }
            else if (b < 0)
            {
                stream->ERROR = 1;
                return SO_EOF;
            }

            stream->LASTOP = CITIRE;
            stream->LENGTH = b;
            stream->OFFSET = 0;
        }

        if (stream->OFFSET == stream->LENGTH)
        {
            stream->EndOF = 1;
            return SO_EOF;
        }

        stream->OFFSET++;
        stream->OFFSET_FILE++;
        return (int)stream->BUFFER[stream->OFFSET - 1];
    }
    else
        stream->ERROR = 1;
    return SO_EOF;
}

int so_fputc(int c, SO_FILE *stream)
{
    // char C = ()
    if (stream->CAN_WRITE == 1)
    {
        if (stream->OFFSET == BUFF_SIZE)
        {
            if (so_fflush(stream) == SO_EOF)
            {
                stream->ERROR = 1;
                return SO_EOF;
            }
        }
        stream->BUFFER[stream->OFFSET] = (char)c;
        stream->OFFSET++;
        stream->OFFSET_FILE++;
        stream->LASTOP = SCRIERE;
        return c;
    }
    stream->ERROR = 1;
    return SO_EOF;
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
    if (stream->LASTOP == CITIRE) // daca ultima op e citire
    {
        stream->OFFSET = 0;
        strcpy(stream->BUFFER, ""); /// se invalideaza bufferul
    }
    else if (stream->LASTOP == SCRIERE) // daca ultima op e scriere
    {
        if (so_fflush(stream) == SO_EOF) // se scrie bufferul in fisier
            return SO_EOF;
    }
    int p = lseek(stream->FD, offset, whence);
    if (p < 0)
        return SO_EOF;
    stream->OFFSET_FILE = p;
    return 0;
}

int so_fflush(SO_FILE *stream)
{
    if (stream->LASTOP == SCRIERE)
    {
        if (write(stream->FD, stream->BUFFER, stream->OFFSET) == -1)
        {
            stream->ERROR = 1;
            return SO_EOF;
        }

        strcpy(stream->BUFFER, "");
        stream->OFFSET = 0;
        // stream->LENGTH = 0;
        return 0;
    }
    return SO_EOF;
}

long so_ftell(SO_FILE *stream)
{

    return stream->OFFSET_FILE;
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    stream->LENGTH = 0;

    if (stream->CAN_READ == 1) // daca l-am deschis cu citire
    {

        if (stream->LASTOP == SCRIERE)
        {
            if (so_fflush(stream) == SO_EOF)
            {
                stream->ERROR = 1;
                return 0;
            }
        }

        char c;
        int i = 0;
        int n = size * nmemb; // cati octeti vreau sa citesc
        char *buff = (char *)ptr;
        while (i < n)
        {
            c = so_fgetc(stream); // citesc byte cu byte
            if (c == SO_EOF)      // daca ajung la EOF
            {
                buff[i] = '\0';
                return 0; // returnez 0 sa semnalez ca am terminat mai repede citirea
            }
            if (stream->ERROR == 1 || stream->EndOF == 1)
                return 0;
            buff[i] = (unsigned char)c; // pun textul citit in buffer
            i++;
        }
        buff[i] = '\0';

        stream->LASTOP = CITIRE;
        return i / size; // returnez nr de elemente citite
    }

    return 0;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    if (stream->CAN_WRITE == 1)
    {
        int i = 0;
        char *buff = (char *)ptr;
        while (i < size * nmemb)
        {
            so_fputc((int)buff[i], stream);
            if (stream->EndOF)
                break;
            i++;
        }
        stream->LASTOP = SCRIERE;
        return nmemb;
    }
    return SO_EOF;
}

int so_feof(SO_FILE *stream)
{
    return stream->EndOF;
}

int so_ferror(SO_FILE *stream)
{
    return stream->ERROR;
}

SO_FILE *so_popen(const char *command, const char *type)
{
    int pipe_fd[2];
    int ret = pipe(pipe_fd);
    if (ret < 0)
    {
        printf("ERROR on creating pipe: ");
        return NULL;
    }

    SO_FILE *file = (SO_FILE *)malloc(sizeof(SO_FILE));
    if (file == NULL)
    {
        file->ERROR = 1;
        free(file);
        printf("Eroare la alocarea memoriei\n");
        return NULL;
    }

    int pid = fork();
    if (pid > 0) // in procesul parinte
    {
        if (strcmp(type, "r") == 0) // vreau sa citesc
        {
            close(pipe_fd[WRITE_END]); // inchid capatul de scriere
            file->FD = pipe_fd[READ_END];
            file->CAN_READ = 1;
            file->CAN_WRITE = 0;
        }
        else if (strcmp(type, "w") == 0) // vreau sa scriu
        {
            close(pipe_fd[READ_END]); // inchid capatul de citire
            file->FD = pipe_fd[WRITE_END];
            file->CAN_READ = 0;
            file->CAN_WRITE = 1;
        }
        else
        {
            printf("Eroare la setarea tipului\n");
            return NULL;
        }
    }
    else if (pid == 0) // daca ma aflu in procesul copil
    {
        if (strcmp(type, "r") == 0) // parintele citeste, copilul scrie
        {
            close(pipe_fd[READ_END]); // inchid capatul de scriere
            dup2(pipe_fd[WRITE_END], STDOUT_FILENO);
            close(pipe_fd[WRITE_END]);
        }
        else if (strcmp(type, "w"))
        {
            close(pipe_fd[WRITE_END]);
            dup2(pipe_fd[READ_END], STDIN_FILENO);
            close(pipe_fd[READ_END]);
        }
        else
        {
            printf("Eroare la setarea tipului\n");
            free(file);
            return NULL;
        }

        int ex = execlp("sh", "sh", "-c", command, NULL);
        exit(1);
    }
    else
    {
        close(pipe_fd[READ_END]);
        close(pipe_fd[WRITE_END]);
        free(file);
        return NULL;
    }

    strcpy(file->BUFFER, "");
    file->PID_CH = pid;
    file->OFFSET = 0;
    file->OFFSET_FILE = 0;
    file->EndOF = 0;
    file->ERROR = 0;
    file->LENGTH = 0;
    file->LASTOP = 0;
    return file;
}

int so_pclose(SO_FILE *stream)
{
    if (stream->PID_CH == -1)
        return SO_EOF;
    else
    {
        if (so_fclose(stream) < 0)
            return SO_EOF;
        int s;
        int w = waitpid(stream->PID_CH, &s, 0);
        if (w < 0)
            return w;
        return s;
    }
}