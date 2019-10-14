/**
 * @file   llmake.h
 * @author Nat Goodspeed
 * @date   2015-12-18
 * @brief  Generic llmake<Template>(arg) function to instantiate
 *         Template<decltype(arg)>(arg).
 *
 *         Many of our class templates have an accompanying helper function to
 *         make an instance with arguments of arbitrary type. llmake()
 *         eliminates the need to declare a new helper function for every such
 *         class template.
 * 
 *         also relevant:
 *
 *         Template argument deduction for class templates
 *         http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0091r3.html
 *         was apparently adopted in June 2016? Unclear when compilers will
 *         portably support this, but there is hope.
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Copyright (c) 2015, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLMAKE_H)
#define LL_LLMAKE_H

template <template<typename...> class CLASS_TEMPLATE, typename... ARGS>
CLASS_TEMPLATE<ARGS...> llmake(ARGS && ... args)
{
    return CLASS_TEMPLATE<ARGS...>(std::forward<ARGS>(args)...);
}

#endif /* ! defined(LL_LLMAKE_H) */
