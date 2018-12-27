// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

#define BOOST_FILESYSTEM_VERSION 3
#define BOOST_NO_CXX11_SCOPED_ENUMS

#include <mutex>
#include <system_error>
#include <boost/filesystem.hpp>

#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "misc_language.h"
#include "p2p/p2p_protocol_defs.h"

#if defined(WIN32) 
#include <dbghelp.h>
#endif

namespace tools
{
  std::string get_host_computer_name();
  std::string get_default_data_dir();
  std::string get_default_user_dir();
  std::string get_current_username();
  std::string get_os_version_string();
  bool create_directories_if_necessary(const std::string& path);
  std::error_code replace_file(const std::string& replacement_name, const std::string& replaced_name);

  inline crypto::hash get_proof_of_trust_hash(const nodetool::proof_of_trust& pot)
  {
    std::string s;
    s.append(reinterpret_cast<const char*>(&pot.peer_id), sizeof(pot.peer_id));
    s.append(reinterpret_cast<const char*>(&pot.time), sizeof(pot.time));
    return crypto::cn_fast_hash(s.data(), s.size());
  }

  inline 
  crypto::public_key get_public_key_from_string(const std::string& str_key)
  {
    crypto::public_key k = AUTO_VAL_INIT(k);
    epee::string_tools::hex_to_pod(str_key, k);
    return k;
  }


  class signal_handler
  {
  public:
    template<typename T>
    static bool install(T t)
    {
#if defined(WIN32)
      bool r = TRUE == ::SetConsoleCtrlHandler(&win_handler, TRUE);
      if (r)
      {
        m_handler = t;
      }
      return r;
#else
      signal(SIGINT, posix_handler);
      signal(SIGTERM, posix_handler);
      m_handler = t;
      return true;
#endif
    }

    template<typename T>
    static void install_fatal(T t)
    {
#if defined(WIN32)
      // NOTE: Unfortunately, there's no way to handle heap corruption signals/exceptions. More info: https://connect.microsoft.com/VisualStudio/feedback/details/664497/cant-catch-0xc0000374-exception-status-heap-corruption
      ::SetUnhandledExceptionFilter(win_unhandled_exception_handler);
#else
      signal(SIGABRT, posix_fatal_handler);
      signal(SIGSEGV, posix_fatal_handler);
      signal(SIGILL,  posix_fatal_handler);
#endif

      m_fatal_handler = t;
    }

    static void uninstall_fatal()
    {
#if defined(WIN32)
      ::SetUnhandledExceptionFilter(NULL);
#endif
      signal(SIGABRT, SIG_DFL);
      signal(SIGSEGV, SIG_DFL);
      signal(SIGILL,  SIG_DFL);
      m_fatal_handler = &noop_fatal_handler;
    }

  private:
#if defined(WIN32)
    static BOOL WINAPI win_handler(DWORD type)
    {
      if (CTRL_C_EVENT == type || CTRL_BREAK_EVENT == type)
      {
        handle_signal();
        return TRUE;
      }
      else
      {
        LOG_PRINT_RED_L0("Got control signal " << type << ". Exiting without saving...");
        return FALSE;
      }
      return TRUE;
    }

    /************************************************************************/
    // This code borrowed from http://www.debuginfo.com/articles/effminidumps2.html 
    /************************************************************************/

    ///////////////////////////////////////////////////////////////////////////////
    // This function determines whether we need data sections of the given module 
    //

