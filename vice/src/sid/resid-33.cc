#include <math.h>

/* Compile ReSID to its own namespace to avoid symbol clashes with the original one, but enable new filters
this time. */
#define NEW_8580_FILTER 1
namespace RESID33
{
#include "resid/sid.h"
#include "resid/dac.cc"
#include "resid/envelope.cc"
#include "resid/extfilt.cc"
#include "resid/filter8580new.cc"
#include "resid/pot.cc"
#include "resid/sid.cc"
#include "resid/voice.cc"
#include "resid/wave.cc"
}
/* Make everything available to resid.cc, but rename the hooks-array */
namespace reSID
{
	using namespace RESID33::reSID;
}
#define resid_hooks resid33_hooks
#include "resid.cc"
