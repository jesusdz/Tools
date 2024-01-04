/*
 *
 * tools_cparser.h
 * Author: Jesus Diaz Garcia
 */

#ifndef TOOLS_CPARSER_H
#define TOOLS_CPARSER_H

struct CParser
{
};

CParser CParserInit();

#if defined(TOOLS_CPARSER_IMPLEMENTATION)

CParser CParserInit()
{
	CParser cparser = {};
	return cparser;
}

#endif

#endif // #ifndef TOOLS_CPARSER_H

