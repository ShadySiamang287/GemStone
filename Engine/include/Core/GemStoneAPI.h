#pragma once

#ifdef GEM_PLATFORM_WINDOWS
	#ifdef GEM_BUILD_DLL
		#define GEM_API __declspec(dllexport)
	#else
		#define GEM_API __declspec(dllimport)
	#endif
#else
	#error GemStone only supports windows!
#endif

#define THROW_IF_FAILED(expression)											\
	do {																    \
		const HRESULT result__ = (expression);								\
		if (FAILED(result__)){												\
			throw std::runtime_error("Direct3D call failed:, " #expression);\
}																			\
	} while (false);