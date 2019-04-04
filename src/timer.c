#include <xpltimer.h>


#if defined(SOLARIS) || defined(LINUX) || defined(S390RH) || defined(MACOSX)


EXPORT int XplTimeoutSet( XplTimeout *timeout, int milliseconds )
{
	int	previous = *timeout;

	errno= 0;

	*timeout = milliseconds;

	return( previous );
}

EXPORT int XplTimeoutGet( XplTimeout *timeout )
{
	errno = 0;
    return( *timeout );
}

int
XplCPUTimerCmp( XplCPUTimer *left, XplCPUTimer *right )
{
	if( left->tv_sec > right->tv_sec ) {
		return 1;
	}

	if( left->tv_sec < right->tv_sec ) {
		return -1;
	}

	/* seconds are equal */

	if( left->tv_nsec > right->tv_nsec ) {
		return 1;
	}

	if( left->tv_nsec < right->tv_nsec ) {
		return -1;
	}

	return 0;
}


void
XplCPUTimerDivide( XplCPUTimer *dividend, unsigned int divisor, XplCPUTimer *quotient )
{
	uint64 dividendNS;
	uint64 quotientNS;
	uint64 quotientS;

	dividendNS = ( ( uint64 )( dividend->tv_sec) * 1000000000 ) + ( uint64 )dividend->tv_nsec;
	quotientNS =  dividendNS / divisor ;
	quotientS = quotientNS / 1000000000;
	quotientNS = quotientNS % 1000000000;

	quotient->tv_sec = ( time_t )quotientS;
	quotient->tv_nsec = ( long )quotientNS;
}

unsigned long
XplCPUTimerInt( XplCPUTimer *timer, XplTimeUnit units )
{
	switch( units ) {
		case XPL_TIME_SEC:
			return( timer->tv_sec );
		case XPL_TIME_MSEC:
			return( timer->tv_nsec / 1000000 );
		case XPL_TIME_USEC:
			return( timer->tv_nsec / 1000 );
		case XPL_TIME_NSEC:
			return( timer->tv_nsec );
	}
	return( 0 );
}

void
XplCPUThreadTimerStart(XplCPUTimer *timer)
{
	if( clock_gettime( CLOCK_THREAD_CPUTIME_ID, timer ) ) {
		timer->tv_sec = 0;
		timer->tv_nsec = 0;
	}
}

void
XplCPUThreadTimerStop( XplCPUTimer *timer )
{
	struct timespec end;

	if( clock_gettime( CLOCK_THREAD_CPUTIME_ID, &end ) == 0 ) {
		XplCPUTimerSubtract( &end, timer, timer );
		return;
	}
	timer->tv_sec = 0;
	timer->tv_nsec = 0;
	return;
}

void
XplTimerStart(XplTimer *timer)
{
    struct timeval start;

    if (gettimeofday(&start, NULL) == 0) {
        timer->sec = start.tv_sec;
        timer->usec = start.tv_usec;
    } else {
        timer->sec = 0;
        timer->usec = 0;
    }

    return;
}

void
XplTimerStop(XplTimer *timer)
{
    struct timeval stop;

    if (gettimeofday(&stop, NULL) == 0) {
        if (stop.tv_sec == timer->sec) {
            timer->sec = 0;
            timer->usec = stop.tv_usec - timer->usec;
        } else {
            timer->sec = stop.tv_sec - timer->sec;

            if (stop.tv_usec <= timer->usec) {
                stop.tv_usec += 1000000;
				timer->sec--;
            }

            timer->usec = stop.tv_usec - timer->usec;
        }
    } else {
        timer->sec = 0;
        timer->usec = 0;
    }

    return;
}

#elif defined(WIN32)

EXPORT int XplTimeoutSet( XplTimeout *timeout, int milliseconds )
{
    int	previous;

	errno = 0;
    if( milliseconds < 4294968 ) {
        previous = ( timeout->tv_sec * 1000 ) + ( timeout->tv_usec / 1000 );

		timeout->tv_sec = milliseconds / 1000;
		timeout->tv_usec = (milliseconds % 1000) * 1000;

#if defined(DEBUG_SOCKWIN)
		XplConsolePrintf("Socket Timeout set to %d ms -> %lu sec %lu usec\n",milliseconds,timeout->tv_sec,timeout->tv_usec);
#endif
        return( previous );
    }

    errno = ERANGE;
    return SOCKET_ERROR;
}

EXPORT int XplTimeoutGet( XplTimeout *timeout )
{
	errno = 0;
    return( ( timeout->tv_sec * 1000 ) + ( timeout->tv_usec / 1000 ) );
}


LARGE_INTEGER XplFrequencyPerSecond = { 0 };
LARGE_INTEGER XplFrequencyPerMilliSecond = { 0 };

EXPORT int XplCPUTimerCmp( XplCPUTimer *left, XplCPUTimer *right )
{
	if( *left > *right ) {
		return 1;
	}

	if( *left < *right ) {
		return -1;
	}

	return 0;
}

