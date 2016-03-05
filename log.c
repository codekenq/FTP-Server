#include "log.h"

void write_log(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	FILE *f = fopen("./log", "a");

	if(fmt == NULL)
		return;
	while(*fmt)
	{
		if(*fmt == '%')
		{
			if((fmt + 1) != NULL && *(fmt + 1) == 'd')
			{
				fprintf(f, "%d", (int)va_arg(ap, int));
				fmt += 2;
			}
			else if((fmt + 1) != NULL && *(fmt + 1) == 's')
			{
				fprintf(f, "%s", (char *)va_arg(ap, char *));
				fmt += 2;
			}
			else if((fmt + 1) != NULL && *(fmt + 1) == 'f')
			{
				fprintf(f, "%f", (double)va_arg(ap, double));
				fmt += 2;
			}
			else
			{
				fprintf(f, "%c", *fmt);
				fmt++;
			}
		}
		else
		{
			fprintf(f, "%c", *fmt);
			fmt++;
		}
	}
	va_end(ap);
	fclose(f);
}
