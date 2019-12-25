// Copyright (c) 2019, Zano Project
// Copyright (c) 2006-2019, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
#pragma once

#define COMBINE1(X,Y) X##Y  // helper macro
#define COMBINE(X,Y) COMBINE1(X,Y)
#define _STR(X) #X
#define STR(X) _STR(X)

#if defined(_MSC_VER)
#define LOCAL_FUNCTION_DEF__ __FUNCTION__
#define UNUSED_ATTRIBUTE
#else
#define LOCAL_FUNCTION_DEF__ __FUNCTION__ 
#define UNUSED_ATTRIBUTE __attribute__((unused))
#endif 

#define LOCATION_SS "[" << LOCAL_FUNCTION_DEF__ << ("] @ " __FILE__ ":" STR(__LINE__))
#define LOCATION_STR (std::string("[") + LOCAL_FUNCTION_DEF__ + "] @ " __FILE__ ":" STR(__LINE__))


//
// Try-catch helpers
//

#define TRY_ENTRY()             try {
#define CATCH_ALL_DO_NOTHING()  }catch(...) {}

#define CATCH_ENTRY_CUSTOM(location, custom_code, return_val) } \
  catch(const std::exception& ex) \
{ \
  (void)(ex); \
  LOG_ERROR("Exception at [" << location << "], what=" << ex.what()); \
  custom_code; \
  return return_val; \
} \
  catch(...) \
{ \
  LOG_ERROR("Exception at [" << location << "], generic exception \"...\""); \
  custom_code; \
  return return_val; \
}
#define CATCH_ENTRY(location, return_val) CATCH_ENTRY_CUSTOM(location, (void)0, return_val)
#define CATCH_ENTRY2(return_val) CATCH_ENTRY_CUSTOM(LOCATION_SS, (void)0, return_val)

#define CATCH_ENTRY_L0(location, return_val) CATCH_ENTRY(location, return_val)
#define CATCH_ENTRY_L1(location, return_val) CATCH_ENTRY(location, return_val)
#define CATCH_ENTRY_L2(location, return_val) CATCH_ENTRY(location, return_val)
#define CATCH_ENTRY_L3(location, return_val) CATCH_ENTRY(location, return_val)
#define CATCH_ENTRY_L4(location, return_val) CATCH_ENTRY(location, return_val)

/// @brief Catches TRY_ENTRY without returning
/// @details Useful within a dtor - but only if nested within another try block
///    (since we can still potentially throw here). See NESTED_*ENTRY()
/// @todo Exception dispatcher class
#define CATCH_ENTRY_NO_RETURN_CUSTOM(location, custom_code) } \
  catch(const std::exception& ex) \
{ \
  (void)(ex); \
  LOG_ERROR("Exception at [" << location << "], what=" << ex.what()); \
  custom_code; \
} \
  catch(...) \
{ \
  LOG_ERROR("Exception at [" << location << "], generic exception \"...\""); \
  custom_code; \
}

#define CATCH_ENTRY_NO_RETURN() CATCH_ENTRY_NO_RETURN_CUSTOM(LOCATION_SS, (void)0)

#define CATCH_ENTRY_WITH_FORWARDING_EXCEPTION() } \
  catch(const std::exception& ex) \
{ \
  LOG_ERROR("Exception at [" << LOCATION_SS << "], what=" << ex.what()); \
  throw std::runtime_error(std::string("[EXCEPTION FORWARDED]: ") + ex.what()); \
} \
  catch(...) \
{ \
  LOG_ERROR("Exception at [" << LOCATION_SS << "], generic unknown exception \"...\""); \
  throw std::runtime_error("[EXCEPTION FORWARDED]"); \
}



#define NESTED_TRY_ENTRY() try { TRY_ENTRY();

#define NESTED_CATCH_ENTRY(location) \
  CATCH_ENTRY_NO_RETURN_CUSTOM(location, {}); \
  } catch (...) {}



