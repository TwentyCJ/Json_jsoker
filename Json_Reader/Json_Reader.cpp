// Json_Reader.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string>
#include "ab.h"

int main()
{
	Json_Reader *j=new Json_Reader;
	wchar_t*r = L"{1:a,2:b,3:[4,5,6],7:[{8:c,9:d},{10:[11,12,13]}],14:e,15:f}";
	Jobject *t = j->Analysis(r,wcslen(r));
	wchar_t *p = L"15";
	wprintf(t->Find_char(p));
	Jreader *pp = t->Find_array(L"3");
	if (!pp->Is_jobject())
	{
		Jarray *d = reinterpret_cast<Jarray*>(pp);
		wprintf((*(d))[2]);
	}
	getchar();
    return 0;
}

