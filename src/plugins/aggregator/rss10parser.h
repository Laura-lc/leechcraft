#ifndef RSS10PARSER_H
#define RSS10PARSER_H
#include "rssparser.h"
#include "channel.h"

class RSS10Parser : public RSSParser
{
	RSS10Parser ();
public:
	virtual ~RSS10Parser ();
    static RSS10Parser& Instance ();
    virtual bool CouldParse (const QDomDocument&) const;
private:
	channels_container_t Parse (const QDomDocument&) const;
	Item* ParseItem (const QDomElement&) const;
};

#endif