EXPORT void XplCPUTimerDivide( XplCPUTimer *dividend, unsigned int divisor, XplCPUTimer *quotient )
{
	/* FIXME - needs implemented for win32 */
//	*result = 0;
}

EXPORT unsigned long XplCPUTimerInt( XplCPUTimer *timer, XplTimeUnit units )
{
	/* FIXME - needs implemented for win32 */
	return( 0 );
}

EXPORT void XplCPUThreadTimerStart(XplCPUTimer *timer)
{
	/* FIXME - needs implemented for win32 */
	*timer = 0;
}

EXPORT void XplCPUThreadTimerStop(XplCPUTimer *timer)
{
	/* FIXME - needs implemented for win32 */
	*timer = 0;
}

EXPORT void XplTimerStart(XplTimer *timer)
{
    LARGE_INTEGER start;
	/*
	   the 'start' variable may seem redundant, but QueryPerformanceCounter() requires
	   a byte aligned structure and we can't assume the one in the timer is aligned.
	   To be safe, we are calling it with a stack variable and then copying what we need
	   into the timer argument
	*/
    timer->sec = 0;
    timer->usec = 0;

    do {
        if (XplFrequencyPerSecond.QuadPart) {
		  if (QueryPerformanceCounter( &start ) ) {
			    timer->usable = TRUE;
			    timer->start.QuadPart = start.QuadPart;
				timer->sec = start.QuadPart / XplFrequencyPerSecond.QuadPart;
				start.QuadPart -=  ( (timer->sec) * XplFrequencyPerSecond.QuadPart );
				timer->usec = start.QuadPart / XplFrequencyPerMilliSecond.QuadPart;
				return;
			} else {
			  int err = GetLastError();
			  DebugPrintf( "XplTimerStart() failed because QueryPerformanceCounter failed with error %s\n", err );
			  DebugAssert( 0 ); // QueryPerformanceCounter() is not working.  It might be a platorm thing ( rodney )
			}
            break;
        }

        if (QueryPerformanceFrequency((LARGE_INTEGER *)&XplFrequencyPerSecond)) {
            if (XplFrequencyPerSecond.QuadPart) {
                if (XplFrequencyPerSecond.QuadPart >= 1000) {
                    XplFrequencyPerMilliSecond.QuadPart = XplFrequencyPerSecond.QuadPart / 1000;
                } else {
                    XplFrequencyPerMilliSecond.QuadPart = 10;
                }

                continue;
            }
        }

        break;
    } while (TRUE);

    timer->usable = FALSE;
    return;
}

EXPORT void XplTimerStop(XplTimer *timer)
{
    LARGE_INTEGER stop;

    if (QueryPerformanceCounter(&stop)) {
        if (timer->usable) {
            stop.QuadPart -= timer->start.QuadPart;

            timer->sec = stop.QuadPart / XplFrequencyPerSecond.QuadPart;
            stop.QuadPart -= (timer->sec) * XplFrequencyPerSecond.QuadPart;

            timer->usec = stop.QuadPart / XplFrequencyPerMilliSecond.QuadPart;
        }

        return;
    }

    return;
}

#endif


EXPORT int XplTimerCmp( XplTimer *left, XplTimer *right )
{
	if( left->sec > right->sec ) {
		return 1;
	}

	if( left->sec < right->sec ) {
		return -1;
	}

	/* seconds are equal */

	if( left->usec > right->usec ) {
		return 1;
	}

	if( left->usec < right->usec ) {
		return -1;
	}

	return 0;
}

EXPORT void XplTimerDivide( XplTimer *dividend, unsigned int divisor, XplTimer *quotient )
{
	uint64 dividendUS;
	uint64 quotientUS;
	uint64 quotientS;

	dividendUS = ( ( uint64 )( dividend->sec) * 1000000 ) + ( uint64 )dividend->usec;
	quotientUS =  dividendUS / divisor ;
	quotientS = quotientUS / 1000000;
	quotientUS = quotientUS % 1000000;

	quotient->sec = ( time_t )quotientS;
	quotient->usec = ( long )quotientUS;
}

EXPORT unsigned long XplTimerInt( XplTimer *timer, XplTimeUnit units )
{
	switch( units ) {
		case XPL_TIME_SEC:
			return( timer->sec );
		case XPL_TIME_MSEC:
			return( timer->usec / 1000 );
		case XPL_TIME_USEC:
			return( timer->usec );
		case XPL_TIME_NSEC:
			return( timer->usec * 1000 );
	}
	return( 0 );
}

EXPORT XplTimer *XplTimerSplit( XplTimer *startTime, XplTimer *splitTime )
{
	XplTimer stopTime;

	XplTimerStart( &stopTime );

	splitTime->sec = stopTime.sec - startTime->sec;
	if( stopTime.usec < startTime->usec ) {
		(splitTime->sec)--;
		splitTime->usec = 1000000 - ( startTime->usec - stopTime.usec );
	} else {
		splitTime->usec = stopTime.usec - startTime->usec;
	}
    return( splitTime );
}

