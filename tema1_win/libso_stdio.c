#include <stdlib.h>
#include <stdio.h>
#include "libso_stdio.h"
#include <string.h>
#include <Windows.h>
#define INVALID_FLAG -1
#define BUFF_SIZE 4096
#define DLL_EXPORTS
#define SCRIERE 2
#define CITIRE 3

struct _so_file
{
    HANDLE handle; // file descriptor
    int OFFSET;    // pozitia cursorului in buffer
    char BUFFER[BUFF_SIZE];
    DWORD OFFSET_FILE; // pozitia cursorului in fisier
    DWORD LENGTH;      // lungimea bufferului
    int EndOF;         // sunt la final
    int CAN_READ;
    int CAN_WRITE;
    int CAN_APPEND;
    int LASTOP; // PROTOCOL: 1-citire 2-scriere 0-nimic
    int ERR;
    PROCESS_INFORMATION PID_CH;
};

SO_FILE *so_fopen(const char *pathname, const char *mode)
{

    SO_FILE *file = (SO_FILE *)malloc(sizeof(SO_FILE));
    if (file == NULL)
    {
        printf("Eroare la alocarea memoriei\n");
        file->ERR = 1;
        free(file);
        return NULL;
    }

    DWORD access_mode, opening_mode;

    if (strcmp(mode, "r") == 0)
    {
        access_mode = GENERIC_READ;
        opening_mode = OPEN_EXISTING;
        file->CAN_READ = 1;
        file->CAN_WRITE = 0;
        file->CAN_APPEND = 0;
    }
    else if (strcmp(mode, "r+") == 0)
    {
        access_mode = GENERIC_READ | GENERIC_WRITE;
        opening_mode = OPEN_EXISTING;
        file->CAN_READ = 1;
        file->CAN_WRITE = 1;
        file->CAN_APPEND = 0;
    }
    else if (strcmp(mode, "w") == 0)
    {
        access_mode = GENERIC_WRITE;
        opening_mode = CREATE_ALWAYS;
        file->CAN_READ = 0;
        file->CAN_WRITE = 1;
        file->CAN_APPEND = 0;
    }
    else if (strcmp(mode, "w+") == 0)
    {
        access_mode = GENERIC_READ | GENERIC_WRITE;
        opening_mode = CREATE_ALWAYS;
        file->CAN_READ = 1;
        file->CAN_WRITE = 1;
        file->CAN_APPEND = 0;
    }
    else if (strcmp(mode, "a") == 0)
    {
        access_mode = GENERIC_WRITE;
        opening_mode = OPEN_ALWAYS;
        file->CAN_READ = 0;
        file->CAN_WRITE = 1;
        file->CAN_APPEND = 1;
    }
    else if (strcmp(mode, "a+") == 0)
    {
        access_mode = GENERIC_READ | GENERIC_WRITE;
        opening_mode = OPEN_ALWAYS;
        file->CAN_READ = 1;
        file->CAN_WRITE = 1;
        file->CAN_APPEND = 1;
    }
    else
    {
        file->ERR = 1;
        return NULL;
    }

    file->handle = CreateFile(
        (LPCSTR)pathname,
        (DWORD)access_mode,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        (DWORD)opening_mode,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (file->handle == INVALID_HANDLE_VALUE)
    {
        perror("Eroare la creare handle\n");
        return NULL;
    }

    strcpy(file->BUFFER, "");
    file->LASTOP = 0;
    file->EndOF = 0;
    file->OFFSET = 0;
    file->OFFSET_FILE = 0;
    file->LENGTH = 0;
    file->ERR = 0;
    file->PID_CH.hProcess = INVALID_HANDLE_VALUE;
    file->PID_CH.hThread = INVALID_HANDLE_VALUE;
    return file;
}

int so_fclose(SO_FILE *stream)
{
    
    if (stream->LASTOP == SCRIERE)
        if (so_fflush(stream) == SO_EOF)
            stream->ERR = 1;
    
    if(CloseHandle(stream->handle) < 0)
    {
        stream->ERR = 1;
        stream->handle = INVALID_HANDLE_VALUE;
    }
    if(stream!= NULL)
    {
        free(stream);
        stream =NULL;
    }
    return 0;
    
} 

HANDLE so_fileno(SO_FILE *stream)
{
    return stream->handle;
}

int so_fgetc(SO_FILE *stream)
{
    if (stream->EndOF == 1)
        return SO_EOF;

    if (stream->CAN_READ == 1)
    {
        if (stream->OFFSET == 0 || stream->OFFSET == BUFF_SIZE || stream->LASTOP == SCRIERE)
        {
            int bytesRead;
            int b = ReadFile(
                stream->handle,
                &stream->BUFFER,
                BUFF_SIZE,
                &bytesRead,
                NULL
            );
            if(bytesRead == 0)
            {
                stream->EndOF = -1;
                return SO_EOF;
            }

            else if (b < 0)
            {
                stream->ERR = 1;
                return SO_EOF;
            }

            stream->LASTOP = CITIRE;
            stream->LENGTH = b;
            stream->OFFSET = 0;
        }

        stream->OFFSET++;
        stream->OFFSET_FILE++;
        return (int)stream->BUFFER[stream->OFFSET - 1];
    }
    else
        stream->ERR = 1;
    return SO_EOF;
    
}

int so_fputc(int c, SO_FILE *stream)
{
    if(stream->CAN_WRITE == 1)
    {
        if(stream->CAN_APPEND == 1 && stream->CAN_READ == 0)
        {
            if(so_fseek(stream,0,SEEK_END) == SO_EOF)
                return SO_EOF;
        }
    
        int offset,whence,p,seek;
        if(stream->LASTOP == CITIRE)
        {
            offset = stream->OFFSET_FILE;
            whence - SEEK_SET;
            if(stream->CAN_APPEND == 1 && stream->CAN_READ ==1)
            {
                offset = 0;
                whence  = SEEK_END;
            }
            seek = 1;
        }


        if(stream->CAN_APPEND == 1 && stream->CAN_READ == 1)
        {
            offset = 0;
            whence = SEEK_END;
            seek = 1;
        }
        if(seek == 1)
        {
            if(so_fseek(stream, offset, whence) == -1)
            {
                stream->ERR = 1;
                return SO_EOF;
            }
        }

        if(stream->OFFSET < BUFF_SIZE)
        {
            stream->OFFSET_FILE ++;
            stream->BUFFER[stream->OFFSET++] = (char)c;
            stream->LENGTH++;
            stream->LASTOP = SCRIERE;
            return c;
        }

        if(so_fflush(stream) == -1)
        {
            stream->ERR = 1;
            return SO_EOF;
        }
        stream->OFFSET_FILE ++;
        stream->BUFFER[stream->OFFSET++] = (char)c;
        stream->LENGTH++;
        stream->LASTOP = SCRIERE;
        return c;

    }
    else{
        stream->ERR = 1;
        return SO_EOF;
    } 

}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
    if (stream->LASTOP == CITIRE) // daca ultima op e citire
    {
        stream->OFFSET = 0;
        stream->LENGTH = 0;
        strcpy(stream->BUFFER, ""); /// se invalideaza bufferul
    }
    else if (stream->LASTOP == SCRIERE) // daca ultima op e scriere
    {
        if (so_fflush(stream) == SO_EOF) // se scrie bufferul in fisier
            return SO_EOF;
    }

    
    if(whence == SEEK_CUR)
    {
        offset += stream->OFFSET;
        whence = SEEK_SET;
    }
    int p = SetFilePointer(
        stream->handle,
        offset,
        NULL,
        (DWORD)whence
    );  
    if (p == INVALID_SET_FILE_POINTER)
    {
        stream->ERR = 1;
        return SO_EOF;
    }
       
    stream->OFFSET_FILE = p;
    return 0;
}

