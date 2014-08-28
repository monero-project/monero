/// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief main network-throttle (count speed and decide on limit)

#include "plotfile.h"

plotFile::plotFile(string _name, plotType _type) :
    name(_name),
    type(_type),
    enabled(true)
{
    this->qname=QString::fromStdString(name);

}

plotFile::plotFile(const plotFile &pFile) {
    this->name = pFile.name;
    this->qname = pFile.qname;
    this->type = pFile.type;
    this->enabled = enabled;
}

void plotFile::setEnabled(bool b) {
    this->enabled = b;
}

void plotFile::changeType(plotType _type) {
    this->type = _type;
}

