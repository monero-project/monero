#ifndef WINDOWS_STREAM_H
#define WINDOWS_STREAM_H

#if defined(_WIN32)

#include <string>
#include <iostream>

class windows_stream
{
public:
    windows_stream(unsigned int pLevel);
    friend std::ostream& operator<<(std::ostream &stream, windows_stream const& object);
private:
    unsigned int mLevel = 0;
};

#endif // _WIN32

#endif // WINDOWS_STREAM_H