    static bool IsDataSectionNeeded(const WCHAR* pModuleName)
    {
      // Check parameters 
      if (pModuleName == 0)
      {
        return false;
      }

      // Extract the module name 

      WCHAR szFileName[_MAX_FNAME] = L"";

      _wsplitpath(pModuleName, NULL, NULL, szFileName, NULL);
      // Compare the name with the list of known names and decide 
      // Note: For this to work, the executable name must be "mididump.exe"
      if (wcsicmp(szFileName, L"mididump") == 0)
      {
        return true;
      }
      else if (wcsicmp(szFileName, L"ntdll") == 0)
      {
        return true;
      }
      // Complete 
      return false;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Custom minidump callback 
    //

    static BOOL CALLBACK MyMiniDumpCallback(PVOID pParam, const PMINIDUMP_CALLBACK_INPUT   pInput,
      PMINIDUMP_CALLBACK_OUTPUT pOutput)
    {
      BOOL bRet = FALSE;
      // Check parameters 
      if (pInput == 0)
        return FALSE;

      if (pOutput == 0)
        return FALSE;

      // Process the callbacks 
      switch (pInput->CallbackType)
      {
      case IncludeModuleCallback:
      {
        // Include the module into the dump 
        bRet = TRUE;
      }
      break;

      case IncludeThreadCallback:
      {
        // Include the thread into the dump 
        bRet = TRUE;
      }
      break;

      case ModuleCallback:
      {
        // Are data sections available for this module ? 

        if (pOutput->ModuleWriteFlags & ModuleWriteDataSeg)
        {
          // Yes, they are, but do we need them? 

          if (!IsDataSectionNeeded(pInput->Module.FullPath))
          {
            wprintf(L"Excluding module data sections: %s \n", pInput->Module.FullPath);

            pOutput->ModuleWriteFlags &= (~ModuleWriteDataSeg);
          }
        }

        bRet = TRUE;
      }
      break;

      case ThreadCallback:
      {
        // Include all thread information into the minidump 
        bRet = TRUE;
      }
      break;

      case ThreadExCallback:
      {
        // Include this information 
        bRet = TRUE;
      }
      break;

      case MemoryCallback:
      {
        // We do not include any information here -> return FALSE 
        bRet = FALSE;
      }
      break;

      case CancelCallback:
        break;
      }

      return bRet;

    }

    static void GenerateCrashDump(EXCEPTION_POINTERS *pep = NULL)

    {
      SYSTEMTIME sysTime = { 0 };
      GetSystemTime(&sysTime);
      // get the computer name
      char compName[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
      DWORD compNameLen = ARRAYSIZE(compName);
      GetComputerNameA(compName, &compNameLen);
      // build the filename: APPNAME_COMPUTERNAME_DATE_TIME.DMP
      char path[MAX_PATH*10] = { 0 };
      std::string folder = epee::log_space::log_singletone::get_default_log_folder();
      sprintf_s(path, ARRAYSIZE(path),"%s\\crashdump_%s_%04u-%02u-%02u_%02u-%02u-%02u.dmp",
        folder.c_str(), compName, sysTime.wYear, sysTime.wMonth, sysTime.wDay,
        sysTime.wHour, sysTime.wMinute, sysTime.wSecond);

      HANDLE hFile = CreateFileA(path, GENERIC_READ | GENERIC_WRITE,
        0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

      if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
      {
        // Create the minidump 
        MINIDUMP_EXCEPTION_INFORMATION mdei;

        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = pep;
        mdei.ClientPointers = FALSE;

        MINIDUMP_CALLBACK_INFORMATION mci;

        mci.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)MyMiniDumpCallback;
        mci.CallbackParam = 0;

        MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)(MiniDumpWithPrivateReadWriteMemory |
          MiniDumpWithDataSegs |
          MiniDumpWithHandleData |
          MiniDumpWithFullMemoryInfo |
          MiniDumpWithThreadInfo |
          MiniDumpWithUnloadedModules);

        BOOL rv = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
          hFile, mdt, (pep != 0) ? &mdei : 0, 0, &mci);

        if (!rv)
        {
          LOG_ERROR("Minidump file create FAILED(error " << GetLastError() << ") on path: " <<  path);
        }
        else
        {
          LOG_PRINT_L0("Minidump file created on path: " << path);
        }
        // Close the file 
        CloseHandle(hFile);
      }
      else
      {
        LOG_ERROR("Minidump FAILED to create file (error " << GetLastError() << ") on path: " << path);
      }
    }



    static LONG WINAPI win_unhandled_exception_handler(_In_ struct _EXCEPTION_POINTERS *ep)
    {
      GenerateCrashDump(ep);
      handle_fatal_signal(ep->ExceptionRecord->ExceptionCode, ep->ExceptionRecord->ExceptionAddress);
      return EXCEPTION_EXECUTE_HANDLER;
    }

#else
    static void posix_handler(int /*type*/)
    {
      handle_signal();
    }
#endif
    
    static void posix_fatal_handler(int sig_number)
    {
      handle_fatal_signal(sig_number, 0);
    }

    static void noop_fatal_handler(int, void*) {}

    static void handle_signal()
    {
      static std::mutex m_mutex;
      std::unique_lock<std::mutex> lock(m_mutex);
      m_handler();
    }

    static void handle_fatal_signal(int sig_number, void* address)
    {
      static std::mutex m_mutex_fatal;
      std::unique_lock<std::mutex> lock(m_mutex_fatal);
      m_fatal_handler(sig_number, address);
      uninstall_fatal();
    }

  private:
    static std::function<void(void)> m_handler;
    static std::function<void(int, void*)>  m_fatal_handler;
  };
}