EXPORT XplTimer *XplTimerLap( XplTimer *lastTime, XplTimer *lapTime )
{
	XplTimer stopTime;

	XplTimerStart( &stopTime );

	lapTime->sec = stopTime.sec - lastTime->sec;
	if( stopTime.usec < lastTime->usec ) {
		(lapTime->sec)--;
		lapTime->usec = 1000000 - ( lastTime->usec - stopTime.usec );
	} else {
		lapTime->usec = stopTime.usec - lastTime->usec;
	}
	lastTime->sec = stopTime.sec;
	lastTime->usec = stopTime.usec;
    return( lapTime );
}

EXPORT XplTimer *XplTimerSplitAndLap( XplTimer *startTime, XplTimer *splitTime, XplTimer *lastTime, XplTimer *lapTime )
{
	XplTimer stopTime;

	XplTimerStart( &stopTime );

	splitTime->sec = stopTime.sec - startTime->sec;
	if( stopTime.usec < startTime->usec ) {
		(splitTime->sec)--;
		splitTime->usec = 1000000 - ( startTime->usec - stopTime.usec );
	} else {
		splitTime->usec = stopTime.usec - startTime->usec;
	}

	lapTime->sec = stopTime.sec - lastTime->sec;
	if( stopTime.usec < lastTime->usec ) {
		(lapTime->sec)--;
		lapTime->usec = 1000000 - ( lastTime->usec - stopTime.usec );
	} else {
		lapTime->usec = stopTime.usec - lastTime->usec;
	}
	lastTime->sec = stopTime.sec;
	lastTime->usec = stopTime.usec;
    return( lapTime );
}

EXPORT void XplTimerAccumulate( XplTimerAccumulator *accum, XplTimer *timer )
{
    if( accum->count ) {
        if( ( timer->sec > accum->max.sec ) || ( ( timer->sec == accum->max.sec ) && ( timer->usec > accum->max.usec ) ) ) {
            accum->max.sec = timer->sec;
            accum->max.usec = timer->usec;
        }

        if( ( timer->sec < accum->min.sec ) || ( ( timer->sec == accum->min.sec ) && ( timer->usec < accum->min.usec ) ) ) {
            accum->min.sec = timer->sec;
            accum->min.usec = timer->usec;
        }

        accum->total.sec += timer->sec;
        accum->total.usec += timer->usec;
        if( accum->total.usec > 1000000 ) {
            accum->total.sec++;
            accum->total.usec -= 1000000;
        }
    } else {
            accum->max.sec = timer->sec;
            accum->max.usec = timer->usec;
            accum->min.sec = timer->sec;
            accum->min.usec = timer->usec;
    }
    accum->count++;
}

EXPORT void XplTimerAverage( XplTimerAccumulator *accum, XplTimer *out )
{
    unsigned long uSec;

    if( accum->count ) {
        out->sec = accum->total.sec / accum->count;
        /* get the remainder in seconds */
        uSec = accum->total.sec % accum->count;
        /* convert that to micoseconds */
        uSec *= 1000000;
        /* add usec total that is already in microseconds */
        uSec += accum->total.usec;
        /* take the average */
        out->usec = uSec / accum->count;
        /* there is a chance that the sec remainder + the usec value will add up to be more than 1 second */
        if( out->usec > 1000000 ) {
            out->sec++;
            out->usec -= 1000000;
        }
        return;
    }
    out->sec = 0;
    out->usec = 0;
    return;
}

EXPORT void XplTimerAccumulatorPrint( XplTimerAccumulator *accum, char *label )
{
    XplTimer t;

    XplTimerAverage( accum, &t );

    XplConsolePrintf( "Ave: %1lu.%06lu Max: %1lu.%06lu Min: %1lu.%06lu %s\n", t.sec, t.usec, accum->max.sec, accum->max.usec, accum->min.sec, accum->min.usec, label );
}

EXPORT void XplTimedCheckStart(XplTimer *timer){
	XplTimerStart(timer);
}

EXPORT void XplTimedCheckStop(XplTimer *timer, const char* checkName, long checkRate , XplAtomic *abortTrigger){
	long delay;
	int i;

	XplTimerStop(timer);
	delay = (checkRate * 1000) - (XplTimerInt( timer , XPL_TIME_SEC ) * 1000  + XplTimerInt(timer, XPL_TIME_MSEC));

	if (delay < 0){
		printf("WARNING: timed check %s took %ldms more than check rate %lds\n", checkName, labs(delay),checkRate );
	} else if (delay) {
		if (abortTrigger){
			for (i=0; i <= (delay/250) ; i++ ){
				if ( XplSafeRead(*abortTrigger) == 1 ){
					break;
				}
				XplDelay(250);
			}
		} else {
			XplDelay(delay);
		}
	}
}
