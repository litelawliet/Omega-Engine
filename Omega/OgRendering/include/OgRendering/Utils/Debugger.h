#pragma once

#include <vulkan/vulkan.h>

#if defined(_WIN32)
	#define DBG_ASSERT(f) { if(!(f)){ __debugbreak(); }; }
#else
	#define DBG_ASSERT(f) {  } // Other operating system debug
#endif

// NAN Test
#define DBG_VALID(f) { if ( (f)!=(f) ) { DBG_ASSERT(false);} }

// Assert with message
#define DBG_ASSERT_MSG( val, errmsg ) \
	if (!(val))						\
	{								\
		DBG_ASSERT( false )			\
	}

#define DBG_ASSERT_VULKAN_MSG( val, errmsg )	\
	if (!((VK_SUCCESS == (val))))				\
	{											\
		DBG_ASSERT( false );					\
	}
