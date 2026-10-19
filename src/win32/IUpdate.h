#ifndef VBA_WIN32_IUPDATE_H
#define VBA_WIN32_IUPDATE_H

#pragma once

class IUpdateListener
{
public:
	virtual void update() = 0;
};

#endif
