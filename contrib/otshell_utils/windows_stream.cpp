#if defined(_WIN32)
#include "windows_stream.h"
#include <windows.h>

windows_stream::windows_stream(unsigned int pLevel)
    :
      mLevel(pLevel)
{
}

std::ostream& operator << (std::ostream &stream, windows_stream const& object)
{
    HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);

    if (object.mLevel >= 100)
    {
        SetConsoleTextAttribute(h_stdout, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_INTENSITY);
        return stream;
    }
    if (object.mLevel >= 90)
    {
        SetConsoleTextAttribute(h_stdout, FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY);
        return stream;
    }
    if (object.mLevel >=  80)
    {
        SetConsoleTextAttribute(h_stdout, BACKGROUND_BLUE | BACKGROUND_RED | BACKGROUND_INTENSITY);
        return stream;
    }
    if (object.mLevel >=  75)
    {
        SetConsoleTextAttribute(h_stdout, BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY);
        return stream;
    }
    if (object.mLevel >=  70)
    {
        SetConsoleTextAttribute(h_stdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        return stream;
    }
    if (object.mLevel >=  50)
    {
        SetConsoleTextAttribute(h_stdout, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        return stream;
    }
    if (object.mLevel >=  40)
    {
        SetConsoleTextAttribute(h_stdout, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY);
        return stream;
    }
    if (object.mLevel >=  30)
    {
        SetConsoleTextAttribute(h_stdout, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        return stream;
    }
    if (object.mLevel >=  20)
    {
        SetConsoleTextAttribute(h_stdout, FOREGROUND_BLUE);
        return stream;
    }

    return stream;
}

#endif
