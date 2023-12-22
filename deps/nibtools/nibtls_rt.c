#include <stdio.h>	/* printk substituted by printf */
#include <unistd.h>	/* usleep() function */
#include <errno.h>	/* EINVAL */
#include <time.h>	/* PC specific includes (outb, inb) */

/* DOS stuff */
#ifdef DJGPP
#include <dos.h>			/* delay() */
#include <sys/movedata.h>	/* _dosmemgetb() */
#include "cbm.h"
#include "kernel.h"
#include "gcr.h"
#endif // DJGPP

#define TO_HANDSHAKED_READ  300000
#define TO_HANDSHAKED_WRITE 300000

static int
cbm_handshaked_write(char data, int toggle);

static int
cbm_handshaked_read(int toggle);

int
cbm_parallel_burst_write_track(int fd, unsigned char * buffer, size_t length)
{
	unsigned int i;

	disable();

	for (i = 0; i < length; i++)
	{
		if (!cbm_handshaked_write(buffer[i], i & 1))
		{
			// timeout
			enable();
			return 0;
		}
	}

	cbm_handshaked_write(0, i & 1);
	cbm_parallel_burst_read(fd);

	enable();

	return (1);
}

static int
cbm_handshaked_write(char data, int toggle)
{
	int to = 0;

	RELEASE(CLK_IN);

	if (!toggle)
	{
		while (GET(DATA_IN))
			if (to++ > TO_HANDSHAKED_WRITE)
				return (0);
	}
	else
	{
		while (!GET(DATA_IN))
			if (to++ > TO_HANDSHAKED_WRITE)
				return (0);
	}
	outportb(parport, data);

	return (1);
}

int
cbm_parallel_burst_read_track(int fd, unsigned char * buffer, unsigned int length)
{
	unsigned int i;
	int dbyte;

	disable();

	for (i = 0; i < NIB_TRACK_LENGTH; i++)
	{
		dbyte = cbm_handshaked_read(i & 1);

		if (dbyte < 0)
		{
			// timeout
			enable();
			return 0;
		}

		buffer[i] = dbyte;
	}

	cbm_parallel_burst_read(fd);

	enable();
	return (1);
}

int
cbm_parallel_burst_read_track_var(int fd, unsigned char * buffer, unsigned int length)
{
	unsigned int i;
	int dbyte;

	disable();

	for (i = 0; i < length; i++)
	{
		dbyte = cbm_handshaked_read(i & 1);

		if (dbyte < 0)
		{
			// timeout
			enable();
			return 0;
		}

		buffer[i] = dbyte;
	}

	cbm_parallel_burst_read(fd);

	enable();
	return (1);
}

static int cbm_handshaked_read(int toggle)
{
        static int oldvalue = -1;
        int returnvalue = 0;
        int returnvalue2, returnvalue3, timeoutcount;
        int to = 0;

	RELEASE(DATA_IN);	// not really needed?

	if(!toggle)
	{
	    while (GET(DATA_IN))
	        if (to++ > TO_HANDSHAKED_READ) return (-1);
	}
	else
	{
	    while (!GET(DATA_IN))
		    if (to++ > TO_HANDSHAKED_READ) return (-1);
	}

        timeoutcount = 0;

        returnvalue3 = inportb(parport);
        returnvalue2 = ~returnvalue3;    // ensure to read once more

        do {
                if (++timeoutcount >= 8)
                {
                        printf("Triple-Debounce TIMEOUT: 0x%02x, 0x%02x, 0x%02x (%d, 0x%02x)\n",
                            returnvalue, returnvalue2, returnvalue3, timeoutcount, oldvalue);
                        break;
                }
                returnvalue  = returnvalue2;
                returnvalue2 = returnvalue3;
                returnvalue3 = inportb(parport);
        } while ((returnvalue != returnvalue2) || (returnvalue != returnvalue3));

        oldvalue = returnvalue;

        return returnvalue;
}
