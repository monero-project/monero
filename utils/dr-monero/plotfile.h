/// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief main network-throttle (count speed and decide on limit)

#ifndef PLOTFILE_H
#define PLOTFILE_H

#include <string>
#include <iostream>

#include <QString>

#define DBG __FUNCTION__<<":"<< __LINE__<<"\t"

using namespace std;

enum  plotType {histogram, curve} ;

class plotFile
{
public:
    plotFile(string _name,  plotType _type);
    plotFile(const plotFile &pFile);
    string getName() { return name; } ;
    QString getQName() { return qname; } ;
    plotType getType() { return type; } ;
    bool isEnabled() { return enabled; } ;
    void setEnabled(bool b);
    void changeType(plotType _type);

private:
    string name;
    QString qname;
    plotType type;
    bool enabled;
};

#endif // PLOTFILE_H