int so_fflush(SO_FILE *stream)
{
     int b;
    DWORD bytesWritten;
   
     if (stream->LASTOP == SCRIERE)
    {
        stream->OFFSET = 0;

        while(stream->LENGTH >0)
        {
            b = WriteFile(
                stream->handle,
                &stream->BUFFER[stream->OFFSET],
                stream->LENGTH,
                &bytesWritten,
                NULL
            );
            if(b == -1)
            {
                stream->ERR = 1;
                return SO_EOF;
            }
            stream->OFFSET += bytesWritten;
            stream->LENGTH -= bytesWritten;
        }   
        stream->OFFSET = 0;      
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
    //stream->LENGTH = 0;

    if (stream->CAN_READ == 1) // daca l-am deschis cu citire
    {

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

int so_ferror(SO_FILE *stream){
    return stream->ERR;
}

SO_FILE *so_popen(const char *command, const char *type)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    HANDLE h_read, h_write, h_parent, h_child;
    char command_exe[1024];
    SO_FILE* file = (SO_FILE*)malloc(sizeof(SO_FILE));
    if(file == NULL)
    {
        file->ERR = 1;
        free(file);
        printf("Nu s-a putut aloca memorie.\n");
        return NULL;
    }

    ZeroMemory(&pi,sizeof(pi));

    ZeroMemory(&si,sizeof(si));
    si.cb = sizeof(si);

    ZeroMemory(&sa, sizeof(sa));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;

    int b = CreatePipe(
        &h_read,
        &h_write,
        &sa,
        0 );

    if(b == -1)
        return NULL;
    
    if(strcmp(type,"r") == 0)
    {
        h_parent = h_read;
        h_child = h_write;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = h_write;

        file->CAN_READ = 1;
        file->CAN_WRITE = 0;
    }
    else if(strcmp(type,"w") == 0)
    {
        h_parent = h_write;
        h_child = h_read;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = h_read;

        file->CAN_READ = 0;
        file->CAN_WRITE = 1;
    }

    SetHandleInformation(h_parent, HANDLE_FLAG_INHERIT,0);

    strcpy(command_exe,command);
     
    b = CreateProcess(
        NULL,
        (LPCSTR)command_exe,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    if(b == -1)
    {
        CloseHandle(h_read);
        CloseHandle(h_write);
        return NULL;
    }

    CloseHandle(h_child);

    file->handle = h_parent;
    file->CAN_APPEND = 0;
    strcpy(file->BUFFER, "");
    file->LASTOP = 0;
    file->OFFSET = 0;
    file->OFFSET_FILE = 0;
    file->EndOF = 0;
    file->ERR = 0;
    file->LENGTH = 0;
    file->PID_CH.hProcess = pi.hProcess;
    file->PID_CH.hThread = pi.hThread;
    return file;
}

int so_pclose(SO_FILE *stream)
{
    PROCESS_INFORMATION pi;
    
    if(stream->PID_CH.hProcess == INVALID_HANDLE_VALUE || stream->PID_CH.hThread == INVALID_HANDLE_VALUE)
        return SO_EOF;
    else {
        pi = stream->PID_CH;
        if(so_fclose(stream) < 0)
            return SO_EOF;
        if(WaitForSingleObject(pi.hProcess,INFINITE) == WAIT_FAILED)
            return SO_EOF;
        int ok;
        int s = GetExitCodeProcess(pi.hProcess, &ok);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return ok;
    }
}

