#include "_string.h"

//此处要保证输入的src为一个str, 该函数保证输出的res为一个str,注意空间res必须得足够
void removeChar(const char *src, char *res, char c)
{
	int len = strlen(src);
	int i, j;

	for(i = 0, j = 0; i < len; i++)
		if(src[i] != c)
			res[j++] = src[i];
	res[j] = 0;
}

void removeCharSelf(char *str, char c)
{
	int len = strlen(str);
	int i, j;

	i = j = 0;
	while(i < len)
	{
		if(str[i] == c)
		{
			i++;
			continue;
		}
		str[j++] = str[i++];
	}
	str[j] = 0;
}
